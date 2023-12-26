"""
TMC CONFIDENTIAL
$JITDFLibId$
Copyright (C) 2021 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""
from google.protobuf.descriptor import FileDescriptor, Descriptor, FieldDescriptor, OneofDescriptor
from google.protobuf.descriptor_pb2 import FieldOptions
from google.protobuf.internal import wire_format
from pathlib import PurePath
from typing import Dict, Iterator
from . import format


wire_type_for_field_type: Dict[int, int] = {
    FieldDescriptor.TYPE_INT32: wire_format.WIRETYPE_VARINT,
    FieldDescriptor.TYPE_INT64: wire_format.WIRETYPE_VARINT,
    FieldDescriptor.TYPE_UINT32: wire_format.WIRETYPE_VARINT,
    FieldDescriptor.TYPE_UINT64: wire_format.WIRETYPE_VARINT,
    FieldDescriptor.TYPE_SINT32: wire_format.WIRETYPE_VARINT,
    FieldDescriptor.TYPE_SINT64: wire_format.WIRETYPE_VARINT,
    FieldDescriptor.TYPE_FIXED32: wire_format.WIRETYPE_FIXED32,
    FieldDescriptor.TYPE_FIXED64: wire_format.WIRETYPE_FIXED64,
    FieldDescriptor.TYPE_SFIXED32: wire_format.WIRETYPE_FIXED32,
    FieldDescriptor.TYPE_SFIXED64: wire_format.WIRETYPE_FIXED64,
    FieldDescriptor.TYPE_DOUBLE: wire_format.WIRETYPE_FIXED64,
    FieldDescriptor.TYPE_FLOAT: wire_format.WIRETYPE_FIXED32,
    FieldDescriptor.TYPE_BOOL: wire_format.WIRETYPE_VARINT,
    FieldDescriptor.TYPE_ENUM: wire_format.WIRETYPE_VARINT,
    FieldDescriptor.TYPE_STRING: wire_format.WIRETYPE_LENGTH_DELIMITED,
    FieldDescriptor.TYPE_BYTES: wire_format.WIRETYPE_LENGTH_DELIMITED,
    FieldDescriptor.TYPE_GROUP: wire_format.WIRETYPE_START_GROUP,
    FieldDescriptor.TYPE_MESSAGE: wire_format.WIRETYPE_LENGTH_DELIMITED,
}

cpp_type_name: Dict[int, str] = {
    FieldDescriptor.CPPTYPE_INT32: 'INT32',
    FieldDescriptor.CPPTYPE_INT64: 'INT64',
    FieldDescriptor.CPPTYPE_UINT32: 'UINT32',
    FieldDescriptor.CPPTYPE_UINT64: 'UINT64',
    FieldDescriptor.CPPTYPE_DOUBLE: 'DOUBLE',
    FieldDescriptor.CPPTYPE_FLOAT: 'FLOAT',
    FieldDescriptor.CPPTYPE_BOOL: 'BOOL',
    FieldDescriptor.CPPTYPE_ENUM: 'ENUM',
    FieldDescriptor.CPPTYPE_STRING: 'STRING',
    FieldDescriptor.CPPTYPE_MESSAGE: 'MESSAGE',
}

cpp_type_size: Dict[int, int] = {
    FieldDescriptor.CPPTYPE_INT32: 4,
    FieldDescriptor.CPPTYPE_INT64: 8,
    FieldDescriptor.CPPTYPE_UINT32: 4,
    FieldDescriptor.CPPTYPE_UINT64: 8,
    FieldDescriptor.CPPTYPE_DOUBLE: 8,
    FieldDescriptor.CPPTYPE_FLOAT: 4,
    FieldDescriptor.CPPTYPE_BOOL: 1,
    FieldDescriptor.CPPTYPE_ENUM: 4,
    FieldDescriptor.CPPTYPE_STRING: 0,
    FieldDescriptor.CPPTYPE_MESSAGE: 0,
}

type_name: Dict[int, str] = {
    FieldDescriptor.TYPE_INT32: 'int32',
    FieldDescriptor.TYPE_INT64: 'int64',
    FieldDescriptor.TYPE_UINT32: 'uint32',
    FieldDescriptor.TYPE_UINT64: 'uint64',
    FieldDescriptor.TYPE_SINT32: 'sint32',
    FieldDescriptor.TYPE_SINT64: 'sint64',
    FieldDescriptor.TYPE_FIXED32: 'fixed32',
    FieldDescriptor.TYPE_FIXED64: 'fixed64',
    FieldDescriptor.TYPE_SFIXED32: 'sfixed32',
    FieldDescriptor.TYPE_SFIXED64: 'sfixed64',
    FieldDescriptor.TYPE_DOUBLE: 'double',
    FieldDescriptor.TYPE_FLOAT: 'float',
    FieldDescriptor.TYPE_BOOL: 'bool',
    FieldDescriptor.TYPE_ENUM: 'enum',
    FieldDescriptor.TYPE_STRING: 'string',
    FieldDescriptor.TYPE_BYTES: 'bytes',
    FieldDescriptor.TYPE_GROUP: 'group',
    FieldDescriptor.TYPE_MESSAGE: 'message',
}

fixed_size: Dict[str, int] = {
    FieldDescriptor.TYPE_INT32: -1,
    FieldDescriptor.TYPE_INT64: -1,
    FieldDescriptor.TYPE_UINT32: -1,
    FieldDescriptor.TYPE_UINT64: -1,
    FieldDescriptor.TYPE_SINT32: -1,
    FieldDescriptor.TYPE_SINT64: -1,
    FieldDescriptor.TYPE_FIXED32: 4,
    FieldDescriptor.TYPE_FIXED64: 8,
    FieldDescriptor.TYPE_SFIXED32: 4,
    FieldDescriptor.TYPE_SFIXED64: 8,
    FieldDescriptor.TYPE_DOUBLE: 8,
    FieldDescriptor.TYPE_FLOAT: 4,
    FieldDescriptor.TYPE_BOOL: 1,
    FieldDescriptor.TYPE_ENUM: -1,
    FieldDescriptor.TYPE_STRING: -1,
    FieldDescriptor.TYPE_BYTES: -1,
    FieldDescriptor.TYPE_GROUP: -1,
    FieldDescriptor.TYPE_MESSAGE: -1,
}

default_value: Dict[str, str] = {
    FieldDescriptor.CPPTYPE_INT32: '0',
    FieldDescriptor.CPPTYPE_INT64: '0',
    FieldDescriptor.CPPTYPE_UINT32: '0',
    FieldDescriptor.CPPTYPE_UINT64: '0',
    FieldDescriptor.CPPTYPE_DOUBLE: '0.0',
    FieldDescriptor.CPPTYPE_FLOAT: '0.0',
    FieldDescriptor.CPPTYPE_ENUM: '0',
    FieldDescriptor.CPPTYPE_BOOL: 'false',
    FieldDescriptor.CPPTYPE_STRING: '""',
    FieldDescriptor.CPPTYPE_MESSAGE: '',
}


def get_pb_file_name(file_descriptor: FileDescriptor) -> str:
    path = PurePath(file_descriptor.name)
    return str(path.with_name(path.stem + '_pb'))\
        .replace('\\', '\\\\').replace('"', '\\"')


def get_package_name(file_descriptor: FileDescriptor) -> str:
    return 'pkg["{}"]'.format(get_pb_file_name(file_descriptor))


def get_class_name(message_type: Descriptor) -> str:
    # if the message is nested, the global message name (classname) is
    # a name in which all contained message names are concatenated with dots
    def get_name(message_type: Descriptor, name: str) -> str:
        if message_type.containing_type is None:
            return name
        else:
            return get_name(
                message_type.containing_type,
                message_type.containing_type.name + '.' + name)
    return get_name(message_type, message_type.name)


def set_field_variables(field: FieldDescriptor, fmt: format.Format) -> None:
    # variables for the field
    variables: format.Vars = {
        'field_name': field.name,
        'field_num': field.number,
        'field_camelcase_name': field.camelcase_name,
        'field_full_name': field.full_name,
        'field_type': type_name[field.type],
        'cpp_type': cpp_type_name[field.cpp_type],
        'sizeof': cpp_type_size[field.cpp_type],
        'fixed_size': fixed_size[field.type],
        'tag_size': wire_format.TagByteSize(field.number),
        'def_val': default_value[field.cpp_type],
    }
    if field.type == FieldDescriptor.TYPE_MESSAGE:
        variables['field_detail_type'] = '.' + field.message_type.full_name
    elif field.type == FieldDescriptor.TYPE_ENUM:
        variables['field_detail_type'] = '.' + field.enum_type.full_name
    else:
        variables['field_detail_type'] = type_name[field.type]
    fmt.set_variables(variables)


def set_message_field_variables(
        message_type: Descriptor,
        fmt: format.Format) -> None:
    variables: format.Vars = {
        # package name of the field
        'field_pkg': get_package_name(message_type.file),
        # global message name of the field
        'field_class': get_class_name(message_type),
    }
    fmt.set_variables(variables)


def set_oneof_field_variables(
        oneof_field: OneofDescriptor,
        fmt: format.Format) -> None:
    variables: format.Vars = {
        'oneof_field_name': oneof_field.name,
    }
    fmt.set_variables(variables)


def print_comment(field: FieldDescriptor, fmt: format.Format) -> None:
    if is_repeated(field):
        # comment of the repeated field
        fmt.format(
            '-- repeated ${field_detail_type} ${field_name} = ${field_num}[json_name = "${field_camelcase_name}"];\n')
    else:
        # comment of the field
        fmt.format(
            '-- ${field_detail_type} ${field_name} = ${field_num}[json_name = "${field_camelcase_name}"];\n')


def is_repeated(field: FieldDescriptor) -> bool:
    if field.label == FieldDescriptor.LABEL_REPEATED:
        return True
    else:
        return False


def is_packed(field: FieldDescriptor) -> bool:
    options: FieldOptions = field.GetOptions()
    if options.HasField('packed') is False or options.packed is True:
        # default or [packed=true]
        return True
    else:
        # [packed=false]
        return False


def is_map(field: FieldDescriptor) -> bool:
    if field.type == FieldDescriptor.TYPE_MESSAGE and field.message_type.GetOptions().map_entry is True:
        return True
    else:
        return False


def field_is_in_oneof(field: FieldDescriptor) -> bool:
    if field.containing_oneof is not None:
        return True
    else:
        return False


def get_fields_excluding_oneof(
        descriptor: Descriptor) -> Iterator[FieldDescriptor]:
    field: FieldDescriptor
    for field in descriptor.fields:
        if field_is_in_oneof(field):
            continue
        yield field
