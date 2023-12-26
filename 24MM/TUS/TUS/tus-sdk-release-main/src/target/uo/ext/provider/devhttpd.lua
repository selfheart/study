--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

local config_util = require('config_util')
local tup_parser = require("tup_parser")

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
local log = make_logger(" (provider.devhttpd): ", 2)

-- values from provider configuration file (ex. provider/config)
local config = {}


------------------------------------------------------------

local function generate_url(params)
  -- authority = [userinfo "@"] host [":" port]
  local authority = nil
  if params.host then
    if params.userinfo then
      authority = params.userinfo .. "@" .. params.host
    else
      authority = params.host
    end
    if params.port then authority = authority .. ":" .. params.port end
  end

  -- URI = scheme ":" ["//" authority] path ["?" query] ["#" fragment]
  local url = params.scheme .. ":"
  if authority then
    url = url .. "//" .. authority
  end
  url = url .. params.path
  if params.query then
    url = url .. "?" .. params.query
  end
  if params.fragment then
    url = url .. "#" .. params.fragment
  end
  return url
end

-- update 'config' by reading (config.config_dir)/config
local function reload_provider_config()
  local dir = config.config_dir
  local path = config_util:concat_path(dir, "./config")
  log(dir, path)
  config = config_util:parse_config_file(path)
  config.config_dir = dir
  log(config)
end

-- create a new file from 'old_path' by replacing keys using 'edits'
-- name of new file will be 'new_path_base' .. "_" .. N
-- where N is a next safe (non existent) number next to postfix
local function prepare_config_file(old_path, new_path_base, postfix, edits)
  local f_new = nil
  local f_old = io.open(config_util:concat_path(config.config_dir, old_path), 'r')
  if not f_old then
    log("missing", old_path)
    return nil -- note: {} is truthy in Lua
  end
  local new_path = nil

  -- mimic mkstemp()
  -- note: using io.tempfile() may not be safe since
  --  /tmp and old_path may not be on a same device
  --  (which prevents rename())
  while new_path == nil do
    postfix = (postfix + 1)
    local candidate = new_path_base .. "_" .. tostring(postfix)
    -- log("candidate", candidate)
    local effective = config_util:concat_path(config.config_dir, candidate)
    local f_test = io.open(effective, 'r')
    if f_test == nil then
      -- fixme: has a race-window, to be provided as a native implementation
      f_new = io.open(effective, 'w')
      if f_new then
        new_path = candidate
        log("made", candidate, effective)
      end
    else
      f_test:close()
    end
  end

  local line = f_old:read()
  while line do
    local orig = line .. '\n'
    local pre, post = string.match(line, '([^#]*)(#.*)')
    line = string.gsub(line, '^%s+', '')
    if post then line = pre end
    line = string.gsub(line, '%s+$', '')
    local k, v = string.match(line, '([^\t =]+)[ \t]*=[ \t]*(.*)')
    -- log(k, v, line)
    if line > '' then
      if edits[k] ~= nil then
        if edits[k] then
          f_new:write(k .. " = " .. edits[k] .. '\n')
        else
          -- just remove the line if edits[k] == false
          -- (note: to write "k=false", edits[k] shall be "false")
        end
        edits[k] = nil -- remove the applied edit
      else
        f_new:write(orig)
      end
    else
      f_new:write(orig)
    end
    line = f_old:read()
  end
  f_old:close()

  -- add new lines
  for k, v in pairs(edits) do
    log("append", k, v)
    f_new:write(k .. " = " .. edits[k] .. '\n')
  end
  f_new:close()
  return new_path
end

------------------------------------------------------------


local provider = {
  id = 'devhttpd',
}

local function check_url(url)
  local parser = tup_parser.from_url(url)
  log(url, parser)
  if nil ~= parser and nil == parser:validate() then
    return parser
  else
    return nil
  end
end

function provider:check_update(uo)
  local path = config_util:concat_path(config.version, config.package_name)
  local url = generate_url {
    path = config.httpd_url_base .. path,
    scheme = "http",
    host = config.httpd_server,
    port = config.httpd_port
  }

  if check_url(url) then
    log("found update package:", url)
    return {url} -- 'updates'
  else
    log("no update package:", url)
    return {}
  end
end

function provider:download_package(update, progress, options)
  -- use the file as-is, no need to actually download
  local url = update -- check_update() returns {path} as updates
  local parser = check_url(url)
  if parser then
    log("found update package", url)
    return {    -- 'results'
      parser = parser,
      url = url,
    }
  else
    return nil, -1 -- FIXME: assign an error code for 'gone'
  end
end


function provider:send_notification(event_type, parameters)
  log(event_type, parameters)
  if event_type == "COMPLETED" then
    local old_config = "./config"
    local new_config = nil
    local version = config.version + 1
    new_config = prepare_config_file(old_config, old_config, 0, {version = version})
    if new_config then
      os.execute("mv " .. config_util:concat_path(config.config_dir, new_config) .. " "
                 .. config_util:concat_path(config.config_dir, old_config))
    end
    local err = reload_provider_config()
    if err then return nil, err end
    for k, v in pairs(config) do
      log("reloaded config:", k, v)
    end
  end
end

function provider:initialize(params)
  assert(params.type == self.id)
  self.package_path = params.package_path
  log(params.type, "will use", self.package_path)
  for k, v in pairs(params) do
    log(" - ", k, v)
    config[k] = v
  end
end

return provider

