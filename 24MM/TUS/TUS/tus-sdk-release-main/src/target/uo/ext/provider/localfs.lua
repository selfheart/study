--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]


local function make_logger(prefix, level)
  return function (...)
    local s = ""
    for i = 1, select('#', ...) do
      s = s .. "\t" .. tostring(select(i, ...))
    end
    local di = nil
    pcall(function () di = debug.getinfo(level + 2) end)
    if di and di.name then
      print(prefix .. di.source .. ":" .. di.currentline .. " " .. di.name .. "() " .. s)
    else
      print(prefix .. s)
    end
  end
end

-- will say " (...): @./FILE:LINE FUNC() ..."
local log = make_logger(" (provider.localfs): ", 2)


local provider = {
  id = 'localfs',
}

local function check_open(path)
  local f = io.open(path, 'r')
  log(path, f)
  if f then
    return true
  else
    return false
  end
end

-- note: to use localfs, a TUP file shall be put at self.package_path,
--  which shall be defined in localfs's config file
--  (like 'package_path=/tmp/provider/reqscript.tup').

function provider:check_update(uo)
  local path = self.package_path
  if not path then
    -- already got 'COMPETED'
    return {}
  end
  if check_open(path) then
    log("found update package:", path)
    return {path} -- 'updates'
  else
    log("no update package:", path)
    return {}
  end
end

function provider:download_package(update, progress, options)
  -- use the file as-is, no need to actually download
  local path = update -- check_update() returns {path} as updates
  if path ~= self.package_path then
    return nil, -1 -- FIXME: assign an error code for 'mismatch'
  end
  if check_open(path) then
    log("found update package", path)
    return {    -- 'results'
      path = path, -- 'downloaded as a file'
    }
  else
    return nil, -1 -- FIXME: assign an error code for 'gone'
  end
end


function provider:send_notification(event_type, parameters)
  log(event_type, parameters)
  log("provider:send_notification(" .. event_type .. ")")
  if event_type == "COMPLETED" then
    -- next check_update() will return nothing
    os.execute("rm -f " .. self.package_path)
    self.package_path = nil
  end
end

function provider:initialize(params)
  assert(params.type == self.id)
  self.package_path = params.package_path
  log(params.type, "will use", self.package_path)
end

return provider

