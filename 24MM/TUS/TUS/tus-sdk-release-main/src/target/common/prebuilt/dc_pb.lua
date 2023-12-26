-- Generate By protoc-gen-lua Do not Edit

local pb = require("tmc_protobuf")
local helper = require("tmc_protobuf_helper")
local message_lite = require("tmc_protobuf_message_lite")
local repeated_value_field = require("tmc_protobuf_repeated_value_field")
local repeated_message_field = require("tmc_protobuf_repeated_message_field")
local oneof_field = require("tmc_protobuf_oneof_field")
local map_field = require("tmc_protobuf_map_field")

local pkg = {}
pkg["dc_pb"] = {}

-- Class Definition (TUS.Names)
pkg["dc_pb"].Names = {}
setmetatable(pkg["dc_pb"].Names, {__index = message_lite})

pkg["dc_pb"].Names.new = function(message)
  local fields = {}
  fields.name = repeated_value_field.new
  local obj = message_lite.new("TUS.Names", fields, message)
  return setmetatable(obj, {__index = pkg["dc_pb"].Names, __eq = message_lite.__eq, __tostring = message_lite.__tostring})
end

pkg["dc_pb"].Names.internal_parse = function(self, ptr, ctx)
  local success = true
  while not ctx:done(ptr) do
    local tag
    ptr, tag = pb.internal.read_tag(ptr)
    if ptr == pb.char.nullptr then return false, true end
    local switch = {}
    -- repeated string name = 1[json_name = "name"];
    switch[1] = function()
      if (tag & 0xFF) == 10 then
        ptr = ptr - 1
        repeat
          ptr = ptr + 1
          local str
          ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
          if not pb.internal.verify_utf8(str, pb.char.nullptr) then return false, true end
          self.name:append(str)
          if ptr == pb.char.nullptr then return false, true end
          if not ctx:data_available(ptr) then break end
        until not pb.internal.expect_tag(ptr, 10)
      else
        return switch["default"]()
      end
      return true, false
    end
    switch["default"] = function()
      local unknown
      ptr, unknown = pb.internal.unknown_field_parse(tag,
          self:get_internal_metadata():unknown_fields(),
          ptr, ctx)
      self:get_internal_metadata():set_unknown_fields(unknown)
      if ptr == pb.char.nullptr then return false, true end
      return true, false
    end
    local break_
    local switch_ = switch[tag >> 3]
    if switch_ == nil then
      success, break_ = switch["default"]()
    else
      success, break_ = switch_()
    end
    if break_ then break end
  end  -- while
  if success then
    return ptr
  else
    return pb.char.nullptr
  end
end

pkg["dc_pb"].Names.internal_serialize = function(self, target, stream)
  -- repeated string name = 1[json_name = "name"];
  for i = 1, #self.name do
    local s = self.name[i]
    pb.wire_format_lite.verify_utf8_string(
        s, s:len(),
        pb.wire_format_lite.SERIALIZE,
        "TUS.Names.name")
    target = stream:write_string(1, s, target)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    target = stream:write_raw(self:get_internal_metadata():unknown_fields(),
        self:get_internal_metadata():unknown_fields():len(), target)
  end
  return target
end

pkg["dc_pb"].Names.byte_size_long = function(self)
  local total_size = 0

  -- repeated string name = 1[json_name = "name"];
  do
    local count = #self.name
    total_size = total_size + (1 * count)
    for i = 1, count do
      total_size = total_size +
          pb.wire_format_lite.string_size(self.name[i])
    end
  end

  if self:get_internal_metadata():have_unknown_fields() then
    total_size = total_size + self:get_internal_metadata():unknown_fields():len()
  end
  self:set_cached_size(total_size)
  return total_size
end

-- Class Definition (TUS.Versions)
pkg["dc_pb"].Versions = {}
setmetatable(pkg["dc_pb"].Versions, {__index = message_lite})

pkg["dc_pb"].Versions.new = function(message)
  local fields = {}
  fields.result_code = 0
  fields.pairs = function(msg)
    return repeated_message_field.new(pkg["dc_pb"].PairedStr.new, msg)
  end
  local obj = message_lite.new("TUS.Versions", fields, message)
  return setmetatable(obj, {__index = pkg["dc_pb"].Versions, __eq = message_lite.__eq, __tostring = message_lite.__tostring})
end

