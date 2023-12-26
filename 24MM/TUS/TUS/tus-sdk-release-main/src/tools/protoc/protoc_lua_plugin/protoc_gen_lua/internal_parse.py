"""
TMC CONFIDENTIAL
$JITDFLibId$
Copyright (C) 2021 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""
from google.protobuf.descriptor import Descriptor, FieldDescriptor
from google.protobuf.internal import wire_format
from typing import Tuple
from sys import stderr, exit
from . import format
from . import helper


class InternalParse:
    def __init__(self, descriptor: Descriptor, fmt: format.Format) -> None:
        self.__descriptor = descriptor
        self.__fmt = fmt

    def gen_code(self) -> None:
        def make_tag(num: int, wiretype: int) -> int:
            return num << 3 | wiretype

        def expected_tag(
                field: FieldDescriptor,
                fallback_tag: int) -> Tuple[int, int]:
            expected_wiretype = helper.wire_type_for_field_type[field.type]
            expected_tag_ = make_tag(field.number, expected_wiretype)
            if self.__is_packable(field):
                fallback_wiretype: int = wire_format.WIRETYPE_LENGTH_DELIMITED
                fallback_tag_ = make_tag(field.number, fallback_wiretype)
                if helper.is_packed(field):
                    return fallback_tag_, expected_tag_
                else:
                    return expected_tag_, fallback_tag_
            else:
                return expected_tag_, fallback_tag

        def get_tag_wire_type(tag: int) -> int:
            return tag & 7

        def should_repeat(field: FieldDescriptor, wiretype: int) -> bool:
            max_two_byte_field_number: int = 16 * 128
            if field.number < max_two_byte_field_number and helper.is_repeated(field) and (
                    self.__is_packable(field) is False or wiretype != wire_format.WIRETYPE_LENGTH_DELIMITED):
                return True
            else:
                return False

        # generate header
        self.__fmt.format(
            '${pkg}.${class}.internal_parse = function(self, ptr, ctx)\n'
            '  local success = true\n'
            '  while not ctx:done(ptr) do\n'
            '    local tag\n'
            '    ptr, tag = ${pb}.${internal}.read_tag(ptr)\n'
            '    if ptr == ${pb}.${char}.nullptr then return false, true end\n'
            '    local switch = {}\n')

        self.__fmt.indent()
        self.__fmt.indent()
        # generate body
        field: FieldDescriptor
        for field in self.__descriptor.fields:
            helper.set_field_variables(field, self.__fmt)
            if helper.is_map(field):
                self.__fmt.add_variable(
                    'field_name', field.name + '._map_')
            if helper.field_is_in_oneof(field):
                self.__fmt.add_variable(
                    'field_name', field.containing_oneof.name + '.' + field.name)

            helper.print_comment(field, self.__fmt)
            self.__fmt.format(
                'switch[${field_num}] = function()\n'
            )
            self.__fmt.indent()

            expected_tag_, fallback_tag = expected_tag(field, 0)
            self.__fmt.add_variable('expected_tag', str(expected_tag_))
            self.__fmt.add_variable('fallback_tag', str(fallback_tag))
            self.__fmt.format(
                'if (tag & 0xFF) == ${expected_tag} then\n')
            self.__fmt.indent()

            wiretype = get_tag_wire_type(expected_tag_)
            tag = make_tag(field.number, wiretype)
            tag_size: int = wire_format.Int32ByteSizeNoTag(tag)
            is_repeat = should_repeat(field, wiretype)
            self.__fmt.add_variable('tag', str(tag))
            self.__fmt.add_variable('tag_byte_size', str(tag_size))

            if is_repeat:
                self.__fmt.format(
                    'ptr = ptr - ${tag_byte_size}\n'
                    'repeat\n'
                    '  ptr = ptr + ${tag_byte_size}\n')
                self.__fmt.indent()
            self.__gen_field_body(wiretype, field)
            if is_repeat:
                self.__fmt.outdent()
                self.__fmt.format(
                    '  if not ctx:data_available(ptr) then break end\n'
                    'until not ${pb}.${internal}.expect_tag(ptr, ${tag})\n')

            self.__fmt.outdent()
            if fallback_tag:
                self.__fmt.format(
                    'elseif (tag & 0xFF) == ${fallback_tag} then\n')
                self.__fmt.indent()
                self.__gen_field_body(get_tag_wire_type(fallback_tag), field)
                self.__fmt.outdent()
            self.__fmt.outdent()
            self.__fmt.format(
                '  else\n'
                '    return switch["default"]()\n'
                '  end\n'
                '  return true, false\n'
                'end\n')

        self.__fmt.outdent()
        self.__fmt.outdent()
        # generate footer
        self.__fmt.format(
            '    switch["default"] = function()\n'
            '      local unknown\n'
            '      ptr, unknown = ${pb}.${internal}.unknown_field_parse(tag,\n'
            '          self:get_internal_metadata():unknown_fields(),\n'
            '          ptr, ctx)\n'
            '      self:get_internal_metadata():set_unknown_fields(unknown)\n'
            '      if ptr == ${pb}.${char}.nullptr then return false, true end\n'
            '      return true, false\n'
            '    end\n'
            '    local break_\n'
            '    local switch_ = switch[tag >> 3]\n'
            '    if switch_ == nil then\n'
            '      success, break_ = switch["default"]()\n'
            '    else\n'
            '      success, break_ = switch_()\n'
            '    end\n'
            '    if break_ then break end\n'
            '  end  -- while\n'
            '  if success then\n'
            '    return ptr\n'
            '  else\n'
            '    return ${pb}.${char}.nullptr\n'
            '  end\n'
            'end\n\n')

    def __gen_field_body(self, wiretype: int, field: FieldDescriptor) -> None:
        if wiretype == wire_format.WIRETYPE_VARINT:
            if field.type == FieldDescriptor.TYPE_ENUM:
                # enum
                self.__fmt.format(
                    'local val\n'
                    'ptr, val = ${pb}.${internal}.read_varint64(ptr, ${pb}.${internal}.${cpp_type})\n')
                if helper.is_repeated(field):
                    self.__fmt.format('self.${field_name}:append(val)\n')
                else:
                    self.__fmt.format('self.${field_name} = val\n')
            elif field.type == FieldDescriptor.TYPE_BOOL:
                # bool
                self.__fmt.format(
                    'local val\n'
                    'ptr, val = ${pb}.${internal}.read_varint64(ptr, ${pb}.${internal}.${cpp_type})\n')
                if helper.is_repeated(field):
                    self.__fmt.format('self.${field_name}:append(val == 1)\n')
                else:
                    self.__fmt.format('self.${field_name} = (val == 1)\n')
            else:
                # int32, int64, sint32, sint64, uint32, uint64
                if field.type == FieldDescriptor.TYPE_SINT32 or field.type == FieldDescriptor.TYPE_UINT32:
                    self.__fmt.add_variable('size', '32')
                else:
                    self.__fmt.add_variable('size', '64')
                if field.type == FieldDescriptor.TYPE_SINT32 or field.type == FieldDescriptor.TYPE_SINT64:
                    self.__fmt.add_variable('zigzag', '_zigzag')
                else:
                    self.__fmt.add_variable('zigzag', '')
                if helper.is_repeated(field):
                    self.__fmt.format(
                        'local val\n'
                        'ptr, val = ${pb}.${internal}.read_varint${zigzag}${size}(ptr, ${pb}.${internal}.${cpp_type})\n'
                        'self.${field_name}:append(val)\n')
                else:
                    self.__fmt.format(
                        'ptr, self.${field_name} = ${pb}.${internal}.read_varint${zigzag}${size}(ptr, ${pb}.${internal}.${cpp_type})\n')
            self.__fmt.format(
                'if ptr == ${pb}.${char}.nullptr then return false, true end\n')
        elif wiretype == wire_format.WIRETYPE_FIXED32 or wiretype == wire_format.WIRETYPE_FIXED64:
            # double, float, fixed32, fixed64, sfixed32, sfixed64
            if helper.is_repeated(field):
                self.__fmt.format(
                    'self.${field_name}:append(${pb}.${internal}.unaligned_load(ptr, ${pb}.${internal}.${cpp_type}))\n')
            else:
                self.__fmt.format(
                    'self.${field_name} = ${pb}.${internal}.unaligned_load(ptr, ${pb}.${internal}.${cpp_type})\n')
            self.__fmt.format(
                'ptr = ptr + ${sizeof}\n')
        elif wiretype == wire_format.WIRETYPE_LENGTH_DELIMITED:
            # all types (string, bytes, message or packable types)
            self.__gen_length_delimimited(field)
            self.__fmt.format(
                'if ptr == ${pb}.${char}.nullptr then return false, true end\n')
        else:
            # unexpected error
            # maybe group but syntax proto3 dose not support group
            print(
                'ERROR: Unexpected field type: {}({}).'.format(
                    helper.type_name[field.type],
                    field.name),
                file=stderr)
            exit(1)

    def __gen_length_delimimited(self, field: FieldDescriptor) -> None:
        if self.__is_packable(field):
            # all packable types (other than string, bytes, message)
            self.__fmt.format(
                'ptr = ${pb}.${internal}.packed_${field_type}_parser(self.${field_name}, ptr, ctx)\n')
        else:
            if field.type == FieldDescriptor.TYPE_STRING or field.type == FieldDescriptor.TYPE_BYTES:
                # string, bytes
                self.__fmt.format(
                    'local str\n'
                    'ptr, str = ${pb}.${internal}.inline_greedy_string_parser(ptr, ctx)\n')
                if field.type == FieldDescriptor.TYPE_STRING:
                    self.__fmt.format(
                        'if not ${pb}.${internal}.verify_utf8(str, ${pb}.${char}.nullptr) then return false, true end\n')
                if helper.is_repeated(field):
                    self.__fmt.format('self.${field_name}:append(str)\n')
                else:
                    self.__fmt.format('self.${field_name} = str\n')
            elif field.type == FieldDescriptor.TYPE_MESSAGE:
                # message
                if helper.is_repeated(field):
                    self.__fmt.format(
                        'ptr = ctx:parse_message(self.${field_name}:add(), ptr)\n')
                else:
                    if helper.field_is_in_oneof(field):
                        helper.set_message_field_variables(
                            field.message_type, self.__fmt)
                        self.__fmt.format(
                            'self.${field_name} = ${field_pkg}.${field_class}.new()\n')
                    self.__fmt.format(
                        'ptr = ctx:parse_message(self.${field_name}, ptr)\n')
            else:
                # unexpected error
                print(
                    'ERROR: Illegal combination for length delimited '
                    'wire field type is {}.'.format(
                        helper.type_name[field.type]),
                    file=stderr)
                exit(1)

    def __is_packable(self, field: FieldDescriptor) -> bool:
        # 'packable' is a possible type of 'packed' (neither string nor bytes nor message)
        if helper.is_repeated(field) and\
                wire_format.IsTypePackable(field.type):
            return True
        else:
            return False
