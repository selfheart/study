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


local function encode_le(value, bytes)
  local ret = ""
  while bytes > string.len(ret) do
    ret = ret .. string.char(value % 256)
    value = value // 256
  end
  return ret
end

local dummy = {
  src = "aaaa",
  diff = "TUS_BSDIFF_010" ..
    encode_le(4, 8) .. -- new size
    encode_le(4, 8) .. -- ctrl[0].add
    encode_le(0, 8) .. -- ctrl[0].insert
    encode_le(4, 8) .. -- ctrl[0].seek
    encode_le(0x04030201, 4) .. -- data[0].add (="\1\2\3\4")
    ""
}

local function run(arg)
  dc.log("!!!!!!!!!!!!!! running a (generated) script for:", arg)

  local fsm = dc.fsm
  local errobj

  errobj = fsm:add_transition {
    from=nil, to='applying',
    action = function (fsm, tr)
      local config = {
	feed_src = function (config, size, offset)
	  local ret = string.sub(dummy.src, offset+1, size+offset)
	  dc.log(tostring(config)..".feed_src", config, size, offset, " -> ", offset+1, size+offset)
	  return ret
	end,
	feed_delta = function (config, size, offset)
	  local ret = string.sub(dummy.delta, offset+1, size+offset)
	  dc.log(tostring(config)..".feed_delta", config, size, offset, " -> ", offset+1, size+offset)
	  return ret
	end,
	feed_delta = function (config, size, offset)
	  dc.log(tostring(config)..".feed_delta", config, size, offset)
	  return string.sub(dummy.diff, offset|1, size+offset)
	end,
	on_patched = function (config, blob, offset)
	  ---- 'aaaa' | '\1\2\3\4' => 'bcde'
	  dc.log(tostring(config)..".on_patched(): ", "blob size=" ..string.len(blob), "offset=" .. offset, blob)
	  fsm:set_environment('applyed', 'true')
	end,
	on_error = function (config, errobj)
	  dc.log(tostring(config)..".on_error(): ", errobj)
	  fsm:set_environment('error', tostring(errobj))
	end,
      }
      local delta_stream
      delta_stream, errobj  = dc.delta_update.new_stream(config)
      if not delta_stream then
	return errobj
      end
      dc.log("stating to apply delta")
      delta_stream:apply()
    end
  }
  if errobj then return errobj end

  errobj = fsm:add_transition {
    from='applying', to='fini',
    condition = function (fsm, tr)
      local cond = fsm:get_environment('applyed')
      if cond then
	dc.log("check cond:", cond, passed)
	return true
      end
      return false
    end,
    action = function (fsm, tr)
      dc.log("(done)")
    end
  }
  if errobj then return errobj end
end

return {
  run = run
}