pkg["dc_pb"].Versions.internal_parse = function(self, ptr, ctx)
  local success = true
  while not ctx:done(ptr) do
    local tag
    ptr, tag = pb.internal.read_tag(ptr)
    if ptr == pb.char.nullptr then return false, true end
    local switch = {}
    -- int32 result_code = 1[json_name = "resultCode"];
    switch[1] = function()
      if (tag & 0xFF) == 8 then
        ptr, self.result_code = pb.internal.read_varint64(ptr, pb.internal.INT32)
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    -- repeated .TUS.PairedStr pairs = 2[json_name = "pairs"];
    switch[2] = function()
      if (tag & 0xFF) == 18 then
        ptr = ptr - 1
        repeat
          ptr = ptr + 1
          ptr = ctx:parse_message(self.pairs:add(), ptr)
          if ptr == pb.char.nullptr then return false, true end
          if not ctx:data_available(ptr) then break end
        until not pb.internal.expect_tag(ptr, 18)
      else
        return switch["default"]()
      end
      return true, false
    end
    switch["default"] = function()
      local unknown
      ptr, unknown = pb.internal.unknown_field_parse(tag,
          self:get_internal_metadata():unknown_fields(),
          ptr, ctx)
      self:get_internal_metadata():set_unknown_fields(unknown)
      if ptr == pb.char.nullptr then return false, true end
      return true, false
    end
    local break_
    local switch_ = switch[tag >> 3]
    if switch_ == nil then
      success, break_ = switch["default"]()
    else
      success, break_ = switch_()
    end
    if break_ then break end
  end  -- while
  if success then
    return ptr
  else
    return pb.char.nullptr
  end
end

pkg["dc_pb"].Versions.internal_serialize = function(self, target, stream)
  -- int32 result_code = 1[json_name = "resultCode"];
  if helper.check_not_zero(self.result_code) then
    target = stream:ensure_space(target)
    target = pb.wire_format_lite.write_int32_to_array(
        1, self.result_code, target)
  end

  -- repeated .TUS.PairedStr pairs = 2[json_name = "pairs"];
  for i = 1, #self.pairs do
    target = stream:ensure_space(target)
    target = pb.wire_format_lite.
        internal_write_message(2, self.pairs[i], target, stream)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    target = stream:write_raw(self:get_internal_metadata():unknown_fields(),
        self:get_internal_metadata():unknown_fields():len(), target)
  end
  return target
end

pkg["dc_pb"].Versions.byte_size_long = function(self)
  local total_size = 0

  -- int32 result_code = 1[json_name = "resultCode"];
  if helper.check_not_zero(self.result_code) then
    total_size = total_size + 1 +
        pb.wire_format_lite.int32_size(self.result_code)
  end

  -- repeated .TUS.PairedStr pairs = 2[json_name = "pairs"];
  do
    local count = #self.pairs
    total_size = total_size + (1 * count)
    for i = 1, count do
      total_size = total_size +
          pb.wire_format_lite.message_size(self.pairs[i])
    end
  end

  if self:get_internal_metadata():have_unknown_fields() then
    total_size = total_size + self:get_internal_metadata():unknown_fields():len()
  end
  self:set_cached_size(total_size)
  return total_size
end

-- Class Definition (TUS.Response)
pkg["dc_pb"].Response = {}
setmetatable(pkg["dc_pb"].Response, {__index = message_lite})

pkg["dc_pb"].Response.new = function(message)
  local fields = {}
  fields.result_code = 0
  fields.result_text = ""
  local obj = message_lite.new("TUS.Response", fields, message)
  return setmetatable(obj, {__index = pkg["dc_pb"].Response, __eq = message_lite.__eq, __tostring = message_lite.__tostring})
end

pkg["dc_pb"].Response.internal_parse = function(self, ptr, ctx)
  local success = true
  while not ctx:done(ptr) do
    local tag
    ptr, tag = pb.internal.read_tag(ptr)
    if ptr == pb.char.nullptr then return false, true end
    local switch = {}
    -- int32 result_code = 1[json_name = "resultCode"];
    switch[1] = function()
      if (tag & 0xFF) == 8 then
        ptr, self.result_code = pb.internal.read_varint64(ptr, pb.internal.INT32)
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    -- string result_text = 2[json_name = "resultText"];
    switch[2] = function()
      if (tag & 0xFF) == 18 then
        local str
        ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
        if not pb.internal.verify_utf8(str, pb.char.nullptr) then return false, true end
        self.result_text = str
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    switch["default"] = function()
      local unknown
      ptr, unknown = pb.internal.unknown_field_parse(tag,
          self:get_internal_metadata():unknown_fields(),
          ptr, ctx)
      self:get_internal_metadata():set_unknown_fields(unknown)
      if ptr == pb.char.nullptr then return false, true end
      return true, false
    end
    local break_
    local switch_ = switch[tag >> 3]
    if switch_ == nil then
      success, break_ = switch["default"]()
    else
      success, break_ = switch_()
    end
    if break_ then break end
  end  -- while
  if success then
    return ptr
  else
    return pb.char.nullptr
  end
end

pkg["dc_pb"].Response.internal_serialize = function(self, target, stream)
  -- int32 result_code = 1[json_name = "resultCode"];
  if helper.check_not_zero(self.result_code) then
    target = stream:ensure_space(target)
    target = pb.wire_format_lite.write_int32_to_array(
        1, self.result_code, target)
  end

  -- string result_text = 2[json_name = "resultText"];
  if self.result_text:len() > 0 then
    local result = pb.wire_format_lite.verify_utf8_string(
        self.result_text, self.result_text:len(),
        pb.wire_format_lite.SERIALIZE,
        "TUS.Response.result_text")
    if result == false then
      error("Checking utf8 string failed.")
    end
    target = stream:write_string_maybe_aliased(
        2, self.result_text, target)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    target = stream:write_raw(self:get_internal_metadata():unknown_fields(),
        self:get_internal_metadata():unknown_fields():len(), target)
  end
  return target
