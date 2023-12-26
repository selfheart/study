--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

--[[
/* TMC CONFIDENTIAL
 * $JITDFLibId$
 * Copyright (C) 2021 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
]]
--[[
This module provides message lite utility which a user indirectly uses.
<br>
Though a user uses functions in this module indirectly, this module is internal API.
To know the protobuf Lua library usage, please see protoc Lua plugin.
@module tmc_protobuf_message_lite
@copyright (C) 2021 TOYOTA MOTOR CORPORATION, All Rights Reserved.
]]
local pb = require("tmc_protobuf")
local helper = require("tmc_protobuf_helper")

local message_lite = {}
setmetatable(message_lite, {__index = pb.message_lite})

message_lite.__index = message_lite

--[[
create message object.
@tparam string type_name type name of the message object.
@tparam table fields field table where the key is the name and the value is the default value or initialization function
@tparam table message initial values for the message object
@treturn table the message object
@usage local message = message_lite.new("User", {"name":"", "id":0}, {"name":"foo", "id":10})
]]
function message_lite.new(type_name, fields, message)
  assert(type(type_name) == "string", "type_name should be string.")
  assert(type(fields) == "table", "fields should be table.")
  assert(type(message) == "nil" or type(message) == "table", "message should be nil or table.")

  local obj = pb.message_lite.new(type_name)
  obj._fields_ = fields
  obj._cached_byte_size_ = {}
  message = message or {}
  for k, v in pairs(fields) do
    if type(v) == "function" then
      obj[k] = v(message[k])
      setmetatable(obj[k], getmetatable(obj[k]))
    else
      obj[k] = message[k] or v
    end
  end
  return setmetatable(obj, message_lite)
end

function message_lite:__delete()
  pb.message_lite.delete(self)
  for k, v in pairs(self._fields_) do
    if type(v) == "function" then
      self[k]:__delete()
    end
    self[k] = nil
  end
  self._fields_ = nil
  self._cached_byte_size_ = nil
end
message_lite.__gc = message_lite.__delete

--[[
clear message object.
@usage message:clear()
]]
function message_lite:clear()
  for k, v in pairs(self._fields_) do
    if type(v) == "function" then
      self[k]:__clear()
    else
      self[k] = v
    end
  end
  self:get_internal_metadata():clear()
end
message_lite.__clear = message_lite.clear

function message_lite:__check_not_default()
  for k, v in pairs(self._fields_) do
    if type(v) == "function" then
      if self[k]:__check_not_default() == true then
        return true
      end
    else
      if self[k] ~= v then
        return true
      end
    end
  end
  return false
end

--[[
merge other message object.
@tparam table message message object
@usage message:merge_from(other_message)
]]
function message_lite:merge_from(message)
  for k, v in pairs(self._fields_) do
    if type(v) == "functions" then
      self[k].merge_from(message[k])
    else
      self[k] = message[k]
    end
  end
end

--[[
copy other message object.
@tparam table message message object
@usage message:copy_from(other_message)
]]
function message_lite:copy_from(message)
  self:clear()
  self:merge_from(message)
end

function message_lite.__eq(a, b)
  assert(type(a) == "table", "a should be table.")
  assert(type(b) == "table", "b should be table.")
  assert(type(a._fields_) == "table", "a._fields_ should be table.")
  assert(type(b._fields_) == "table", "b._fields_ should be table.")

  if #a._fields_ ~= #b._fields_ then
    return false
  end
  for k, v in pairs(b._fields_) do
    if a[k] ~= b[k] then
      return false
    end
  end
  return true
end

function message_lite:__tostring()
  local str = "{\n"
  helper.indent()
  for k, v in pairs(self._fields_) do
    if type(self[k]) == "string" then
      str = str .. helper.format(k .. " = '" .. self[k] .. "',\n")
    else
      str = str .. helper.format(k .. " = " .. tostring(self[k]) .. ",\n")
    end
  end
  helper.outdent()
  str = str .. helper.format("}")
  return str
end

return message_lite
