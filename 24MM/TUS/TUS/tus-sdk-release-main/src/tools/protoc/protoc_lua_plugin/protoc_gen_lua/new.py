"""
TMC CONFIDENTIAL
$JITDFLibId$
Copyright (C) 2021 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""
from google.protobuf.descriptor import Descriptor, FieldDescriptor, OneofDescriptor
from . import format
from . import helper


class New:
    def __init__(self, descriptor: Descriptor, fmt: format.Format) -> None:
        self.__descriptor = descriptor
        self.__fmt = fmt

    def gen_code(self) -> None:
        # generate header
        self.__fmt.format(
            '${pkg}.${class}.new = function(message)\n'
            '  local fields = {}\n')

        # generate body
        self.__fmt.indent()
        for field in helper.get_fields_excluding_oneof(self.__descriptor):
            helper.set_field_variables(field, self.__fmt)
            if field.cpp_type == FieldDescriptor.CPPTYPE_MESSAGE:
                helper.set_message_field_variables(
                    field.message_type, self.__fmt)
                if helper.is_repeated(field):
                    if helper.is_map(field):
                        # generage map field
                        self.__fmt.format(
                            'fields.${field_name} = function(map)\n'
                            '  return ${map_field}.new(${field_pkg}.${field_class}.new, map)\n'
                            'end\n')
                    else:
                        # generage repeated message field
                        self.__fmt.format(
                            'fields.${field_name} = function(msg)\n'
                            '  return ${rpt_msg_field}.new(${field_pkg}.${field_class}.new, msg)\n'
                            'end\n')
                else:
                    # generate message field
                    self.__fmt.format(
                        'fields.${field_name} = ${field_pkg}.${field_class}.new\n')
            else:
                if helper.is_repeated(field):
                    # generate repeated primitive(scalar value or enum) or
                    # repeated string field
                    self.__fmt.format(
                        'fields.${field_name} = ${rpt_val_field}.new\n')
                else:
                    # generate primitive or string field
                    self.__fmt.format('fields.${field_name} = ${def_val}\n')

        oneof_field: OneofDescriptor
        for oneof_field in self.__descriptor.oneofs:
            helper.set_oneof_field_variables(oneof_field, self.__fmt)
            self.__fmt.format(
                'fields.${oneof_field_name} = function(oneof)\n'
                '  local oneof_fields = {\n')
            self.__fmt.indent()
            self.__fmt.indent()
            for field in oneof_field.fields:
                helper.set_field_variables(field, self.__fmt)
                if field.cpp_type == FieldDescriptor.CPPTYPE_MESSAGE:
                    # generate message field
                    helper.set_message_field_variables(
                        field.message_type, self.__fmt)
                    self.__fmt.format(
                        '${field_name} = ${field_pkg}.${field_class}.new,\n')
                else:
                    # generate primitive or string field
                    self.__fmt.format('${field_name} = ${def_val},\n')
            self.__fmt.outdent()
            self.__fmt.outdent()
            self.__fmt.format(
                '  }\n'
                '  return ${oneof_field}.new(oneof_fields, oneof)\n'
                'end\n')
        self.__fmt.outdent()

        # generate footer
        self.__fmt.format(
            '  local obj = ${msg_l}.new("${full_name}", fields, message)\n'
            '  return setmetatable(obj, {__index = ${pkg}.${class}, __eq = ${msg_l}.__eq, __tostring = ${msg_l}.__tostring})\n'
            'end\n\n')