end

pkg["dc_pb"].Response.byte_size_long = function(self)
  local total_size = 0

  -- int32 result_code = 1[json_name = "resultCode"];
  if helper.check_not_zero(self.result_code) then
    total_size = total_size + 1 +
        pb.wire_format_lite.int32_size(self.result_code)
  end

  -- string result_text = 2[json_name = "resultText"];
  if self.result_text:len() > 0 then
    total_size = total_size + 1 +
        pb.wire_format_lite.string_size(self.result_text)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    total_size = total_size + self:get_internal_metadata():unknown_fields():len()
  end
  self:set_cached_size(total_size)
  return total_size
end

-- Class Definition (TUS.Script)
pkg["dc_pb"].Script = {}
setmetatable(pkg["dc_pb"].Script, {__index = message_lite})

pkg["dc_pb"].Script.new = function(message)
  local fields = {}
  fields.parameters = ""
  fields.body = ""
  fields.session_id = ""
  local obj = message_lite.new("TUS.Script", fields, message)
  return setmetatable(obj, {__index = pkg["dc_pb"].Script, __eq = message_lite.__eq, __tostring = message_lite.__tostring})
end

pkg["dc_pb"].Script.internal_parse = function(self, ptr, ctx)
  local success = true
  while not ctx:done(ptr) do
    local tag
    ptr, tag = pb.internal.read_tag(ptr)
    if ptr == pb.char.nullptr then return false, true end
    local switch = {}
    -- string parameters = 1[json_name = "parameters"];
    switch[1] = function()
      if (tag & 0xFF) == 10 then
        local str
        ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
        if not pb.internal.verify_utf8(str, pb.char.nullptr) then return false, true end
        self.parameters = str
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    -- bytes body = 2[json_name = "body"];
    switch[2] = function()
      if (tag & 0xFF) == 18 then
        local str
        ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
        self.body = str
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    -- string session_id = 3[json_name = "sessionId"];
    switch[3] = function()
      if (tag & 0xFF) == 26 then
        local str
        ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
        if not pb.internal.verify_utf8(str, pb.char.nullptr) then return false, true end
        self.session_id = str
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    switch["default"] = function()
      local unknown
      ptr, unknown = pb.internal.unknown_field_parse(tag,
          self:get_internal_metadata():unknown_fields(),
          ptr, ctx)
      self:get_internal_metadata():set_unknown_fields(unknown)
      if ptr == pb.char.nullptr then return false, true end
      return true, false
    end
    local break_
    local switch_ = switch[tag >> 3]
    if switch_ == nil then
      success, break_ = switch["default"]()
    else
      success, break_ = switch_()
    end
    if break_ then break end
  end  -- while
  if success then
    return ptr
  else
    return pb.char.nullptr
  end
end

pkg["dc_pb"].Script.internal_serialize = function(self, target, stream)
  -- string parameters = 1[json_name = "parameters"];
  if self.parameters:len() > 0 then
    local result = pb.wire_format_lite.verify_utf8_string(
        self.parameters, self.parameters:len(),
        pb.wire_format_lite.SERIALIZE,
        "TUS.Script.parameters")
    if result == false then
      error("Checking utf8 string failed.")
    end
    target = stream:write_string_maybe_aliased(
        1, self.parameters, target)
  end

  -- bytes body = 2[json_name = "body"];
  if self.body:len() > 0 then
    target = stream:write_bytes_maybe_aliased(
        2, self.body, target)
  end

  -- string session_id = 3[json_name = "sessionId"];
  if self.session_id:len() > 0 then
    local result = pb.wire_format_lite.verify_utf8_string(
        self.session_id, self.session_id:len(),
        pb.wire_format_lite.SERIALIZE,
        "TUS.Script.session_id")
    if result == false then
      error("Checking utf8 string failed.")
    end
    target = stream:write_string_maybe_aliased(
        3, self.session_id, target)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    target = stream:write_raw(self:get_internal_metadata():unknown_fields(),
        self:get_internal_metadata():unknown_fields():len(), target)
  end
  return target
end

pkg["dc_pb"].Script.byte_size_long = function(self)
  local total_size = 0

  -- string parameters = 1[json_name = "parameters"];
  if self.parameters:len() > 0 then
    total_size = total_size + 1 +
        pb.wire_format_lite.string_size(self.parameters)
  end

  -- bytes body = 2[json_name = "body"];
  if self.body:len() > 0 then
    total_size = total_size + 1 +
        pb.wire_format_lite.bytes_size(self.body)
  end

  -- string session_id = 3[json_name = "sessionId"];
  if self.session_id:len() > 0 then
    total_size = total_size + 1 +
        pb.wire_format_lite.string_size(self.session_id)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    total_size = total_size + self:get_internal_metadata():unknown_fields():len()
  end
  self:set_cached_size(total_size)
  return total_size
