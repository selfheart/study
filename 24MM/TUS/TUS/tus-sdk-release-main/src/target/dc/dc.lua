--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

-- provide global symbol 'dc' for DC main and DC script

local config_util = require('config_util')

local function log(...)
  local s = ""
  for i = 1, select('#', ...) do
    s = s .. "\t" .. tostring(select(i, ...))
  end
  local level = 1 + 1  -- skip (getinfo) & (log)
  local di = nil
  pcall(function () di = debug.getinfo(level + 2) end)
  if di and di.name then
    if (string.len(di.source) < 256) then
      print(" (dc.log): " .. di.source .. ":" .. di.currentline .. " " .. di.name .. "() " .. s)
    else -- di.source is too long to dump as a path
      print(" (dc.log): string(" .. string.len(di.source) .. "):" .. di.currentline .. " " .. di.name .. "() " .. s)
    end
  else
    print(" (dc.log): " .. s)
  end
end

local function make_errobj(code, desc)
  return {
    code = code,
    to_number = function(self) return self.code end,
    desc = desc,
    __tostring = function(self) return self.desc end,
    stack = debug.traceback("", 2), -- trace from the caller
  }
end

local updaters = {} -- initialized below

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
    if di and di.name then
      print(prefix .. di.source .. ":" .. di.currentline .. " " .. di.name .. "() " .. s)
    else
      print(prefix .. s)
    end
  end
end

-- version which represents this DC's feature set for UO
local DC_VERSION="0"

-- common part  DC main / DC script runtime,
local dc= {
  log = log,
  errors = errors,

  get_version_from_lua = nil; -- to be set from the runtime
  get_version = function (self, target_spec, variation, context)
    if not target_spec or target_spec == "" then
      -- dc:get_version(nil) is expected to return DC's feature set
      return DC_VERSION
    end
    if self.get_version_from_lua then
      -- usually comsumes =< 1KiB (up to 32 subtarget, <32 bytes of data for each ST)
      local blob_limit = 4096
      local blob, errobj = self.get_version_from_lua(target_spec, blob_limit,
					     variation or 0,
					     context or 0)
      if blob then
	log("got version:", target_spec, "->", blob, errobj)
	return blob, errobj
      else
	return nil, errobj or -1
      end
    else
      log("missing get_version_from_lua()")
      return nil, -1 -- TODO: assign error code
    end
  end,

  update_version_from_lua = nil; -- to be set from the runtime
  update_version = function (self, target_spec, value, variation, context)
    if not target_spec then
      log("cannot set version on nothing")
      return -1
    end
    if self.update_version_from_lua then
      log("update version:", target_spec, "->", value)
      local blob = self.update_version_from_lua(target_spec, value,
						variation or 0,
						context or 0)
      return
    else
      log("missing update_version_from_lua()")
      return -1 --
    end
  end,

  compare_versions = function (self, lhs, rhs, target_spec)
    if target_spec then
      return nil, make_errobj(errors.ENOENT, "notyet")
    end
    if lhs < rhs then return -1 end
    if lhs > rhs then return 1 end
    return 0; -- equal
  end,

  make_updater_from_lua = nil,
  call_updater_from_lua = nil,
  free_updater_from_lua = nil,
  get_updater = function (self, updater_id, target_id)
    log("get_updater:", updater_id, target_id)
    if self.make_updater_from_lua then
      -- log("try custom updater(from PAL):", updater_id, target_id)
      local native, errobj = self.make_updater_from_lua(updater_id, target_id)
      if native then
	-- log("made native userdata:", native)
	local updater = {
	  _native = native, -- Lua 'userdata'
	  _mt = {}, -- metatable
	  specific = {}, -- used to call as updater_xxx.specific.XXX()
	}
	updater._mt.__index = function (updater, key)
	  return function (...)
	    -- ToDo: better to let '_native' callable?
	    -- log("call_updater_from_lua:", self, native, key, ...)
	    return self.call_updater_from_lua(native, key, ...)
	  end
	end
	updater._mt.__gc = function (obj)
	  log("gone:", updater._native, self.free_updater_from_lua)
	  self.free_updater_from_lua(updater._native)
	  updater._native = nil
	end
	setmetatable(updater.specific, updater._mt)
	return updater
      end
    end
    log("try updaters", updaters)
    local updater = updaters[updater_id]
    if updater then
      if target_id then
	if updater.create_for_target then
	  local for_target, errobj = updater:create_for_target(target_id)
	  if for_target then
	    log("updater_id:", updater_id, "target_id:", target_id, "->", for_target)
	    return for_target
	  else
	    return nil, errobj
	  end
	else
	  log(updater_id, "does not accept", target_id)
	  return nil, -1
	end
      else
	-- for updaters that does not take a target
	log("updater_id:", updater_id, "->", updater)
	return updater
      end
    else
      return nil, -1 -- FIXME: construct an error obj
    end
  end,

  download = function (self, url, dst, opts)
    -- expecting global 'UO_URL' is passed from UO
    log("download URL: " .. url)
    local ret = self.fetch_from_lua(url, dst, opts)
    if ret and ret ~= 0 then
      log("download failed", ret)
      return ret -- return as an error object
    end
    log("created: " .. dst)
  end,

  specific = { -- domain-specific functionst
    dpkg = { -- Debian/Ubuntu
      install = function (path, progress_report)
        if progress_report then progress_report(0) end
        print('! NotYet, dpkg.install(): ' ..
              'os.execute("dpkg -i ' .. path .. ');')
        if progress_report then progress_report(100) end
      end,
    },
    apk = { -- Alpine APK
      install = function (path, progress_report)
        if progress_report then progress_report(0) end
        print('! NotYet, apk.install(): ' ..
              'os.execute("apk -i ' .. path .. ');')
        if progress_report then progress_report(100) end
      end,
    },
    -- ...
  },

  -- fields to be updated by inializer
  id = "unknown", -- identifier of this domain, like "ECU1"

  -- following members are valid only on DC script runtime
  session_id = nil, -- ID of current exec ctx
  fetch_from_lua = nil, -- used by download()
  fsm = nil,

  terminate_session_from_lua = nil,
  make_logger = make_logger,
}

