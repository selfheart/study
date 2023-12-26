--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

-- system configuration
--  (shall be retrieved from PAL)

local config_util = require('config_util')

local httpd_config = {}

-- setup configuration directories by reading 'config'
--  which may be placed along with 'main.lua' / 'runner'
local config_dirs = (function ()
    local config_base = nil
    local t = nil
    if basedir then
      -- run from $basedir/main.lua, try $basedir/config
      t = config_util:parse_config_file(basedir .. "/config")
      if t then
        config_base = basedir
      end
    end
    if not t then
      config_base = "." -- default to current working dir
      t = config_util:parse_config_file(config_base .. "/config")
      if t then
        -- print("loaded ./config", t)
      else
        -- print("no ./config")
      end
    end
    local conf = {}
    for _, k in ipairs({"provider", "domain"}) do
      if t and t[k] then
        if string.sub(t[k], 1, 1) == '/' then
          conf[k] = t[k] .. '/' -- use the absolute path from a config file
        else
          conf[k] = config_base .. '/' ..  t[k] .. '/' -- relative path from a config file
        end
      else
        -- fallback to use default relative dirs
        if basedir then
          conf[k] = basedir .. '/' .. k .. '/'
        else
          conf[k] = config_base .. '/' .. k .. '/'
        end
      end
    end

    -- UO embedded-HTTPd configurations
    for k, default in pairs({
        -- HTTP hostname for DC (IP addr or hostname of UO)
        host = "localhost",
        -- HTTP port for DC (port number / service name as a string UO)
        port = "80",
        -- URL base for DC (base of the URL's path part)
        url_base = "/tus/",
    }) do
      httpd_config[k] = t["httpd_" .. k] or default
      print("HTTPd config:", k, httpd_config[k])
    end

    conf.retry_interval = tonumber(t['retry_interval'] or 0)

    return conf
end)()

local function init_providers(path)
  local cpath = path .. "/config"
  local config = config_util:parse_config_file(cpath)

  if config and config.type then
    config.config_dir = path
    local m = require("provider/" .. config.type)
    if not m:initialize(config) then
      return {m} -- returns an array
    end
  end
  -- failed to init
  return {}
end

-- init list of domain-controllers
local function init_domain_list(dir)
  local domains = {}
  local config = config_util:parse_config_file(dir .. "/config")
  if config then
    for k, v in pairs(config) do
      -- print("config domain", k, "using", v)
      domains[k] = {}

      local dc = config_util:parse_config_file(dir .. "/" .. v)
      if dc.grpc then
        -- gRPC endpoint of the DC
        domains[k].hostname = dc.grpc
      end
    end
  else
    -- print("failed to load", dir)
  end
  return domains
end

local device_settings <const> = {
  httpd = httpd_config,
  domains = init_domain_list(config_dirs.domain),
}


--------------------------------------------------------------------------
-- utilities for Lua scripts run on an update-orchestrator

local grpc = require('tmc_grpc')
local dc_pb = require('dc_pb')
local dc_pb_grpc = require('dc_pb_grpc')

local errors = { -- fixme:
  ENOENT = {code=2,desc="ENOENT"},
  EINVAL = {code=22,desc="EINVAL"},
}

local function make_logger(prefix, level)
  return function (...)
    local s = ""
    for i = 1, select('#', ...) do
      s = s .. "\t" .. tostring(select(i, ...))
    end
    local di = nil
    pcall(function () di = debug.getinfo(level + 2) end)
    if di and di.name then
      if (string.len(di.source) < 256) then
        print(prefix .. di.source .. ":" .. di.currentline .. " " .. di.name .. "() " .. s)
      else
        print(prefix .. "string(" .. string.len(di.source) .. "):" .. di.currentline .. " " .. di.name .. "() " .. s)
      end
    else
      print(prefix .. s)
    end
  end
end

-- will say " (uo.log): @./FILE:LINE FUNC() ..."
local log = make_logger(" (uo.log): ", 2)


local function execute_on_domain(self, body)
  log("- exec on domain: " .. self.id .. ": ", body)
  local script = dc_pb.Script.new{
    parameters="",
    body=body,
    session_id=self.uo.session_id,
  }
  local response = self.grpc_stub.Execute(script)
  if response then
    return response.result_code, response.result_text
  else
    log("! no response from DC?")
    return
  end
end

local function execute_from_url(self, url)
  log("- exec on  domain '" .. self.id .. "' from URL:", url)
  local atag = self.uo:make_atag()
  local params = dc_pb.ExecuteAsyncParams.new{
    url = url,
    tag = atag,
    session_id = self.uo.session_id,
  }
  local response = self.grpc_stub.ExecuteAsync(params)
  if response then
    -- caller shall wait using 'tag'
    if response.result_code == 0 then
      log(" => ", atag)
      return atag
    else
      -- fixme: make a error obj from the code
      return nil, response.result_code
    end
  else
    log("! no response from DC?")
    return nil, errors.EINVAL
  end
end

local domains = {} -- will be used as uo.domains. initialized by build_domains() below.

local function get_domain(identifier)
  log("req:", identifier)
  for _, dc in ipairs(domains) do
    if dc.id == identifier then
      log("found: ", dc.id, dc)
      return dc
    end
  end
  return nil, errors.ENOENT
end

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

local uo = {
  -- logger
  log = log,
  errors = errors,

  providers = init_providers(config_dirs.provider),

  config = config_dirs,

  -- DC states
  domains = domains,

  get_data_url =function (self, ipkg_index, what, opt)
    if what == "UPDATEFLOW" then
      local ipkg = ""
      if ipkg_index > 0 then
        ipkg = "ipkg" .. ipkg_index
        -- outer TUP's UPDATEFLOW for ipkg#N (in pkginfo#N)
        -- ex: "http://localhost:7681/tus/tup/ipkg1/meta/updateflow"
      else
        -- outer TUP's global UPDATEFLOW
        -- ex: "http://localhost:7681/tus/tup/meta/updateflow"
      end
      return generate_url {
        path = device_settings.httpd.url_base ..
          "/tup/" .. ipkg .. "/meta/updateflow",
        scheme = "http",
        host = device_settings.httpd.host,
        port = 7681, -- tentative
      }
    end
    if what == "TUP/DATA" then
      local ipkg = ""
      if ipkg_index > 0 then
        ipkg = "ipkg" .. ipkg_index .. "/tup"
        -- inner TUP (outer's ipkg #N)'s data#N
        -- ex: "http://localhost:7681/tus/tup/ipkgN/tup/ipkgM/data"
      else
        -- outer TUP's data (== which may be a inner TUP)
        -- ex: "http://localhost:7681/tus/tup/ipkgM/data"
      end
      return generate_url {
        path = device_settings.httpd.url_base ..
          "/tup/" .. ipkg .. "/ipkg" .. opt .. "/data",
        scheme = "http",
        host = device_settings.httpd.host,
        port = 7681, -- tentative
      }
    end
    error(what)
  end,

  -- construct a DC representation on an UO
  get_domain = function (self, identifier)
    return get_domain(identifier)
  end,

  make_logger = make_logger,

  -- called on boot to return device settings
  check_device_settings = function ()
    log("! NotYet: use PAL to get device configurations")
    return device_settings
  end,

  process_update_result = function (campaign)
    -- FIXME: call ota_client to report results
    log("! NotYet: notify completion of '" .. campaign .. "'")
  end,

  last_tag = 10000,
  make_atag = function (self)
    self.last_tag = self.last_tag + 1
    return self.last_tag
  end,
  -- for TUP UO scripts (not for UO main)
  session_id = nil,
  cleanup = function(self)
    if not self.session_id then return end -- nothing to cleanup
    log(self, self.domains)
    for _, dc in ipairs(self.domains) do
      log("cleanup DC", dc, dc.id)
      if dc.fsm and dc.fsm:get_environment('connected') then
	-- update env to trigger shutdown on-DC FSM
	-- (value is expected to be a string for now)
	dc.fsm:set_environment('shutdown', 'true')
      end
      dc:execute("dc:terminate_session_from_lua('" .. self.session_id .."')")
    end
  end,
  start_grcp_worker = nil,
  update_dc_fsm = function(self, domain_name, key, value)
    log("update_dc_fsm", domain_name, key, value)
    local dc = get_domain(domain_name)
    dc.fsm.cache[key] = value
    if self.fsm._in_transit then
      return -- a trainstion is in-progress, defer starting another
    end
    self.fsm:run()
  end,
}

local function get_version(dc, target_spec)
  -- get and update dc.versions
  if target_spec == nil then
    target_spec = "" -- encoded as a string
  end
  local opts = dc_pb.Names.new({name={target_spec}})
  local versions, err = dc.grpc_stub.GetVersions(opts)
  if not versions then
    -- log(type(target_spec), target_spec, opts, "-> raw error:", err)
    return nil, err -- todo: re-convert to an error object?
  end
  if versions.result_code ~= 0 then
    log(type(target_spec), target_spec, opts, "-> raw response:", versions, err)
    return nil, versions.result_code -- todo: re-convert to an error object?
  end
  if versions then
      for _, p in ipairs(versions.pairs) do
        log("", "-", p.key, ":", p.value)
        dc.versions[p.key] = p.value
      end
  end
  return dc.versions[target_spec]
end

-- init uo.domains
uo.build_domains = function (uo, is_script_runtime)
  local key = 1 -- numeric id for domains
  for identifier, config in pairs(device_settings.domains) do
    local host = config.hostname
    -- create a gRPC stub for the domain
    local channel = grpc.insecure_channel(host)
    local dc = {
      uo = uo,
      id = identifier,
      key = key,

      grpc_stub = dc_pb_grpc.DomainControllerStub.new(channel),
      versions = {},

      get_version = get_version,

      execute = execute_on_domain,
      execute_from_url = execute_from_url,
    }
    key = key + 1

    if is_script_runtime then
      log("init for UO script runtime:", is_script_runtime)
      -- for UO script runtime, init DC FSM state proxy
      local fsm = {
        dc = dc,
        cache = {
           phase = nil, -- DC-FSM's phase
           connected = nil -- non-nil if recentry got states
        },
        set_environment = function(fsm, k, v)
          log(fsm, k, v)
          local params = dc_pb.SetEnvironmentParams.new({
              session = {id = dc.uo.session_id},
              key = k,
              value= v,
          })
          local resp = dc.grpc_stub.SetEnvironment(params)
          if resp and resp.result_code == 0 then
            return -- error object == nil
          end
          return nil, dc.uo.EINVAL -- FIXME
        end,
        get_environment = function(self, k)
          return self.cache[k]
        end,
      }
      dc.fsm = fsm
      log("- domain '" ..identifier .. "', gRPC=" .. host)
      uo.start_grpc_worker(dc.id, host)
    else
      -- check connection by querying '' (== DC itself's feature set)
      -- (all DC is expected to be able to respond to '')
      dc._feature = dc:get_version('')
      local count = 0
      while dc._feature == nil do
        log("- connecting domain '" ..identifier .. "', gRPC=" .. host, count)
        if uo.sleep then
          local interval = 3 -- seconds
          local ret = uo.sleep(interval)
          if ret ~= nil then
            log("interrupted?", ret)
            return ret
          end
          count = count + 1
          dc._feature = dc:get_version('')
        else
          -- WA if executed on non-TUS Lua
          log('missing uo.sleep(), skip connection check')
          break
        end
      end
      if dc._feature then
        log("-", "connected to '" .. dc.id .. "'", dc._feature)
      end
      -- query DC version for 'domain'
      dc.versions.domain =  dc:get_version('domain')
    end
    table.insert(uo.domains, dc)
    log("-> ", uo.domains[#domains].id, dc)
  end
end

if type(_tuslua) ~= "table" then
   -- require TUS-extgended Lua runtime
   log("imcompatible interpreter? '_tuslua' shall be a table:"
       ..type(_tuslua))
else
   -- let custom VM features accessible through 'uo'
   -- note: global "_tuslua" will be hidded from UO scripts
   log("(init with", _tuslua, ")")
   for _, name in ipairs {"has_feature",
			  "sleep",
			  "syslog",
			  "tus_config"} do
      log("import", name, _tuslua[name])
      uo[name] = _tuslua[name]
   end
end


return uo