end

-- Class Definition (TUS.PairedStr)
pkg["dc_pb"].PairedStr = {}
setmetatable(pkg["dc_pb"].PairedStr, {__index = message_lite})

pkg["dc_pb"].PairedStr.new = function(message)
  local fields = {}
  fields.key = ""
  fields.value = ""
  local obj = message_lite.new("TUS.PairedStr", fields, message)
  return setmetatable(obj, {__index = pkg["dc_pb"].PairedStr, __eq = message_lite.__eq, __tostring = message_lite.__tostring})
end

pkg["dc_pb"].PairedStr.internal_parse = function(self, ptr, ctx)
  local success = true
  while not ctx:done(ptr) do
    local tag
    ptr, tag = pb.internal.read_tag(ptr)
    if ptr == pb.char.nullptr then return false, true end
    local switch = {}
    -- string key = 1[json_name = "key"];
    switch[1] = function()
      if (tag & 0xFF) == 10 then
        local str
        ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
        if not pb.internal.verify_utf8(str, pb.char.nullptr) then return false, true end
        self.key = str
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    -- string value = 2[json_name = "value"];
    switch[2] = function()
      if (tag & 0xFF) == 18 then
        local str
        ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
        if not pb.internal.verify_utf8(str, pb.char.nullptr) then return false, true end
        self.value = str
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    switch["default"] = function()
      local unknown
      ptr, unknown = pb.internal.unknown_field_parse(tag,
          self:get_internal_metadata():unknown_fields(),
          ptr, ctx)
      self:get_internal_metadata():set_unknown_fields(unknown)
      if ptr == pb.char.nullptr then return false, true end
      return true, false
    end
    local break_
    local switch_ = switch[tag >> 3]
    if switch_ == nil then
      success, break_ = switch["default"]()
    else
      success, break_ = switch_()
    end
    if break_ then break end
  end  -- while
  if success then
    return ptr
  else
    return pb.char.nullptr
  end
end

pkg["dc_pb"].PairedStr.internal_serialize = function(self, target, stream)
  -- string key = 1[json_name = "key"];
  if self.key:len() > 0 then
    local result = pb.wire_format_lite.verify_utf8_string(
        self.key, self.key:len(),
        pb.wire_format_lite.SERIALIZE,
        "TUS.PairedStr.key")
    if result == false then
      error("Checking utf8 string failed.")
    end
    target = stream:write_string_maybe_aliased(
        1, self.key, target)
  end

  -- string value = 2[json_name = "value"];
  if self.value:len() > 0 then
    local result = pb.wire_format_lite.verify_utf8_string(
        self.value, self.value:len(),
        pb.wire_format_lite.SERIALIZE,
        "TUS.PairedStr.value")
    if result == false then
      error("Checking utf8 string failed.")
    end
    target = stream:write_string_maybe_aliased(
        2, self.value, target)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    target = stream:write_raw(self:get_internal_metadata():unknown_fields(),
        self:get_internal_metadata():unknown_fields():len(), target)
  end
  return target
end

pkg["dc_pb"].PairedStr.byte_size_long = function(self)
  local total_size = 0

  -- string key = 1[json_name = "key"];
  if self.key:len() > 0 then
    total_size = total_size + 1 +
        pb.wire_format_lite.string_size(self.key)
  end

  -- string value = 2[json_name = "value"];
  if self.value:len() > 0 then
    total_size = total_size + 1 +
        pb.wire_format_lite.string_size(self.value)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    total_size = total_size + self:get_internal_metadata():unknown_fields():len()
  end
  self:set_cached_size(total_size)
  return total_size
end

-- Class Definition (TUS.AsyncResult)
pkg["dc_pb"].AsyncResult = {}
setmetatable(pkg["dc_pb"].AsyncResult, {__index = message_lite})

pkg["dc_pb"].AsyncResult.new = function(message)
  local fields = {}
  fields.tag = 0
  fields.result_code = 0
  fields.result_text = ""
  local obj = message_lite.new("TUS.AsyncResult", fields, message)
  return setmetatable(obj, {__index = pkg["dc_pb"].AsyncResult, __eq = message_lite.__eq, __tostring = message_lite.__tostring})
end

