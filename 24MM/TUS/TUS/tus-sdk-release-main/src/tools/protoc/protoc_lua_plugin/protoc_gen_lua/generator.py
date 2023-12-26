"""
TMC CONFIDENTIAL
$JITDFLibId$
Copyright (C) 2021 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""
from google.protobuf.descriptor import Descriptor, FileDescriptor
from google.protobuf.descriptor_pb2 import FileOptions
from sys import stderr, exit
from typing import List, Dict
from . import format
from . import helper
from . import new
from . import internal_parse
from . import internal_serialize
from . import byte_size_long


class Generator:
    def __init__(
            self,
            file_descriptor: FileDescriptor,
            parameter: str) -> None:
        self.__file_descriptor = file_descriptor

        # set variables to format
        variables: format.Vars = {
            # namespace or class in tmc_protobuf.so
            'char': 'char',
            'eps_copy_output_stream': 'eps_copy_output_stream',
            'internal_metadata': 'internal_metadata',
            'internal': 'internal',
            'parse_context': 'parse_context',
            'wire_format_lite': 'wire_format_lite',

            # variable to which 'require' is assigned
            'pb': 'pb',
            'helper': 'helper',
            'msg_l': 'message_lite',
            'rpt_val_field': 'repeated_value_field',
            'rpt_msg_field': 'repeated_message_field',
            'oneof_field': 'oneof_field',
            'map_field': 'map_field',

            # package name
            'pkg': helper.get_package_name(file_descriptor),
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

    def generate_code(self) -> str:
        # generate header
        self.__fmt.format(
            '-- Generate By protoc-gen-lua Do not Edit\n\n'
            'local ${pb} = require("tmc_protobuf")\n'
            'local ${helper} = require("tmc_protobuf_helper")\n'
            'local ${msg_l} = require("tmc_protobuf_message_lite")\n'
            'local ${rpt_val_field} = require("tmc_protobuf_repeated_value_field")\n'
            'local ${rpt_msg_field} = require("tmc_protobuf_repeated_message_field")\n'
            'local ${oneof_field} = require("tmc_protobuf_oneof_field")\n'
            'local ${map_field} = require("tmc_protobuf_map_field")\n\n')
        # package variable (dict)
        self.__fmt.format(
            'local pkg = {}\n'
            '${pkg} = {}\n')
        # generate dependencies (import)
        dependency: FileDescriptor
        for dependency in self.__file_descriptor.dependencies:
            if dependency.syntax != 'proto3':
                print(
                    'Error: The protoc --lua_out support syntax is only proto3, even for imported proto files.',
                    file=stderr)
                exit(1)
            variables: format.Vars = {
                # package name of 'import'
                'import_pkg': helper.get_package_name(dependency),
                # file name of 'import'
                'import_file_name': helper.get_pb_file_name(dependency),
            }
            self.__fmt.set_variables(variables)
            self.__fmt.format(
                '${import_pkg} = require("${import_file_name}")\n')
        self.__fmt.format('\n')
        # generate body
        self.__gen_body(
            [msg for msg in self.__file_descriptor.message_types_by_name.values()])
        # generate footer
        self.__fmt.format(
            'return ${pkg}\n')
        return self.__fmt.string

    def __gen_body(self, message_type: List[Descriptor]) -> None:
        for msg in message_type:
            # generate class
            self.__gen_class(msg)
            # generate sub body of nested classes
            self.__gen_body(msg.nested_types)

    def __gen_class(self, message_type: Descriptor) -> None:
        variables = {
            # message's full name
            'full_name': message_type.full_name,
            # global message name which is used createing message by user
            'class': helper.get_class_name(message_type),
        }
        self.__fmt.set_variables(variables)
        # generate header
        self.__fmt.format(
            '-- Class Definition (${full_name})\n'
            '${pkg}.${class} = {}\n'
            'setmetatable(${pkg}.${class}, {__index = ${msg_l}})\n\n')

        # generate enum
        for enum_type in message_type.enum_types:
            self.__fmt.add_variable('enum_name', enum_type.name)
            self.__fmt.format(
                '${pkg}.${class}.${enum_name} = {}\n')
            for enum_value in enum_type.values:
                self.__fmt.add_variable('enum_val_name', enum_value.name)
                self.__fmt.add_variable('enum_val_num', str(enum_value.number))
                self.__fmt.format(
                    '${pkg}.${class}.${enum_name}.${enum_val_name} = ${enum_val_num}\n')
            self.__fmt.format('\n')

        # generage new() function
        new.New(message_type, self.__fmt).gen_code()
        # generate internal_parse() function
        internal_parse.InternalParse(message_type, self.__fmt).gen_code()
        # generate internal_serialize() function
        internal_serialize.InternalSerialize(
            message_type, self.__fmt).gen_code()
        # generate byte_size_long() function
        byte_size_long.ByteSizeLong(message_type, self.__fmt).gen_code()
