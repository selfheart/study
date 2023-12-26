--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]



local function run(arg)
  dc.log("!!!!!!!!!!!!!! running a (generated) script for:", arg)

  local fsm = dc.fsm
  local async_tags = {
    write = nil,
  }
  local updater = dc:get_updater("image")
  errobj = fsm:add_transition {
    from=nil, to='writing',
    action = function (fsm, tr)
      tag, errobj = updater.image.write("/tmp/image", "image_data")
      if error_obj then return error_obj end
      async_tags.write = tag
    end
  }
  if errobj then return errobj end

  errobj = fsm:add_transition {
    from='writing', to='fini',
    condition = function (fsm, tr)
      if async_tags.write then
	local ret = dc.async:wait({async_tags.write}, 0)
	dc.log("ret=", ret, async_tags.write)
	if #ret then
	  tr.tag = async_tags.write
	  async_tags.write = nil
	  return true
	end
      end
      return false
    end,
    action = function (fsm, tr)
      dc.log("tr.tag=", tr.tag)
      local error_obj = dc.async:drop(tr.tag)
      if error_obj then return error_obj end
      dc.log("(done)")
    end
  }
  if errobj then return errobj end
end

return {
  run = run
}