pkg["dc_pb"].AsyncResult.internal_parse = function(self, ptr, ctx)
  local success = true
  while not ctx:done(ptr) do
    local tag
    ptr, tag = pb.internal.read_tag(ptr)
    if ptr == pb.char.nullptr then return false, true end
    local switch = {}
    -- uint32 tag = 1[json_name = "tag"];
    switch[1] = function()
      if (tag & 0xFF) == 8 then
        ptr, self.tag = pb.internal.read_varint32(ptr, pb.internal.UINT32)
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    -- int32 result_code = 2[json_name = "resultCode"];
    switch[2] = function()
      if (tag & 0xFF) == 16 then
        ptr, self.result_code = pb.internal.read_varint64(ptr, pb.internal.INT32)
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    -- string result_text = 3[json_name = "resultText"];
    switch[3] = function()
      if (tag & 0xFF) == 26 then
        local str
        ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
        if not pb.internal.verify_utf8(str, pb.char.nullptr) then return false, true end
        self.result_text = str
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    switch["default"] = function()
      local unknown
      ptr, unknown = pb.internal.unknown_field_parse(tag,
          self:get_internal_metadata():unknown_fields(),
          ptr, ctx)
      self:get_internal_metadata():set_unknown_fields(unknown)
      if ptr == pb.char.nullptr then return false, true end
      return true, false
    end
    local break_
    local switch_ = switch[tag >> 3]
    if switch_ == nil then
      success, break_ = switch["default"]()
    else
      success, break_ = switch_()
    end
    if break_ then break end
  end  -- while
  if success then
    return ptr
  else
    return pb.char.nullptr
  end
end

pkg["dc_pb"].AsyncResult.internal_serialize = function(self, target, stream)
  -- uint32 tag = 1[json_name = "tag"];
  if helper.check_not_zero(self.tag) then
    target = stream:ensure_space(target)
    target = pb.wire_format_lite.write_uint32_to_array(
        1, self.tag, target)
  end

  -- int32 result_code = 2[json_name = "resultCode"];
  if helper.check_not_zero(self.result_code) then
    target = stream:ensure_space(target)
    target = pb.wire_format_lite.write_int32_to_array(
        2, self.result_code, target)
  end

  -- string result_text = 3[json_name = "resultText"];
  if self.result_text:len() > 0 then
    local result = pb.wire_format_lite.verify_utf8_string(
        self.result_text, self.result_text:len(),
        pb.wire_format_lite.SERIALIZE,
        "TUS.AsyncResult.result_text")
    if result == false then
      error("Checking utf8 string failed.")
    end
    target = stream:write_string_maybe_aliased(
        3, self.result_text, target)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    target = stream:write_raw(self:get_internal_metadata():unknown_fields(),
        self:get_internal_metadata():unknown_fields():len(), target)
  end
  return target
end

pkg["dc_pb"].AsyncResult.byte_size_long = function(self)
  local total_size = 0

  -- uint32 tag = 1[json_name = "tag"];
  if helper.check_not_zero(self.tag) then
    total_size = total_size + 1 +
        pb.wire_format_lite.uint32_size(self.tag)
  end

  -- int32 result_code = 2[json_name = "resultCode"];
  if helper.check_not_zero(self.result_code) then
    total_size = total_size + 1 +
        pb.wire_format_lite.int32_size(self.result_code)
  end

  -- string result_text = 3[json_name = "resultText"];
  if self.result_text:len() > 0 then
    total_size = total_size + 1 +
        pb.wire_format_lite.string_size(self.result_text)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    total_size = total_size + self:get_internal_metadata():unknown_fields():len()
  end
  self:set_cached_size(total_size)
  return total_size
end

-- Class Definition (TUS.Session)
pkg["dc_pb"].Session = {}
setmetatable(pkg["dc_pb"].Session, {__index = message_lite})

pkg["dc_pb"].Session.new = function(message)
  local fields = {}
  fields.id = ""
  local obj = message_lite.new("TUS.Session", fields, message)
  return setmetatable(obj, {__index = pkg["dc_pb"].Session, __eq = message_lite.__eq, __tostring = message_lite.__tostring})
end

pkg["dc_pb"].Session.internal_parse = function(self, ptr, ctx)
  local success = true
  while not ctx:done(ptr) do
    local tag
    ptr, tag = pb.internal.read_tag(ptr)
    if ptr == pb.char.nullptr then return false, true end
    local switch = {}
    -- string id = 1[json_name = "id"];
    switch[1] = function()
      if (tag & 0xFF) == 10 then
        local str
        ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
        if not pb.internal.verify_utf8(str, pb.char.nullptr) then return false, true end
        self.id = str
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    switch["default"] = function()
      local unknown
      ptr, unknown = pb.internal.unknown_field_parse(tag,
          self:get_internal_metadata():unknown_fields(),
          ptr, ctx)
      self:get_internal_metadata():set_unknown_fields(unknown)
      if ptr == pb.char.nullptr then return false, true end
      return true, false
    end
    local break_
    local switch_ = switch[tag >> 3]
    if switch_ == nil then
      success, break_ = switch["default"]()
    else
      success, break_ = switch_()
    end
    if break_ then break end
  end  -- while
  if success then
    return ptr
  else
    return pb.char.nullptr
  end
end

