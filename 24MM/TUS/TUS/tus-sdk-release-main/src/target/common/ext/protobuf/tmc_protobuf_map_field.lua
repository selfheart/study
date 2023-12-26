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
This module provides map utility which a user indirectly uses.
<br>
Though a user uses functions in this module indirectly, this module is internal API.
To know the protobuf Lua library usage, please see protoc Lua plugin.
@module tmc_protobuf_map_field
@copyright (C) 2021 TOYOTA MOTOR CORPORATION, All Rights Reserved.
]]
local repeated_message_field = require("tmc_protobuf_repeated_message_field")
local helper = require("tmc_protobuf_helper")

local map_field = {}

map_field.__index = map_field

--[[
create map object.
@tparam function new message constructor of the map object
@tparam table map initial values for the map object
@treturn table the map object
@usage local map = map_field.new(package.MapEntry.new, {"foo":1, "bar":2})
]]
function map_field.new(new, map)
  local obj = {}
  obj._map_ = repeated_message_field.new(new)
  map = map or {}
  for k, v in pairs(map) do
    obj._map_:add({key = k, value = v})
  end
  return setmetatable(obj, map_field)
end

--[[
clear values stored in map object.
@usage map:clear()
]]
function map_field:clear()
  self._map_:__clear()
end
map_field.__clear = map_field.clear

function map_field:__delete()
  self._map_:__delete()
end

--[[
iterator of keys and values.
@usage
for k, v in map:items() do
  print(k, v)
end
]]
function map_field:items()
  local i = 0
  return function()
    i = i + 1
    local map = self._map_[i]
    if map then
      return map.key, map.value
    end
  end
end

--[[
iterator of keys.
@usage
for k in map:keys() do
  print(k)
end
]]
function map_field:keys()
  local i = 0
  return function()
    i = i + 1
    local map = self._map_[i]
    if map then
      return map.key
    end
  end
end

--[[
iterator of values.
@usage
for k in map:values() do
  print(v)
end
]]
function map_field:values()
  local i = 0
  return function()
    i = i + 1
    local map = self._map_[i]
    if map then
      return map.value
    end
  end
end

--[[
update values of map object.
@tparam table map values to update.
@usage map:update({"foo":5, "bar":6})
]]
function map_field:update(map)
  for k, v in pairs(map) do
    self[k] = v
  end
end

--[[
merge other map object.
@tparam table map map object
@usage map:merge_from(other_map)
]]
function map_field:merge_from(map)
  for k, v in map.items() do
    self[k] = v
  end
end

--[[
copy other map object.
@tparam table map map object
@usage map:copy_from(other_map)
]]
function map_field:copy_from(map)
  self:clear()
  self:merge_from()
end

local func = {
  new = map_field.new,
  clear = map_field.clear,
  __clear = map_field.__clear,
  __delete = map_field.__delete,
  items = map_field.items,
  keys = map_field.keys,
  values = map_field.values,
  update = map_field.update,
}

function map_field:__index(index)
  if index == "_map_" then
    return rawget(self, "_map_")
  end
  if func[index] ~= nil then
    return func[index]
  end
  local map = rawget(self, "_map_")
  for i, v in ipairs(map) do
    if v.key == index then
      return v.value
    end
  end
  return nil
end

function map_field:__newindex(index, value)
  for i, v in ipairs(self._map_) do
    if v.key == index then
      if type(v) == "table" then
        v.value:copy_from(value)
      else
        v.value = value
      end
      return
    end
  end
  if type(value) == "table" then
    self._map_:add({key = index}).value:merge_from(value)
  else
    self._map_:add({key = index, value = value})
  end
end

function map_field.__eq(a, b)
  return a._map_ == b._map_
end

function map_field:__tostring()
  local str = "{\n"
  helper.indent()
  for i, v in ipairs(self._map_) do
    str = str .. helper.format(tostring(v.key) .. " = " .. tostring(v.value) .. ",\n")
  end
  helper.outdent()
  str = str .. helper.format("}")
  return str
end

return map_field
