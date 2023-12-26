--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]
local dpkg_cmd = "dpkg"

local function dpkg_install(path)
  dc.log(path)

  local tag = dc.async.generate_async_tag_from_lua()
  dc.log("created async tag", tag)
  if tag == 0 then
    return nil, -1 -- shall not happen
  end

  local succerss = false
  local p = io.popen("sudo dpkg --install " .. path, 'r')
  if p then
    local result = p:read("*a")
    success = p:close()
  end
  
  if success ~= true then
    dc.log("failed to install", pkg_name)
    local code = -1  -- fixme: convert
    errobj = dc.async:set_async_results_from_lua(tag, code)
  else
    dc.log("installed", path)
    errobj = dc.async:set_async_results_from_lua(tag, 0)
  end

  return tag
end

local function dpkg_uninstall(pkg_names)
  local tag = dc.async.generate_async_tag_from_lua()
  dc.log("created async tag", tag)
  if tag == 0 then
    return nil, -1 -- shall not happen
  end

  local success = true
  for _,pkg_name in ipairs(pkg_names) do
    dc.log(pkg_name)
    local p = io.popen("sudo dpkg --purge " .. pkg_name, 'r')
    if p then
      local result = p:read("*a")
      success = p:close() -- excpect success or fail
    end
    if success ~= true then
      dc.log("failed to uninstall", pkg_name)
      break
    else
      dc.log("uninstalled", pkg_name)
    end
  end

  if success ~= true then
    local code = -1  -- fixme: convert
    errobj = dc.async:set_async_results_from_lua(tag, code)
  else
    dc.log("uninstalled", pkg_name)
    errobj = dc.async:set_async_results_from_lua(tag, 0)
  end
  return tag
end

local function dpkg_read_metadata(path)
  dc.log(path)
  local p = io.popen("dpkg-deb --info " .. path, 'r')
  if p then
    local result = p:read("*a")
    local success = p:close()
    if success then
      return result
    else
      return nil, -1 -- todo: error handling
    end
  end
  return nil, -1 -- // todo: error handling
end

local function dpkg_get_status(pkg_names)
  local results = {}
  for _,pkg_name in ipairs(pkg_names) do
    dc.log(pkg_name)
    local p = io.popen("dpkg-query --status " .. pkg_name, 'r')
    if p then
      local result = p:read("*a")
      local success = p:close()
      dc.log("query", pkg_name, success)
      if success then
        table.insert(results, result)
      else
        return nil, -1 -- todo: error handling
      end
    end
  end
  return results
end

return {
  dc = nil, -- to be set be calling _init()
  _comment_ = "debian package",
  _init = function (self, dc, conf) -- called from dc
    self.dc = dc
    if conf.dpkg_cmd then
      dpkg_cmd = conf.dpkg_cmd
    end
    dc.log("init:", self._comment_, "dpkg_cmd:", dpkg_cmd)
  end,

  -- updater features
  specific = {
    install = dpkg_install,
    uninstall = dpkg_uninstall,
    read_metadata = dpkg_read_metadata,
    get_status = dpkg_get_status,
  }
}