pkg["dc_pb"].Session.internal_serialize = function(self, target, stream)
  -- string id = 1[json_name = "id"];
  if self.id:len() > 0 then
    local result = pb.wire_format_lite.verify_utf8_string(
        self.id, self.id:len(),
        pb.wire_format_lite.SERIALIZE,
        "TUS.Session.id")
    if result == false then
      error("Checking utf8 string failed.")
    end
    target = stream:write_string_maybe_aliased(
        1, self.id, target)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    target = stream:write_raw(self:get_internal_metadata():unknown_fields(),
        self:get_internal_metadata():unknown_fields():len(), target)
  end
  return target
end

pkg["dc_pb"].Session.byte_size_long = function(self)
  local total_size = 0

  -- string id = 1[json_name = "id"];
  if self.id:len() > 0 then
    total_size = total_size + 1 +
        pb.wire_format_lite.string_size(self.id)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    total_size = total_size + self:get_internal_metadata():unknown_fields():len()
  end
  self:set_cached_size(total_size)
  return total_size
end

-- Class Definition (TUS.Updates)
pkg["dc_pb"].Updates = {}
setmetatable(pkg["dc_pb"].Updates, {__index = message_lite})

pkg["dc_pb"].Updates.new = function(message)
  local fields = {}
  fields.result_code = 0
  fields.pairs = function(msg)
    return repeated_message_field.new(pkg["dc_pb"].PairedStr.new, msg)
  end
  fields.arets = function(msg)
    return repeated_message_field.new(pkg["dc_pb"].AsyncResult.new, msg)
  end
  local obj = message_lite.new("TUS.Updates", fields, message)
  return setmetatable(obj, {__index = pkg["dc_pb"].Updates, __eq = message_lite.__eq, __tostring = message_lite.__tostring})
end

pkg["dc_pb"].Updates.internal_parse = function(self, ptr, ctx)
  local success = true
  while not ctx:done(ptr) do
    local tag
    ptr, tag = pb.internal.read_tag(ptr)
    if ptr == pb.char.nullptr then return false, true end
    local switch = {}
    -- int32 result_code = 1[json_name = "resultCode"];
    switch[1] = function()
      if (tag & 0xFF) == 8 then
        ptr, self.result_code = pb.internal.read_varint64(ptr, pb.internal.INT32)
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    -- repeated .TUS.PairedStr pairs = 2[json_name = "pairs"];
    switch[2] = function()
      if (tag & 0xFF) == 18 then
        ptr = ptr - 1
        repeat
          ptr = ptr + 1
          ptr = ctx:parse_message(self.pairs:add(), ptr)
          if ptr == pb.char.nullptr then return false, true end
          if not ctx:data_available(ptr) then break end
        until not pb.internal.expect_tag(ptr, 18)
      else
        return switch["default"]()
      end
      return true, false
    end
    -- repeated .TUS.AsyncResult arets = 3[json_name = "arets"];
    switch[3] = function()
      if (tag & 0xFF) == 26 then
        ptr = ptr - 1
        repeat
          ptr = ptr + 1
          ptr = ctx:parse_message(self.arets:add(), ptr)
          if ptr == pb.char.nullptr then return false, true end
          if not ctx:data_available(ptr) then break end
        until not pb.internal.expect_tag(ptr, 26)
      else
        return switch["default"]()
      end
      return true, false
    end
    switch["default"] = function()
      local unknown
      ptr, unknown = pb.internal.unknown_field_parse(tag,
          self:get_internal_metadata():unknown_fields(),
          ptr, ctx)
      self:get_internal_metadata():set_unknown_fields(unknown)
      if ptr == pb.char.nullptr then return false, true end
      return true, false
    end
    local break_
    local switch_ = switch[tag >> 3]
    if switch_ == nil then
      success, break_ = switch["default"]()
    else
      success, break_ = switch_()
    end
    if break_ then break end
  end  -- while
  if success then
    return ptr
  else
    return pb.char.nullptr
  end
end

pkg["dc_pb"].Updates.internal_serialize = function(self, target, stream)
  -- int32 result_code = 1[json_name = "resultCode"];
  if helper.check_not_zero(self.result_code) then
    target = stream:ensure_space(target)
    target = pb.wire_format_lite.write_int32_to_array(
        1, self.result_code, target)
  end

  -- repeated .TUS.PairedStr pairs = 2[json_name = "pairs"];
  for i = 1, #self.pairs do
    target = stream:ensure_space(target)
    target = pb.wire_format_lite.
        internal_write_message(2, self.pairs[i], target, stream)
  end

  -- repeated .TUS.AsyncResult arets = 3[json_name = "arets"];
  for i = 1, #self.arets do
    target = stream:ensure_space(target)
    target = pb.wire_format_lite.
        internal_write_message(3, self.arets[i], target, stream)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    target = stream:write_raw(self:get_internal_metadata():unknown_fields(),
        self:get_internal_metadata():unknown_fields():len(), target)
  end
  return target
end

