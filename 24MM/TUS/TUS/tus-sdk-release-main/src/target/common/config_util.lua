--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

-- utilities for TUS configuration file 

local config_util = {
  -- may keep 'base directory' here
}

function config_util:concat_path(parent, path)
  if string.sub(path, 1, 1) == "/" then
    return path
  else
    return parent .. "/" .. path
  end
end

function config_util:parse_config_file(base, rel)
  local path
  if rel then
    path = config_util:concat_path(base, rel)
  else
    path = base -- use 'path' as-is
  end
  local f = io.open(path, 'r')
  if not f then
    error("failed to open: " .. path)
    return nil -- note: {} is truthy in Lua
  end
  local t = {}
  local line = f:read()
  while line do
    local pre, post = string.match(line, '([^#]*)(#.*)')
    line = string.gsub(line, '^%s+', '')
    if post then line = pre end
    line = string.gsub(line, '%s+$', '')
    local k, v = string.match(line, '([^\t =]+)[ \t]*=[ \t]*(.*)')
    -- log(k, v, line)
    if line > '' then
      t[k] = v -- ToDo: convert to an array if k was already set?
    end
    line = f:read()
  end
  f:close()
  return t
end

return config_util


