--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]



local function deviceimage_write(path, data, offset)
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

local function deviceimage_write_sync(path, data, offset)
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


local function deviceimage_read(path, size, offset)
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

local function deviceimage_read_sync(path, size, offset)
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
    -- dc.log("all", data)
  else
    data, errobj = f:read(size)
    -- dc.log(size, data)
  end
  f:close()
  if errobj then
    return nil, errobj
  end
  return data
end

return {
  dc = nil, -- to be set be calling _init()
  _comment_ = "device image",
  _init = function (self, dc) -- called from dc
    self.dc = dc
    print("init:", self._comment_)
  end,

  -- updater features
  image = {
    read = deviceimage_read,
    read_sync = deviceimage_read_sync,
    write = deviceimage_write,
    write_sync = deviceimage_write_sync,

    -- add features to control devices ownerships. ex:
    -- * mount
    -- * umount
    -- * chmod
    -- * chown
    -- * bank switch
    -- * partion table sync (re-gen child nodes)
  }
}
