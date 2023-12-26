--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

local function start_aync_op(updater)
  local tag = dc.async.generate_async_tag_from_lua()
  dc.log("created async tag", tag)

  local errobj
  if tag ~= 0 then
    -- mark the async op completed with code==0 on return
    -- so that succeeding wait() will see the result
    errobj = dc.async:set_async_results_from_lua(tag, 0)
  else
    errobj = -1 -- fixme: cannot reach here for now
  end
  if errobj then
    return nil, errobj
  end
  return tag
end

return {
  dc = nil, -- to be set be calling _init()
  _comment_ = "dummy implementation to test updater framework",
  _init = function (self, dc) -- called from dc
    self.dc = dc
    dc.log("init:", self._comment_)
  end,

  -- updater features
  specific = {
    start_aync_op = start_aync_op,
  }
}

