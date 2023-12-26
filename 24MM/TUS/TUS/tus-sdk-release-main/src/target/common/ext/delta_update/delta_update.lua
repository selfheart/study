--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]


local _success, du = pcall(function() return require("tmc_delta_update") end)
if not _success then
  return nil, du -- 'du' is actually an error message on failure
end
-- print("delta_update.lua: loaded native impl.", du)

local log = print
local delta_update = {
  request_resume = nil, -- to be set by runtime to auto-consume '_coroutines'
}

function delta_update.init(request_resume, logger)
  delta_update.request_resume = request_resume
  if logger then
    log = logger
    log("delta_update.init", request_resume, logger)
  end
end

------------------------------------------------------------
-- to be synced with src/lib/delta_update/include/delta_update.h
_states = {}
_state_names = {}
for i, name in ipairs { -- TUS_DELTA_UPDATE_STATE_*
    "INVALID",
    "CANCELED",
    "FINISHED",
    "INITIALIZED",
    "PATCH_CONT",
    "PATCH_DONE",
    "SKIP_CONT",
    "SKIP_DONE",
    "RESUMED",
    "RESUMED_SKIP",
    "SUSPENDED",
} do
  _states[name] = i - 1
  _state_names[i - 1] = name
  -- print("STATES:", name, _states[name])
end

local _results = {}
local result_names = {}
for i, name in ipairs { -- TUS_DELTA_UPDATE_PATCH_*
  "OK",
  "ERROR",
  "DONE",
  "DONE_REMAINING",
  "PATCHED_FULL",
  "SRC_SEEK",
  "DELTA_NEXT",
  "SKIP",
} do
  _results[name] = i - 1
  result_names[i - 1] = name
  -- log("_results:", name, _results[name])
end

---------------------------------------------------------------

local _coroutines  = {
  -- map stream -> coroutine
}

-- call config.on_error() if errobj was non-nil
local function notify_error(config, errobj)
  if config and config.on_error and errobj then
    print("notify error to user", config, errobj)
    config.on_error(config, errobj)
  else
    -- ignore
  end
  -- return the errobj as-is to write as 'return notify_error(..., errobj)'
  return errobj
end

-- common part of initialize/resume
local function make_stream(native, config)
  log("make_stream() args:", native, config)
  function finalizer(desc)
    return function (stream)
      if stream._native then	
	log("finalizing for:", desc, "stream=", stream, "stream._native=", stream._native)
	stream._native:finalize()
	stream._native = nil
      else
	-- log("ignore:", stream)
      end
    end
  end
  local stream_metatable = {
    __gc = finalizer("__gc"),
    __close = finalizer("__close"),
  }

  -- call config.on_patched() for currently stored patched data
  local function flush_patched(self, config)
    -- errobj (if any) is expected to be handled by the caller
    local blob, offset, errobj
    blob, errobj = self._native:get_patched_blob()
    -- ToDo: better to avoid copying blob?
    if not blob then
      return errobj or -1 -- ToDo:
    end
    offset, errobj = self._native:get_patched_offset()
    if not offset then
      return errobj or -1 -- ToDo:
    end
    -- returns nil / errobj
    return config.on_patched(config, blob, offset - string.len(blob))
  end

  -- delta-patch stream object
  local stream = {
    _native = native,
    config = config, -- note: 'config' may be shared between streams
    cancel = function(self) return self._native:cancel() end,
    skip = function(self, offset)
      local errobj
      local result = _results.DELTA_NEXT
      while result == _results.DELTA_NEXT do
	result, errobj = self._native:skip(offset)
	if not result or result == _results.ERROR then
	  log("!! skip() has failed", result, errobj)
	  if not errobj then errobj = -1 end -- ToDo:
	  return notify_error(config, errobj)

	elseif result == _results.OK then
	  -- may return an errobj
	  errobj = self._native:set_delta(config.feed_delta, config)
	  if errobj then
	    log("!! skip() has failed to set_delta", errobj)
	    return notify_error(config, errobj)
	  end
	  errobj = self._native:set_src(config.feed_src, config)
	  if errobj then
	    log("!! skip() has failed to set_src", errobj)
	    return notify_error(config, errobj)
	  end
	elseif result ~= _results.DELTA_NEXT then
	  log("skip() returned unexpected result", _result_names[result]..":".. result)
	  return notify_error(config, -1)
	end
      end
    end,
    suspend = function(self) return self._native:suspend() end,
    apply = function(self)
      if _coroutines[self] then
	return -1 -- XXX: EBUSY?
      end
      local co = coroutine.create(function()
	  log("started co for stream:", self)
	  local errobj = nil
	  while not errobj do
	    local result
	    result, errobj = self._native:patch()

	    if not result or result == _results.ERROR then
	      log("!! patch() has failed", result, errobj)
	      if not errobj then errobj = -1 end -- ToDo:
	      return errobj

	    elseif result == _results.DONE then
	      log("patch():", result, result_names[result], errobj)
	      -- expected to have no more data to flush
	      return -- let resume_coroutines() return {true,nil}, coroutine will be dead

	    elseif result == _results.DONE_REMAINING then
	      log("patch():", result, result_names[result], errobj)
	      -- expected to flush. write error shall abort the loop
	      errobj = flush_patched(self, config)
	      return errobj -- no need to call again regardless of errobj

	    elseif result == _results.PATCHED_FULL then
	      log("patch():", result, result_names[result], errobj)
	      -- requested to feed delta data, write error shall abort the loop
	      errobj = flush_patched(self, config)

	    elseif result == _results.SRC_SEEK then
	      log("patch():", result, result_names[result], errobj)
	      -- requested to feed src data
	      errobj = self._native:set_src(config.feed_src, config)

	    elseif result == _results.DELTA_NEXT then
	      log("patch():", result, result_names[result], errobj)
	      -- requested to feed delta data
	      errobj = self._native:set_delta(config.feed_delta, config)

	    else
	      log("!! patch() returned unexpected result", result, result_names[result])
	      if not errobj then errobj = result end
	      break
	    end
	    -- call user-supplied on_error() if errobj was non-nil
	    -- ToDo: better to allow to edit errobj from on_error()?
	    notify_error(config, errobj)

	    -- yielding non-nil errobj will stop further resume
	    local now = coroutine.yield(errobj)
	    log("arg from resume_coroutines()", now)
	  end

      end)
      log("apply: started for", self, co)
      _coroutines[self] = co
      if delta_update.request_resume then
	log("request to call resume now")
	delta_update.request_resume(0.0)
	-- ToDo: what to do if an error was returned?
      else
	log("!! call resume_coroutines() as needed")
      end
    end,
    get_patched_size = function(self) return self._native:get_patched_size() end,
    get_state = function(self) return self._native:get_state() end,
  }
  setmetatable(stream, stream_metatable)
  log("made a stream:", stream)
  return stream
