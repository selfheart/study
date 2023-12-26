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
This module provides oneof utility which a user indirectly uses.
<br>
Though a user uses functions in this module indirectly, this module is internal API.
To know the protobuf Lua library usage, please see protoc Lua plugin.
@module tmc_protobuf_oneof_field
@copyright (C) 2021 TOYOTA MOTOR CORPORATION, All Rights Reserved.
]]
local helper = require("tmc_protobuf_helper")

local oneof_field = {}

oneof_field.__index = oneof_field

--[[
create oneof object.
@tparam table fields field table where the key is the name and the value is the default value or initialization function
@tparam table oneof initial values for the oneof object
@treturn table the oneof object
@usage local oneof = oneof_field.new({"name":"", "id":0}, {"name":"foo"})
]]
function oneof_field.new(fields, oneof)
  local obj = {}
  obj._fields_ = fields
  obj._oneof_case_ = nil
  oneof = oneof or {}
  for k, v in pairs(oneof) do
    if fields[k] ~= nil then
      obj._oneof_case_ = k
      if type(fields[k]) == "function" then
        obj[k] = v(oneof[k])
        setmetatable(obj[k], getmetatable(obj[k]))
      else
        obj[k] = v
      end
      break
    end
  end
  return setmetatable(obj, oneof_field)
end

--[[
return if the oneof object has the field.
@tparam string field_name the field name
@treturn boolean boolean value of whether to have the field
@usage oneof:has_field("id")
]]
function oneof_field:has_field(field_name)
  return self._oneof_case_ == field_name
end

--[[
return the field that the oneof object has
@treturn string the field name that the object has
@usage local oneof_case = oneof:which_oneof()
]]
function oneof_field:which_oneof()
  return self._oneof_case_
end

--[[
clear values stored in oneof object.
@usage oneof:clear()
]]
function oneof_field:clear()
  if self._oneof_case_ == nil then
    return
  end
  if type(self[self._oneof_case_]) == "function" then
    self[self._oneof_case_]:__delete()
  end
  self[self._oneof_case_] = nil
  self._oneof_case_ = nil
end
oneof_field.__delete = oneof_field.clear
oneof_field.__clear = oneof_field.clear

--[[
merge other oneof object.
@tparam table oneof oneof object
@usage oneof:merge_from(other_oneof)
]]
function oneof_field:merge_from(oneof)
  if type(self[self._oneof_case_]) == "function" then
    self[self._oneof_case_].merge_from(oneof[self._oneof_case_])
  else
    self[self._oneof_case_] = oneof[self._oneof_case_]
  end
end

--[[
copy other oneof object.
@tparam table oneof oneof object
@usage oneof:copy_from(other_oneof)
]]
function oneof_field:copy_from(oneof)
  self:clear()
  self:merge_from(oneof)
end

function oneof_field:__newindex(index, value)
  if index == "_oneof_case_" then
    rawset(self, index, value)
    return
  end
  if self._fields_[index] ~= nil then
    rawset(self, index, value)
    if self._oneof_case_ ~= nil then
      if type(self[self._oneof_case_]) == "function" then
        self[self._oneof_case_]:__delete()
      end
      self[self._oneof_case_] = nil
    end
    self._oneof_case_ = index
  end
end

function oneof_field:__check_not_default()
  return self._oneof_case_ ~= nil
end

function oneof_field.__eq(a, b)
  if a.__oneof_case_ ~= b.__oneof_case_ then
    return false
  end
  return a[a.__oneof_case_] == b[b.__oneof_case_]
end

function oneof_field:__tostring()
  local str = "{\n"
  helper.indent()
  if self:which_oneof() == nil then
    str = str .. helper.format("which_oneof() = nil,\n")
  else
    str = str .. helper.format("which_oneof() = " .. self:which_oneof() .. ",\n")
  end
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

return oneof_field
