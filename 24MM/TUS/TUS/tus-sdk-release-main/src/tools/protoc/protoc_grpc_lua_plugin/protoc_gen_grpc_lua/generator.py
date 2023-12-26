"""
TMC CONFIDENTIAL
$JITDFLibId$
Copyright (C) 2021 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""
from google.protobuf.descriptor import FileDescriptor, MethodDescriptor, ServiceDescriptor, Descriptor
from google.protobuf.descriptor_pb2 import FileOptions, MethodDescriptorProto
from sys import stderr, exit
from pathlib import PurePath
from typing import List, Dict
from . import format


class Generator:
    def __init__(
            self,
            file_descriptor: FileDescriptor,
            parameter: str) -> None:
        self.__file_descriptor = file_descriptor

        # set variables to Format
        pb_file_name = self.__get_pb_file_name(file_descriptor)
        grpc_file_name = self.__get_grpc_file_name(file_descriptor)
        variables: format.Vars = {
            # package name
            'pkg': self.__get_package_name(grpc_file_name),
            # protobuf package name
            'pb_pkg': self.__get_package_name(pb_file_name),
            # protobuf file name
            'imported_pb_file_name': self.__get_imported_pb_file_name(file_descriptor),
        }
        self.__fmt = format.Format(variables)

        # set generator option
        lite_runtime = False
        options: Dict[str, str] = self.__parse_parameter(parameter)
        first: str
        second: str
        for first, second in options.items():
            if first == 'lite':
                lite_runtime = True
            else:
                print('Error: Unknown generator option: ' + first, file=stderr)
                exit(1)

        # error if syntax is not proto3
        if file_descriptor.syntax != 'proto3':
            print(
                'Error: The protoc --lua_out support syntax is only proto3.',
                file=stderr)
            exit(1)
        options: FileOptions = file_descriptor.GetOptions()
        # error if not lite runtime
        if lite_runtime is False and\
                (options.HasField('optimize_for') is False or
                 options.optimize_for != FileOptions.LITE_RUNTIME):
            print(
                'Error: The protoc --lua_out only supports lite runtime.\n'
                'Please write "option optimize_for = LITE_RUNTIME;" in .proto file or '
                'use "lite" option like "protoc --lua_out=lite:${OUTPUT_DIR} path/to/your/proto/file".',
                file=stderr)
            exit(1)

    def generate_code(self) -> str:
        def get_method_full_name(method: MethodDescriptor) -> str:
            package: str = method.containing_service.file.package
            if package != '':
                package += '.'
            return '/' + package + method.containing_service.name + '/' + method.name

        def get_pkg(descriptor: Descriptor) -> str:
            return self.__get_package_name(
                self.__get_pb_file_name(descriptor.file))

        # generate header
        self.__fmt.format(
            '-- Generate By protoc-gen-grpc-lua Do not Edit\n\n')

        # package variable (dict)
        self.__fmt.format(
            'local pkg = {}\n'
            '${pkg} = {}\n'
            '${pb_pkg} = require("${imported_pb_file_name}")\n\n')

        # generate body
        service: ServiceDescriptor
        for service in self.__file_descriptor.services_by_name.values():
            # set variables to Format
            variables: format.Vars = {
                # service's full name
                'full_name': service.full_name,
                # class name
                'class': service.name + 'Stub',
            }
            self.__fmt.set_variables(variables)
            # generate header
            self.__fmt.format(
                '-- Class Definition (${full_name})\n'
                '${pkg}.${class} = {}\n\n')
            # generate new function
            self.__fmt.format(
                '${pkg}.${class}.new = function(channel)\n'
                '  local obj = {}\n')
            self.__fmt.indent()
            method: MethodDescriptor
            for method in service.methods:
                method_desc_proto = MethodDescriptorProto()
                method.CopyToProto(method_desc_proto)
                # set variables to Format
                variables = {
                    'method_name': method.name,
                    'method_full_name': get_method_full_name(method),
                    'client': 'stream' if method_desc_proto.client_streaming else 'unary',
                    'server': 'stream' if method_desc_proto.server_streaming else 'unary',
                    'input_pkg': get_pkg(
                        method.input_type),
                    'input_class': self.__get_class_name(
                        method.input_type),
                    'output_pkg': get_pkg(
                        method.output_type),
                    'output_class': self.__get_class_name(
                        method.output_type),
                }
                self.__fmt.set_variables(variables)
                self.__fmt.format(
                    'obj.${method_name} = channel:${client}_${server}(\n'
                    '  "${method_full_name}",\n'
                    '  ${input_pkg}.${input_class},\n'
                    '  ${output_pkg}.${output_class}\n'
                    ')\n')
            self.__fmt.outdent()
            self.__fmt.format(
                '  return obj\n'
                'end\n\n')

        # generate footer
        self.__fmt.format(
            'return ${pkg}\n')
        return self.__fmt.string

    def __get_imported_pb_file_name(
            self, file_descriptor: FileDescriptor) -> str:
        file_name = PurePath(file_descriptor.name).stem + '_pb'
        return file_name.replace('\\', '\\\\').replace('"', '\\"')

    def __get_pb_file_name(self, file_descriptor: FileDescriptor) -> str:
        path = PurePath(file_descriptor.name)
        file_name = str(path.with_name(path.stem + '_pb'))
        return file_name.replace('\\', '\\\\').replace('"', '\\"')

    def __get_grpc_file_name(self, file_descriptor: FileDescriptor) -> str:
        path = PurePath(file_descriptor.name)
        file_name = str(path.with_name(path.stem + '_pb_grpc'))
        return file_name.replace('\\', '\\\\').replace('"', '\\"')

    def __get_package_name(self, file_name: str) -> str:
        return 'pkg["{}"]'.format(file_name)

    def __parse_parameter(self, parameter: str) -> Dict[str, str]:
        parts: List[str] = parameter.split(',')
        options: Dict[str, str] = dict()
        for part in parts:
            if part == '':
                continue
            equals_pos = part.find('=')
            if equals_pos == -1:
                options[part] = ''
            else:
                options[part[:equals_pos]] = part[equals_pos + 1:]
        return options

    def __get_class_name(self, message_type: Descriptor) -> str:
        # if the message is nested, the global message name (classname) is
        # a name in which all contained message names are concatenated with
        # dots
        def get_name(message_type: Descriptor, name: str) -> str:
            if message_type.containing_type is None:
                return name
            else:
                return get_name(
                    message_type.containing_type,
                    message_type.containing_type.name + '.' + name)
        return get_name(message_type, message_type.name)
