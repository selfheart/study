--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

local function wait_tag(tag)
  -- wait tag's completion
  local value
  while tag do
    local completed
    completed = dc.async:wait({tag})
    for k, v in pairs(completed) do
      if k == tag then
        dc.log("completed async tag", tag, v)
        value = v
        dc.async:drop(tag)
        tag = nil
      end
    end
  end
  -- caller may use value.code and (optionally) value.result
  return value
end

local function run(arg)
  dc.log("!!!!!!!!!!!!!! running a (generated) script for:", arg)

  local fsm = dc.fsm
  local errobj
  
  errobj = fsm:monitor_environment(
    "progress",
    {
      -- 'self' whold be this table itself
      callback = function(self, k, v, old_v)
	if k == "progress" then
	  local last_reported = self.current
	  if last_reported then
	    -- note: zero as a number is truthy for Lua
	    if (tonumber(v) - tonumber(last_reported) < 10) then
	      dc.log("!! hide minor progress:", v, "from", last_reported)
	      return
	    end
	  end
	  dc.log("!! got progress:", v, "from", old_v)
	  dc.fsm:propagate_environment(k, v)
	  self.current = v
	end
      end,
      current = nil, -- value previoulsy used
      -- todo: keep update stamp using monotonic clock?
    }
  )
  if errobj then return errobj end

  errobj = fsm:monitor_environment(
    "global_state",
    {
      -- 'self' whold be this table itself
      callback = function(self, k, v, old_v)
	if k == "global_state" then
	  dc.log("!! got global_state:", v)
	  dc.fsm:propagate_environment(k, v, "from", old_v)
	end
      end,
      -- todo: keep update stamp using monotonic clock?
    }
  )
  if errobj then return errobj end

  errobj = fsm:add_transition {
    from=nil, to='wait',
    action = function (fsm, tr)
      dc.log("called action for nil->wait")
    end
  }
  if errobj then return errobj end

  errobj = fsm:add_transition {
    from='wait', to='partial',
    condition = function (fsm, tr)
      local passed = (fsm:get_environment('global_state') == "start")
      dc.log("check cond:", cond, passed)
      return passed
    end,
    action = function (fsm, tr)
      dc.log("unblocked transit 'wait' -> 'partial'")
    end
  }
  if errobj then return errobj end

  errobj = fsm:add_transition {
    from='partial', to='fini',
    action = function (fsm, tr)
      dc.log("transit 'partial' -> 'fini'")
    end
  }
  if errobj then return errobj end
end

return {
  run = run
}
