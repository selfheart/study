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
local helper = {}

local indent = 0

function helper.indent()
  indent = indent + 1
end

function helper.outdent()
  indent = indent - 1
end

function helper.format(fmt)
  local space = ""
  for i = 1, indent do
    space = space .. "  "
  end
  local str = space .. fmt
  return str
end

function helper.check_not_zero(val)
  if type(val) == "boolean" then
    return val
  elseif type(val) == "number" then
    return (val ~= 0)
  elseif type(val) == "string" then
    local v = tonumber(val)
    if v == nil then
      error("Invalid type. '" .. val .. "' is " .. type(val) .. ".", 2)
    else
      return (v ~= 0)
    end
  else
    error("Invalid type. '" .. val .. "' is " .. type(val) .. ".", 2)
  end
end

function helper.copy_table(src)
  local dest = {}
  for key, value in pairs(src) do
    dest[key] = value
  end
  return dest
end

return helper
