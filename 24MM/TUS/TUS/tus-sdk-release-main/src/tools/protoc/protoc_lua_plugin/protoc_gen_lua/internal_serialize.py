"""
TMC CONFIDENTIAL
$JITDFLibId$
Copyright (C) 2021 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""
from google.protobuf.descriptor import Descriptor, FieldDescriptor
from . import format
from . import helper


class InternalSerialize:
    def __init__(self, descriptor: Descriptor, fmt: format.Format) -> None:
        self.__descriptor = descriptor
        self.__fmt = fmt

    def gen_code(self) -> None:
        # generate header
        self.__fmt.format(
            '${pkg}.${class}.internal_serialize = function(self, target, stream)\n')

        # generate body
        field: FieldDescriptor
        for field in self.__descriptor.fields:
            helper.set_field_variables(field, self.__fmt)
            self.__fmt.indent()
            helper.print_comment(field, self.__fmt)

            if helper.field_is_in_oneof(field):
                # oneof field
                helper.set_oneof_field_variables(
                    field.containing_oneof, self.__fmt)
                if field.type == FieldDescriptor.TYPE_MESSAGE:
                    # message field
                    self.__gen_oneof_message(field)
                elif field.type == FieldDescriptor.TYPE_STRING or field.type == FieldDescriptor.TYPE_BYTES:
                    # string field
                    self.__gen_oneof_string(field)
                else:
                    # primitive field
                    self.__gen_oneof_primitive(field)
            else:
                if helper.is_repeated(field):
                    if field.type == FieldDescriptor.TYPE_MESSAGE:
                        if helper.is_map(field):
                            self.__fmt.add_variable(
                                'field_name', field.name + '._map_')
                        # repeated message field
                        self.__gen_repeated_message(field)
                    elif field.type == FieldDescriptor.TYPE_STRING or field.type == FieldDescriptor.TYPE_BYTES:
                        # repeated string field
                        self.__gen_repeated_string(field)
                    else:
                        # repeated primitive field
                        self.__gen_repeated_primitive(field)
                else:
                    if field.type == FieldDescriptor.TYPE_MESSAGE:
                        # message field
                        self.__gen_message(field)
                    elif field.type == FieldDescriptor.TYPE_STRING or field.type == FieldDescriptor.TYPE_BYTES:
                        # string field
                        self.__gen_string(field)
                    else:
                        # primitive field
                        self.__gen_primitive(field)
            self.__fmt.outdent()
            self.__fmt.format('\n')

        # generate footer
        self.__fmt.format(
            '  if self:get_internal_metadata():have_unknown_fields() then\n'
            '    target = stream:write_raw(self:get_internal_metadata():unknown_fields(),\n'
            '        self:get_internal_metadata():unknown_fields():len(), target)\n'
            '  end\n'
            '  return target\n'
            'end\n\n')

    def __gen_primitive(self, field: FieldDescriptor) -> None:
        self.__fmt.format(
            'if ${helper}.check_not_zero(self.${field_name}) then\n'
            '  target = stream:ensure_space(target)\n'
            '  target = ${pb}.${wire_format_lite}.write_${field_type}_to_array(\n'
            '      ${field_num}, self.${field_name}, target)\n'
            'end\n')

    def __gen_oneof_primitive(self, field: FieldDescriptor) -> None:
        self.__fmt.format(
            'if self.${oneof_field_name}:has_field("${field_name}") then\n'
            '  target = stream:ensure_space(target)\n'
            '  target = ${pb}.${wire_format_lite}.write_${field_type}_to_array(\n'
            '      ${field_num}, self.${oneof_field_name}.${field_name}, target)\n'
            'end\n')

    def __gen_repeated_primitive(self, field: FieldDescriptor) -> None:
        if helper.is_packed(field):
            if helper.fixed_size[field.type] > 0:
                # fixed and packed
                self.__fmt.format(
                    'if #self.${field_name} > 0 then\n'
                    '  target = stream:write_fixed_packed(${field_num}, self.${field_name},\n'
                    '      target, ${pb}.${eps_copy_output_stream}.${cpp_type})\n'
                    'end\n')
            else:
                # non-fixed and packed
                self.__fmt.format(
                    'do\n'
                    '  local byte_size = self._cached_byte_size_["${field_name}"]\n'
                    '  if byte_size > 0 then\n'
                    '    target = stream:write_${field_type}_packed(\n'
                    '        ${field_num}, self.${field_name}, byte_size, target)\n'
                    '  end\n'
                    'end\n')
        else:
            # non-packed
            self.__fmt.format(
                'target = ${pb}.${wire_format_lite}.write_${field_type}_to_array(\n'
                '    ${field_num}, self.${field_name}, target)\n')

    def __gen_string(self, field: FieldDescriptor) -> None:
        self.__fmt.format(
            'if self.${field_name}:len() > 0 then\n')
        self.__fmt.indent()

        if field.type == FieldDescriptor.TYPE_STRING:
            # verify utf8 string if string
            self.__fmt.format(
                'local result = ${pb}.${wire_format_lite}.verify_utf8_string(\n'
                '    self.${field_name}, self.${field_name}:len(),\n'
                '    ${pb}.${wire_format_lite}.SERIALIZE,\n'
                '    "${field_full_name}")\n'
                'if result == false then\n'
                '  error("Checking utf8 string failed.")\n'
                'end\n')

        self.__fmt.format(
            'target = stream:write_${field_type}_maybe_aliased(\n'
            '    ${field_num}, self.${field_name}, target)\n')

        self.__fmt.outdent()
        self.__fmt.format('end\n')

    def __gen_oneof_string(self, field: FieldDescriptor) -> None:
        self.__fmt.format(
            'if self.${oneof_field_name}:has_field("${field_name}") then\n')
        self.__fmt.indent()

        if field.type == FieldDescriptor.TYPE_STRING:
            # verify utf8 string if string
            self.__fmt.format(
                'local result = ${pb}.${wire_format_lite}.verify_utf8_string(\n'
                '    self.${oneof_field_name}.${field_name}, self.${oneof_field_name}.${field_name}:len(),\n'
                '    ${pb}.${wire_format_lite}.SERIALIZE,\n'
                '    "${field_full_name}")\n'
                'if result == false then\n'
                '  error("Checking utf8 string failed.")\n'
                'end\n')

        self.__fmt.format(
            'target = stream:write_${field_type}_maybe_aliased(\n'
            '    ${field_num}, self.${oneof_field_name}.${field_name}, target)\n')

        self.__fmt.outdent()
        self.__fmt.format('end\n')

    def __gen_repeated_string(self, field: FieldDescriptor) -> None:
        self.__fmt.format(
            'for i = 1, #self.${field_name} do\n'
            '  local s = self.${field_name}[i]\n')
        self.__fmt.indent()

        if field.type == FieldDescriptor.TYPE_STRING:
            # verify utf8 string if string
            self.__fmt.format(
                '${pb}.${wire_format_lite}.verify_utf8_string(\n'
                '    s, s:len(),\n'
                '    ${pb}.${wire_format_lite}.SERIALIZE,\n'
                '    "${field_full_name}")\n')

        self.__fmt.format(
            'target = stream:write_${field_type}(${field_num}, s, target)\n')

        self.__fmt.outdent()
        self.__fmt.format('end\n')

    def __gen_message(self, field: FieldDescriptor) -> None:
        self.__fmt.format(
            'if self.${field_name}:__check_not_default() then\n'
            '  target = stream:ensure_space(target)\n'
            '  target = ${pb}.${wire_format_lite}.\n'
            '      internal_write_${field_type}(${field_num}, self.${field_name}, target, stream)\n'
            'end\n')

    def __gen_oneof_message(self, field: FieldDescriptor) -> None:
        self.__fmt.format(
            'if self.${oneof_field_name}:has_field("${field_name}") then\n'
            '  target = stream:ensure_space(target)\n'
            '  target = ${pb}.${wire_format_lite}.\n'
            '      internal_write_${field_type}(${field_num}, self.${oneof_field_name}.${field_name}, target, stream)\n'
            'end\n')

    def __gen_repeated_message(self, field: FieldDescriptor) -> None:
        self.__fmt.format(
            'for i = 1, #self.${field_name} do\n'
            '  target = stream:ensure_space(target)\n'
            '  target = ${pb}.${wire_format_lite}.\n'
            '      internal_write_${field_type}(${field_num}, self.${field_name}[i], target, stream)\n'
            'end\n')