pkg["dc_pb"].Updates.byte_size_long = function(self)
  local total_size = 0

  -- int32 result_code = 1[json_name = "resultCode"];
  if helper.check_not_zero(self.result_code) then
    total_size = total_size + 1 +
        pb.wire_format_lite.int32_size(self.result_code)
  end

  -- repeated .TUS.PairedStr pairs = 2[json_name = "pairs"];
  do
    local count = #self.pairs
    total_size = total_size + (1 * count)
    for i = 1, count do
      total_size = total_size +
          pb.wire_format_lite.message_size(self.pairs[i])
    end
  end

  -- repeated .TUS.AsyncResult arets = 3[json_name = "arets"];
  do
    local count = #self.arets
    total_size = total_size + (1 * count)
    for i = 1, count do
      total_size = total_size +
          pb.wire_format_lite.message_size(self.arets[i])
    end
  end

  if self:get_internal_metadata():have_unknown_fields() then
    total_size = total_size + self:get_internal_metadata():unknown_fields():len()
  end
  self:set_cached_size(total_size)
  return total_size
end

-- Class Definition (TUS.ExecuteAsyncParams)
pkg["dc_pb"].ExecuteAsyncParams = {}
setmetatable(pkg["dc_pb"].ExecuteAsyncParams, {__index = message_lite})

pkg["dc_pb"].ExecuteAsyncParams.new = function(message)
  local fields = {}
  fields.url = ""
  fields.tag = 0
  fields.session_id = ""
  local obj = message_lite.new("TUS.ExecuteAsyncParams", fields, message)
  return setmetatable(obj, {__index = pkg["dc_pb"].ExecuteAsyncParams, __eq = message_lite.__eq, __tostring = message_lite.__tostring})
end

pkg["dc_pb"].ExecuteAsyncParams.internal_parse = function(self, ptr, ctx)
  local success = true
  while not ctx:done(ptr) do
    local tag
    ptr, tag = pb.internal.read_tag(ptr)
    if ptr == pb.char.nullptr then return false, true end
    local switch = {}
    -- string url = 1[json_name = "url"];
    switch[1] = function()
      if (tag & 0xFF) == 10 then
        local str
        ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
        if not pb.internal.verify_utf8(str, pb.char.nullptr) then return false, true end
        self.url = str
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    -- uint32 tag = 2[json_name = "tag"];
    switch[2] = function()
      if (tag & 0xFF) == 16 then
        ptr, self.tag = pb.internal.read_varint32(ptr, pb.internal.UINT32)
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    -- string session_id = 3[json_name = "sessionId"];
    switch[3] = function()
      if (tag & 0xFF) == 26 then
        local str
        ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
        if not pb.internal.verify_utf8(str, pb.char.nullptr) then return false, true end
        self.session_id = str
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    switch["default"] = function()
      local unknown
      ptr, unknown = pb.internal.unknown_field_parse(tag,
          self:get_internal_metadata():unknown_fields(),
          ptr, ctx)
      self:get_internal_metadata():set_unknown_fields(unknown)
      if ptr == pb.char.nullptr then return false, true end
      return true, false
    end
    local break_
    local switch_ = switch[tag >> 3]
    if switch_ == nil then
      success, break_ = switch["default"]()
    else
      success, break_ = switch_()
    end
    if break_ then break end
  end  -- while
  if success then
    return ptr
  else
    return pb.char.nullptr
  end
end

pkg["dc_pb"].ExecuteAsyncParams.internal_serialize = function(self, target, stream)
  -- string url = 1[json_name = "url"];
  if self.url:len() > 0 then
    local result = pb.wire_format_lite.verify_utf8_string(
        self.url, self.url:len(),
        pb.wire_format_lite.SERIALIZE,
        "TUS.ExecuteAsyncParams.url")
    if result == false then
      error("Checking utf8 string failed.")
    end
    target = stream:write_string_maybe_aliased(
        1, self.url, target)
  end

  -- uint32 tag = 2[json_name = "tag"];
  if helper.check_not_zero(self.tag) then
    target = stream:ensure_space(target)
    target = pb.wire_format_lite.write_uint32_to_array(
        2, self.tag, target)
  end

  -- string session_id = 3[json_name = "sessionId"];
  if self.session_id:len() > 0 then
    local result = pb.wire_format_lite.verify_utf8_string(
        self.session_id, self.session_id:len(),
        pb.wire_format_lite.SERIALIZE,
        "TUS.ExecuteAsyncParams.session_id")
    if result == false then
      error("Checking utf8 string failed.")
    end
    target = stream:write_string_maybe_aliased(
        3, self.session_id, target)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    target = stream:write_raw(self:get_internal_metadata():unknown_fields(),
        self:get_internal_metadata():unknown_fields():len(), target)
  end
  return target
end

pkg["dc_pb"].ExecuteAsyncParams.byte_size_long = function(self)
  local total_size = 0

  -- string url = 1[json_name = "url"];
  if self.url:len() > 0 then
    total_size = total_size + 1 +
        pb.wire_format_lite.string_size(self.url)
  end

  -- uint32 tag = 2[json_name = "tag"];
  if helper.check_not_zero(self.tag) then
    total_size = total_size + 1 +
        pb.wire_format_lite.uint32_size(self.tag)
  end

  -- string session_id = 3[json_name = "sessionId"];
  if self.session_id:len() > 0 then
    total_size = total_size + 1 +
        pb.wire_format_lite.string_size(self.session_id)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    total_size = total_size + self:get_internal_metadata():unknown_fields():len()
  end
  self:set_cached_size(total_size)
  return total_size