-- to be used with xpcall()
local function error_handler(err)
  log("Lua error:", err)
  log(debug.traceback())
  return err
end

-- used as dc.fsm on DC script runtime 
local fsm = {
  dbg = "pre-init",
  phase = nil,

  env = {
    -- holds KV-pairs for fsm.get_environment() / fsm.set_environment()
  },
  monitors = {}, -- for monitor_environment()
  externals = {}, -- for monitor_externals()

  propagate_environment_native = nil,
  set_environment_native = nil,
  set_environment = function(fsm, k, v)
    -- may run monitors registered for the key

    for name, config in pairs(fsm.monitors) do
      local ret = config.callback(config, k, v, fsm.env[k])
      -- todo: utilize the returned value?
    end

    -- todo: split as set() & commit()?
    fsm.env[k] = v
    if fsm.set_environment_native then
      -- log("notify native", fsm.set_environment_native, fsm, k, v)
      fsm.set_environment_native(k, v)
    else
      log("missing fsm.set_environment_native, incompatible runtime?")
    end
    if fsm._in_transit then
      return -- a trainstion is in-progress, defer starting another
    end
    repeat
      log("try to drive FSM")
    until not fsm:run()
  end,

  get_environment = function(fsm, k)
    if k == 'phase' then -- todo: better to make get_phase() ?
      return fsm.phase
    end
    v = fsm.env[k]
    log(fsm, k, "->", v)
    return v
  end,

  -- FSM transitions
  transisions = {
  },
  add_transition = function(self, params)
    -- params = {from =, to=, action=, condition=, }
    log(self, params)
    table.insert(self.transisions, params)
    repeat
      log("try to drive FSM")
    until not self:run()
  end,

  run = function (self)
    self._in_transit = true
    for _, tr in ipairs(self.transisions) do
      local may_run = false
      if tr.from == self.phase then
        if tr.condition == nil then
          log("alwyas allowed")
          may_run = true
        else
          pcall(function() may_run = tr.condition(self, tr) end)
          log("judged", may_run)
        end
      else
        -- log("skip: ", tr.from, "!=", self.phase)
      end
      if may_run then
        log("ready to run:", tr.from,  " ==>", tr.to)
        -- call "tr.action(self, tr)", call error_handler on Lua errors
        local completed, errobj = xpcall(tr.action, error_handler, self, tr)
        if completed then
          -- no Lua exception, errobj is returned from tr.action
          log("action returned", errobj)
        else
          -- errobj is a raised object. ToDo: convert to TUS error?
          log("action RAISED", errobj)
          if not errobj then
            -- force to be non-nil, ToDo: assign an error code?
            errobj = -1
          end
        end
        if not errobj then
          self.phase = tr.to
          self.set_environment_native("phase", self.phase)
          log("changed phase to ", self.phase)
          self._in_transit = false
          return true -- may retry for new phase
        end
      end
    end
    log("=> keep phase:", self.phase)
    self._in_transit = false
    return false -- no ransition has been executed
  end,

  register_rollback = function(self)
  end,

  get_events = function(self)
  end,
}


-- let a DC FSM env's value wpdated baed on external system's states
-- note: 'checker' shall not sleep/block.
function fsm.monitor_externals(self, key, config)
  if self.externals[key] and config ~= nil then
    log("duplicated key for externals?", key)
    return self.externals[key]
  end
  log("monitor_externals", key, config)
  self.externals[key] = config

  -- update native layer's 'ctx->script.next_wakeup' and start pooling
  dc.set_next_wakeup_from_lua(0.0)
end