end


-- module-global ops
function delta_update.new_stream(config)
  local native, errobj = du.initialize_stream(config)
  if not native then
    -- avoid calling on_error(), caller will get errobj synchronously
    if not errobj then errobj = -1 end
    return native, errobj
  end
  return make_stream(native, config)
end

function delta_update.resume_stream(config, suspended)
  local native, errobj = du.resume_stream(config, suspended)
  if not native then
    -- avoid calling on_error(), caller will get errobj synchronously
    if not errobj then errobj = -1 end
    return native, errobj
  end

  local stream = make_stream(native, config)
  local state = stream:get_state()

  if state == _states.RESUMED then
    -- resume fully completed
    local errobj = native:set_delta(config.feed_delta, config)
    if errobj then
      log("failed to set_delta", errobj)
      return nil, errobj
    end
    return stream
  end

  -- post-process if _SKIP
  while stream:get_state() == _states.RESUMED_SKIP do
    local errobj = stream:skip(0)
    if errobj then
      log("failed to skip(0) after resume", errobj)
      return nil, errobj
    end
  end
  log("resumed state=", _state_names[state] .. ":"..state)
  if state ~= _states.RESUMED then
    -- assume to be failed if stream's state has not converged to 'RESUMED'
    return nil, -1 -- ToDo: map to an error code
  end
  return stream
end

-- re-start conroutines created by apply()
function delta_update.resume_coroutines(now)
  local done = {}
  local has_any = false
  local call_again = false
  -- log("wakeup:", now)
  local _finished_ = {} -- need a unique hidden object
  for stream, co in pairs(_coroutines) do
    -- note: coroutine.resume() is a protected call, errors will be caught as msg
    has_any = true
    local ret, errobj = coroutine.resume(co, now)
    if ret then
      -- co could have been resumed and yielded a value
      if errobj then
	-- failed to resume a coroutine
	log("errobj from resumed coroutine:", errobj)
	done[stream] = errobj -- errobj
      else
	if coroutine.status(co) == "suspended" then
	  log("will call again", co)
	  call_again = true
	else
	  log("finished:", co)
	  done[stream] = _finished_
	end
      end
    else
      -- resume() failed due to an Lua error (raised, or co was already dead)
      log("caught a Lua error", co, errobj)
      done[stream] = errobj -- errobj
    end
  end
  for stream, msg in pairs(done) do
    local co = _coroutines[stream]
    _coroutines[stream] = nil
    if msg == _finished_ then
      log("finished coroutine for:", stream)
      -- no need to notify
    else
      -- terminating due to an error
      -- (errobj shall already have been passed to on_error before reaching here)
      log("drop co for stream:", stream, "reason="..msg)
    end
    coroutine.close(co)
  end
  if call_again then
    log("call_again:", call_again)
  else
    if has_any then
      log("no more coroutine to resume")
    end
  end
  return call_again -- there if there us a coroutine to resume()
end

return delta_update