end

-- Class Definition (TUS.SetEnvironmentParams)
pkg["dc_pb"].SetEnvironmentParams = {}
setmetatable(pkg["dc_pb"].SetEnvironmentParams, {__index = message_lite})

pkg["dc_pb"].SetEnvironmentParams.new = function(message)
  local fields = {}
  fields.session = pkg["dc_pb"].Session.new
  fields.key = ""
  fields.value = ""
  local obj = message_lite.new("TUS.SetEnvironmentParams", fields, message)
  return setmetatable(obj, {__index = pkg["dc_pb"].SetEnvironmentParams, __eq = message_lite.__eq, __tostring = message_lite.__tostring})
end

pkg["dc_pb"].SetEnvironmentParams.internal_parse = function(self, ptr, ctx)
  local success = true
  while not ctx:done(ptr) do
    local tag
    ptr, tag = pb.internal.read_tag(ptr)
    if ptr == pb.char.nullptr then return false, true end
    local switch = {}
    -- .TUS.Session session = 1[json_name = "session"];
    switch[1] = function()
      if (tag & 0xFF) == 10 then
        ptr = ctx:parse_message(self.session, ptr)
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    -- string key = 2[json_name = "key"];
    switch[2] = function()
      if (tag & 0xFF) == 18 then
        local str
        ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
        if not pb.internal.verify_utf8(str, pb.char.nullptr) then return false, true end
        self.key = str
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    -- string value = 3[json_name = "value"];
    switch[3] = function()
      if (tag & 0xFF) == 26 then
        local str
        ptr, str = pb.internal.inline_greedy_string_parser(ptr, ctx)
        if not pb.internal.verify_utf8(str, pb.char.nullptr) then return false, true end
        self.value = str
        if ptr == pb.char.nullptr then return false, true end
      else
        return switch["default"]()
      end
      return true, false
    end
    switch["default"] = function()
      local unknown
      ptr, unknown = pb.internal.unknown_field_parse(tag,
          self:get_internal_metadata():unknown_fields(),
          ptr, ctx)
      self:get_internal_metadata():set_unknown_fields(unknown)
      if ptr == pb.char.nullptr then return false, true end
      return true, false
    end
    local break_
    local switch_ = switch[tag >> 3]
    if switch_ == nil then
      success, break_ = switch["default"]()
    else
      success, break_ = switch_()
    end
    if break_ then break end
  end  -- while
  if success then
    return ptr
  else
    return pb.char.nullptr
  end
end

pkg["dc_pb"].SetEnvironmentParams.internal_serialize = function(self, target, stream)
  -- .TUS.Session session = 1[json_name = "session"];
  if self.session:__check_not_default() then
    target = stream:ensure_space(target)
    target = pb.wire_format_lite.
        internal_write_message(1, self.session, target, stream)
  end

  -- string key = 2[json_name = "key"];
  if self.key:len() > 0 then
    local result = pb.wire_format_lite.verify_utf8_string(
        self.key, self.key:len(),
        pb.wire_format_lite.SERIALIZE,
        "TUS.SetEnvironmentParams.key")
    if result == false then
      error("Checking utf8 string failed.")
    end
    target = stream:write_string_maybe_aliased(
        2, self.key, target)
  end

  -- string value = 3[json_name = "value"];
  if self.value:len() > 0 then
    local result = pb.wire_format_lite.verify_utf8_string(
        self.value, self.value:len(),
        pb.wire_format_lite.SERIALIZE,
        "TUS.SetEnvironmentParams.value")
    if result == false then
      error("Checking utf8 string failed.")
    end
    target = stream:write_string_maybe_aliased(
        3, self.value, target)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    target = stream:write_raw(self:get_internal_metadata():unknown_fields(),
        self:get_internal_metadata():unknown_fields():len(), target)
  end
  return target
end

pkg["dc_pb"].SetEnvironmentParams.byte_size_long = function(self)
  local total_size = 0

  -- .TUS.Session session = 1[json_name = "session"];
  if self.session:__check_not_default() then
    total_size = total_size + 1 +
        pb.wire_format_lite.message_size(self.session)
  end

  -- string key = 2[json_name = "key"];
  if self.key:len() > 0 then
    total_size = total_size + 1 +
        pb.wire_format_lite.string_size(self.key)
  end

  -- string value = 3[json_name = "value"];
  if self.value:len() > 0 then
    total_size = total_size + 1 +
        pb.wire_format_lite.string_size(self.value)
  end

  if self:get_internal_metadata():have_unknown_fields() then
    total_size = total_size + self:get_internal_metadata():unknown_fields():len()
  end
  self:set_cached_size(total_size)
  return total_size
end

return pkg["dc_pb"]
