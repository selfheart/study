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
This module provides repeated message field utility which a user indirectly uses.
<br>
Though a user uses functions in this module indirectly, this module is internal API.
To know the protobuf Lua library usage, please see protoc Lua plugin.
@module tmc_protobuf_repeated_message_field
@copyright (C) 2021 TOYOTA MOTOR CORPORATION, All Rights Reserved.
]]
local helper = require("tmc_protobuf_helper")

local repeated_message_field = {}

--[[
create repeated message object.
@tparam function new the message object constructor
@tparam table repeated_message initial values for the value object
@treturn table the repeated message object
@usage local repeated_message = repeated_message_field.new(package.User.new, {{"name":"foo", id:10}, {"name":"bar", id:15}})
]]
function repeated_message_field.new(new, repeated_message)
  local obj = {}
  repeated_message_field.new_msg = new
  local copied_repeated_message_field = helper.copy_table(repeated_message_field)
  copied_repeated_message_field.__index = copied_repeated_message_field
  setmetatable(obj, copied_repeated_message_field)
  repeated_message = repeated_message or {}
  for i = 1, #repeated_message do
    obj:add(repeated_message[i])
  end
  return obj
end

--[[
add message object to repeated message object.
@tparam table message initial values for the oneof object
@treturn table new message object
@usage local new_message = repeated_message:add({"name":"baz", id:20})
]]
function repeated_message_field:add(message)
  local message_field = self.new_msg(message)
  table.insert(self, message_field)
  return message_field
end

--[[
append message object to repeated message object.
@tparam table field message object
@usage repeated_message:append(message)
]]
function repeated_message_field:append(field)
  table.insert(self, field)
end

--[[
insert message object to repeated message object.
@tparam num index the position to insert
@tparam table field message object
@usage repeated_message:insert(2, message)
]]
function repeated_message_field:insert(index, field)
  table.insert(self, index, field)
end

--[[
extend repeated message object.
@tparam table repeated_field repeated message object
@usage repeated_message:insert(additional_repeated_message)
]]
function repeated_message_field:extend(repeated_field)
  for i = 1, #repeated_field do
    table.insert(self, repeated_field[i])
  end
end

--[[
clear values stored in repeated message object.
@usage repeated_message:clear()
]]
function repeated_message_field:clear()
  for i = 1, #self do
    self[i]:__delete()
    self[i] = nil
  end
end
repeated_message_field.__delete = repeated_message_field.clear
repeated_message_field.__clear = repeated_message_field.clear

--[[
merge other repeated_message object.
@tparam table repeated_message repeated_message object
@usage repeated_message:merge_from(other_repeated_message)
]]
function repeated_message_field:merge_from(repeated_message)
  for i = 1, #repeated_message do
    self:add():merge_from(repeated_message[i])
  end
end

--[[
copy other repeated_message object.
@tparam table repeated_message repeated_message object
@usage repeated_message:copy_from(other_repeated_message)
]]
function repeated_message_field:copy_from(repeated_message)
  self:clear()
  self:merge_from(repeated_message)
end

function repeated_message_field:__check_not_default()
  return #self ~= 0
end

function repeated_message_field.__eq(a, b)
  if #a ~= #b then
    return false
  end
  for i = 1, #a do
    if a[i] ~= b[i] then
      return false
    end
  end
  return true
end

function repeated_message_field:__tostring()
  local str = "{\n"
  helper.indent()
  for i = 1, #self do
    str = str .. helper.format(tostring(self[i]) .. ",\n")
  end
  helper.outdent()
  str = str .. helper.format("}")
  return str
end

return repeated_message_field
