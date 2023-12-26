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
This module provides repeated value field utility which a user indirectly uses.
<br>
Though a user uses functions in this module indirectly, this module is internal API.
To know the protobuf Lua library usage, please see protoc Lua plugin.
@module tmc_protobuf_repeated_value_field
@copyright (C) 2021 TOYOTA MOTOR CORPORATION, All Rights Reserved.
]]
local helper = require("tmc_protobuf_helper")

local repeated_value_field = {}

repeated_value_field.__index = repeated_value_field

--[[
create repeated value object.
@tparam table repeated_value initial values for the repeated value object
@treturn table the repeated value object
@usage local repeated_value = repeated_value_field.new({"one", "two", "three"})
]]
function repeated_value_field.new(repeated_value)
  local obj = {}
  setmetatable(obj, repeated_value_field)
  repeated_value = repeated_value or {}
  obj:extend(repeated_value)
  return obj
end

--[[
append value to repeated value object.
@tparam table field value
@usage repeated_value:append(value)
]]
function repeated_value_field:append(field)
  table.insert(self, field)
end

--[[
insert value to repeated value object.
@tparam num index the position to insert
@tparam table field value
@usage repeated_value:insert(2, value)
]]
function repeated_value_field:insert(index, field)
  table.insert(self, index, field)
end

--[[
extend repeated value object.
@tparam table repeated_field repeated value object
@usage repeated_value:insert(additional_repeated_value)
]]
function repeated_value_field:extend(repeated_field)
  for i = 1, #repeated_field do
    table.insert(self, repeated_field[i])
  end
end
repeated_value_field.merge_from = repeated_value_field.extend

--[[
clear values stored in repeated value object.
@usage repeated_value:clear()
]]
function repeated_value_field:clear()
  for i = 1, #self do
    self[i] = nil
  end
end
repeated_value_field.__delete = repeated_value_field.clear
repeated_value_field.__clear = repeated_value_field.clear

function repeated_value_field:__check_not_default()
  return #self ~= 0
end

--[[
copy other repeated_value object.
@tparam table repeated_field repeated_value object
@usage repeated_value:copy_from(other_repeated_value)
]]
function repeated_value_field:copy_from(repeated_field)
  self:clear()
  self:merge_from(repeated_field)
end

function repeated_value_field.__eq(a, b)
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

function repeated_value_field:__tostring()
  local str = "{"
  for i = 1, #self do
    if type(self[i]) == "string" then
      str = str .. "'" .. self[i] .. "'"
    else
      str = str .. tostring(self[i])
    end
    if i ~= #self then
      str = str .. ", "
    end
  end
  str = str .. "}"
  return str
end

return repeated_value_field