-- to be called periodically, 'now' is a monotonic time (as a NUMBER)
function fsm.sync_from_externals(self, now)
  local DEFAULT_INTERVAL = 1.0
  local needs_rerun = false

  local selected_key = nil

  local candidate_key = nil
  local candidate_stamp = nil

  local timed_out = {}
  log("sync_from_externals: now=", now)
  -- find a key to check
  for key, config in pairs(self.externals) do
    local previous = config.previous
    -- log("sync_from_externals: checking key=", key, "previous=", previous)
    if config.first_call == nil then
      config.first_call = now
    end
    if config.timeout and now > config.first_call + config.timeout then
      timed_out[key] = now - config.first_call
      -- continue, multiple keys may timeout at once
    elseif config.ingore_interval then
      -- the key shall be chacked (ignoring interval)
      selected_key = key
      break
    elseif previous == nil then
      -- the key has never been checked, priotize
      selected_key = key
      break
    elseif candidate_key == nil then
      -- it may be the best key
      candidate_key = key
      local interval = config.interval or DEFAULT_INTERVAL
      candidate_stamp = (previous + interval)
      log("sync_from_externals: candidate=", candidate_key, candidate_stamp)

    else
      -- note: config.previous == nil shall have been cathed above
      local interval = config.interval or DEFAULT_INTERVAL
      if candidate_stamp > (previous + interval) then
        -- switch candidate
        candidate_key = key
        candidate_stamp = (previous + interval)
      end
    end
  end
  for key, threshold in pairs(timed_out) do
    local config = self.externals[key]
    log("timed-out", key, threshold)
    self.externals[key] = nil
    if config.on_timeout then
      config.on_timeout(fsm, key)
    end
  end
  if selected_key then
    log("force check", selected_key)
  else
    if candidate_key then
      if now >= candidate_stamp then
        selected_key = candidate_key
        -- log("ready to check", selected_key)
      else
        log("shall wait until", candidate_stamp, "now=", now)
        return candidate_key, nil, candidate_stamp - now
      end
    else
      log("nothing to do")
      return nil, nil, nil -- 'no need to call again'
    end
  end

  -- check external states
  local config = self.externals[selected_key]
  config.ingore_interval = nil

  local value, errobj = self.gather_environment_native(selected_key, config.previous)
  config.previous = now -- 'previous tried' stamp shall be updated even on errors
  if value and value > "" then
    -- ToDo: allow to tweak the value using a user-supplied function?
    log("got '" .. value .. "', completed", selected_self)
    self.externals[selected_key] = nil -- 
    return selected_key, value, 0.0
  else
    log("failed to get a value for ", selected_key, errobj)
    -- ToDo: limit number of retries?
    return selected_key, nil, 0.0 -- 'try next one now'
  end
end


-- start monitoring updates on FSM environment using 'config'
function fsm.monitor_environment(self, name, config)
  if true then
    if self.monitors[name] and config ~= nil then
      -- ToDo: prefer gensym() like semantics?
      log("duplicated name?", name) 
      return self.monitors[name]
    end
    log("monitor_environment", name, config)
    self.monitors[name] = config
  end
end

-- to be called from fsm:set_environment()
-- so that external systems can be notifed about the DC FSM env. update
function fsm.propagate_environment(self, key, value)
  -- kick platform-dependant native implementation
  if fsm.propagate_environment_native then
    -- log(fsm.propagate_environment_native, key, value)
    fsm.propagate_environment_native(key, value)
  else
    log("!! missing fsm.propagate_environment_native, incompatible runtime?")
  end
end

-- initialize 'dc'

--- tentative, to be set be DC's configuretion file
local PAL = {
  updaters = {  --- list of updaters taht can be used on a DC
    debug = {}, -- for debugging
    dpkg = {
      dpkg_cmd="sudo dpkg", -- use 'sudo dpkg' instead of raw 'dpkg'
    },
    file = {},
    image = {},
  }
}

-- init local 'updates' which is used from dc.get_updater()
for updater_id, conf in pairs(PAL.updaters) do
  local updater = require("updater_" .. updater_id)
  updater:_init(dc, conf)
  for facility, ftable in pairs(updater) do
    if facility ~= "dc" and string.sub(facility, 1, 1) ~= "_" then
      log("", facility, ftable)
      if type(ftable) == "table" then
	for fname, func in pairs(ftable) do
	  log("", updater_id, "", fname, func)
	end
      end
    end
  end
  updaters[updater_id] = updater
end


-- ToDo: no need to init FSM for DC main?
dc.fsm = fsm

-- load dc/config
dc.config = {
  -- use "./config" relative to the DC execvutable by default
  _base_dir = ".",
  _reload = function (base_dir)
    dc.log("loading 'config' from", "'" .. base_dir .. "'")
    if not base_dir then
      base_dir = dc.config._base_dir
    end
    new_config = config_util:parse_config_file(base_dir, "config") or {}
    -- preserve 'config._*'
    new_config._base_dir = base_dir
    new_config._reload = dc.config._reload
    dc.config = new_config
  end,
}

return dc
