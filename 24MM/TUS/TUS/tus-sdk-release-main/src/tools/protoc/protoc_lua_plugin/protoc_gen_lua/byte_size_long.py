"""
TMC CONFIDENTIAL
$JITDFLibId$
Copyright (C) 2021 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""
from google.protobuf.descriptor import Descriptor, FieldDescriptor, OneofDescriptor
from . import format
from . import helper


class ByteSizeLong:
    def __init__(self, descriptor: Descriptor, fmt: format.Format) -> None:
        self.__descriptor = descriptor
        self.__fmt = fmt

    def gen_code(self) -> None:
        # generate header
        self.__fmt.format(
            '${pkg}.${class}.byte_size_long = function(self)\n'
            '  local total_size = 0\n\n')

        # generate body
        self.__fmt.indent()
        for field in helper.get_fields_excluding_oneof(self.__descriptor):
            helper.set_field_variables(field, self.__fmt)
            helper.print_comment(field, self.__fmt)
            if helper.is_repeated(field):
                if field.type == FieldDescriptor.TYPE_STRING or field.type == FieldDescriptor.TYPE_BYTES or field.type == FieldDescriptor.TYPE_MESSAGE:
                    if helper.is_map(field):
                        self.__fmt.add_variable(
                            'field_name', field.name + '._map_')
                    # repeated string/message field
                    self.__gen_repeated_string_or_message(field)
                else:
                    # repeated primitive field
                    self.__gen_repeated_primitive(field)
            else:
                # primitive/string/message field
                self.__gen_single(field)
            self.__fmt.format('\n')

        oneof_field: OneofDescriptor
        for oneof_field in self.__descriptor.oneofs:
            helper.set_oneof_field_variables(oneof_field, self.__fmt)
            self.__fmt.format(
                'do\n'
                '  local switch = {}\n')
            self.__fmt.indent()
            field_in_oneof: FieldDescriptor
            for field_in_oneof in oneof_field.fields:
                helper.set_field_variables(field_in_oneof, self.__fmt)
                self.__fmt.format(
                    'switch["${field_name}"] = function()\n')
                self.__fmt.indent()
                self.__gen_oneof_single(field_in_oneof)
                self.__fmt.outdent()
                self.__fmt.format('end\n')
            self.__fmt.outdent()
            self.__fmt.format(
                '  switch[self.${oneof_field_name}:which_oneof()]()\n'
                'end\n\n')
        self.__fmt.outdent()

        # generate footer
        self.__fmt.format(
            '  if self:get_internal_metadata():have_unknown_fields() then\n'
            '    total_size = total_size + self:get_internal_metadata():unknown_fields():len()\n'
            '  end\n'
            '  self:set_cached_size(total_size)\n'
            '  return total_size\n'
            'end\n\n')

    def __gen_single(self, field: FieldDescriptor) -> None:
        # default value check (if the field value is default,
        # the length of serialized string of the field is 0.)
        if field.type == FieldDescriptor.TYPE_STRING or field.type == FieldDescriptor.TYPE_BYTES:
            self.__fmt.format(
                'if self.${field_name}:len() > 0 then\n')
        elif field.type == FieldDescriptor.TYPE_MESSAGE:
            self.__fmt.format(
                'if self.${field_name}:__check_not_default() then\n')
        else:
            self.__fmt.format(
                'if ${helper}.check_not_zero(self.${field_name}) then\n')
        self.__fmt.indent()

        # calc the length of serialized string of the field
        if helper.fixed_size[field.type] == -1:
            # not fixed size
            self.__fmt.format(
                'total_size = total_size + ${tag_size} +\n'
                '    ${pb}.${wire_format_lite}.${field_type}_size(self.${field_name})\n')
        else:
            # fixed size
            self.__fmt.format(
                'total_size = total_size + ${tag_size} + ${fixed_size}\n')
        self.__fmt.outdent()
        self.__fmt.format('end\n')

    def __gen_oneof_single(self, field: FieldDescriptor) -> None:
        # calc the length of serialized string of the field
        if helper.fixed_size[field.type] == -1:
            # not fixed size
            self.__fmt.format(
                'total_size = total_size + ${tag_size} +\n'
                '    ${pb}.${wire_format_lite}.${field_type}_size(self.${oneof_field_name}.${field_name})\n')
        else:
            # fixed size
            self.__fmt.format(
                'total_size = total_size + ${tag_size} + ${fixed_size}\n')

    def __gen_repeated_primitive(self, field: FieldDescriptor) -> None:
        self.__fmt.format('do\n')
        self.__fmt.indent()
        self.__fmt.format(
            'local count = #self.${field_name}\n')

        if helper.fixed_size[field.type] == -1:
            # not fixed size
            self.__fmt.format(
                'local data_size = ${pb}.${wire_format_lite}.${field_type}_size(self.${field_name})\n')
        else:
            # fixed size
            self.__fmt.format(
                'local data_size = ${fixed_size} * count\n')

        if helper.is_packed(field):
            # packed (default)
            self.__fmt.format(
                'if data_size > 0 then\n'
                '  total_size = total_size + ${tag_size} +\n'
                '      ${pb}.${wire_format_lite}.int32_size(data_size)\n'
                'end\n'
                'self._cached_byte_size_["${field_name}"] = data_size\n'
                'total_size = total_size + data_size\n')
        else:
            # [packed=false]
            self.__fmt.format(
                'total_size = total_size + (${tag_size} * count) + data_size\n')

        self.__fmt.outdent()
        self.__fmt.format('end\n')

    def __gen_repeated_string_or_message(self, field: FieldDescriptor) -> None:
        self.__fmt.format(
            'do\n'
            '  local count = #self.${field_name}\n'
            '  total_size = total_size + (${tag_size} * count)\n'
            '  for i = 1, count do\n'
            '    total_size = total_size +\n'
            '        ${pb}.${wire_format_lite}.${field_type}_size(self.${field_name}[i])\n'
            '  end\n'
            'end\n')
