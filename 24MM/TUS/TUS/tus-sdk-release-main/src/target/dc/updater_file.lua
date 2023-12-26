--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

local function fs_write(path, data, offset)
  local f, errobj = io.open(path, "r+")
  if not f then
    f, errobj = io.open(path, "w")
    if not f then return nil, errobj end
  end

  if offset and offset ~= 0 then
    f:seek ("set", offset)
  end

  local tag = dc.async.generate_async_tag_from_lua()
  dc.log("created async tag", tag)
  if tag == 0 then
    return nil, -1 -- shall not happen
  end

  -- ToDo: move actual operation to an async worker
  local _, errobj = f:write(data)
  if errobj then
    local code = -1  -- fixme: convert from errobj
    errobj = dc.async:set_async_results_from_lua(tag, code)
  else
    errobj = dc.async:set_async_results_from_lua(tag, 0)
  end
  f:close()
  if errobj then
    dc.log("faied to set async result", errobj, "code", code)
    return nil, errobj
  end
  return tag
end

local function fs_write_sync(path, data, offset)
  dc.log(path, data, offset)
  local f, errobj = io.open(path, "r+")
  if not f then
    f, errobj = io.open(path, "w")
    if not f then return nil, errobj end
  end

  if offset and offset ~= 0 then
    f:seek ("set", offset)
  end

  -- ToDo: move actual operation to an async worker
  local _, errobj = f:write(data)
  f:close()
  if errobj then
    return errobj
  end
  return
end


local function fs_read(path, size, offset)
  dc.log(path, size, offset)
  local f, errobj = io.open(path, "r")
  if not f then
    return nil, errobj
  end

  if offset and offset ~= 0 then
    f:seek ("set", offset)
  end

  local tag = dc.async.generate_async_tag_from_lua()
  dc.log("created async tag", tag)
  if tag == 0 then
    return nil, -1 -- shall not happen
  end

  -- ToDo: move actual operation to an async worker
  local data, errobj
  if size == nil then
    data, errobj = f:read("*all")
  else
    data, errobj = f:read(size)
  end
  f:close()
  if errobj then
    local code = -1  -- fixme: convert from errobj
    errobj = dc.async:set_async_results_from_lua(tag, code)
  else
    errobj = dc.async:set_async_results_from_lua(tag, 0, data)
  end
  if errobj then
    dc.log("faied to set async result", errobj, "code", code)
    return nil, errobj
  end
  return tag
end

local function fs_read_sync(path, size, offset)
  dc.log(path, size, offset)
  -- blocking read (for convenience)
  local f, errobj = io.open(path, "r")
  dc.log(f, errobj)
  if not f then
    return nil, errobj
  end

  if offset and offset ~= 0 then
    f:seek ("set", offset)
  end

  local data, errobj
  if size == nil then
    data, errobj = f:read("*all")
    dc.log("all", data)
  else
    data, errobj = f:read(size)
    dc.log(size, data)
  end
  f:close()
  if errobj then
    return nil, errobj
  end
  return data
end

local function fs_rename(old, new)
  dc.log(old, new)
  local ret = os.execute("mv " .. old .. " " .. new)
  if ret then
    return
  end
  dc.log(ret)
  return -1 -- todo: convert from exit code?
end

local function fs_unlink(path)
  dc.log(path)
  local ret = os.execute("rm -f " .. path)
  if ret then
    return
  end
  dc.log(ret)
  return -1 -- todo: convert from exit code?
end

return {
  dc = nil, -- to be set be calling _init()
  _comment_ = "local filesystem",
  _init = function (self, dc) -- called from dc
    self.dc = dc
    print("init:", self._comment_)
  end,

  -- updater features
  filesystem = {
    read = fs_read,
    read_sync = fs_read_sync,
    write = fs_write,
    write_sync = fs_write_sync,
    rename = fs_rename,
    unlink = fs_unlink,
  }
}

