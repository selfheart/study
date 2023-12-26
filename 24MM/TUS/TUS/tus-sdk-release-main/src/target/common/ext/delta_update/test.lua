--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]


local basedir = "."
package.path = package.path..';' .. basedir .. '/?.lua;'
package.cpath = package.cpath..';' .. basedir .. '/?.so'

local log = print
local du = require("delta_update")
du.init(nil, log)

local function encode_le(value, bytes)
  local ret = ""
  while bytes > string.len(ret) do
    ret = ret .. string.char(value % 256)
    value = value // 256
  end
  return ret
end

local dummy = {
  src = "aaaabbbb".."xxxxyyyy",
  delta = "TUS_BSDIFF_010" ..
    encode_le(4+4+8+8, 8) .. -- new size
    -- chunk#0, expected offset = 0
    encode_le(0, 8) .. -- ctrl[0].add
    encode_le(4, 8) .. -- ctrl[0].insert (4bytes from data[0].insert == "new1")
    encode_le(0, 8) .. -- ctrl[0].seek (next offset = 0)
    "" .. -- data[0].add
    "new1" .. -- data[0].insert
    -- chunk#1, expected offset = 0
    encode_le(4, 8) .. -- ctrl[1].add
    encode_le(0, 8) .. -- ctrl[1].insert
    encode_le(4, 8) .. -- ctrl[1].seek (next = cur + 4 + 4 = 8)
    encode_le(0x0, 4) .. -- data[1].add (== use 4 bytes as-is from src[0] == 'aaaa')
    "" .. -- data[1].insert
    -- chunk#2, expected offset = 8
    encode_le(8, 8) .. -- ctrl[2].add
    encode_le(0, 8) .. -- ctrl[2].insert
    encode_le(0x800000000000000a, 8) .. -- ctrl[2].seek (-10 from 8+8 -> 6)
    encode_le(0x0000010101010101, 8) .. -- data[2].add ("xxxxyyyy"+"\1\1\1\1\1\1\0\0" = 'yyyyzzyy')
    "" .. -- data[2].insert
    -- chunk#3 expected offset = 6
    encode_le(8, 8) .. -- ctrl[3].add
    encode_le(0, 8) .. -- ctrl[3].insert
    encode_le(0, 8) .. -- ctrl[3].seek
    encode_le(0x00, 8) .. -- data[3].add  (== use as-is "bb"+"xxxxyy")
    "" .. -- data[3].insert
    "", -- end of data
  _expected_patched_size = (8+8+8), -- sum of each chunk size
}

local config = {
  feed_src = function (config, size, offset)
    local ret = string.sub(dummy.src, offset+1, size+offset) 
    log("config.feed_src", config, size, offset, " -> ", offset+1, size+offset)
    return ret
  end,
  feed_delta = function (config, size, offset)
    local ret = string.sub(dummy.delta, offset+1, size+offset) 
    log("config.feed_delta", config, size, offset, " -> ", offset+1, size+offset)
    return ret
  end,
  _expected_patched = {},
  on_patched = function (config, blob, offset)
    log("config.on_patched(): ", "blob size=" ..string.len(blob), "offset=" .. offset, "'"..blob.."'")
    if #config._expected_patched > 0 then
      local expected = config._expected_patched[1]
      if expected ~= blob then
	config.error = "'"..expected .. "' != '" .. blob .. "'"
	return config.error
      else
	log(" (got expected '" .. expected .. "')")
      end
      table.remove(config._expected_patched, 1)
    end
  end,
  error = nil,
  on_error = function (config, errobj)
    log("config.on_error(): ", errobj)
    config.error = errobj
  end,
}


local function test()
  local errobj
  config._expected_patched = {"new1aaaayyyyzzyybbxxxxyy"}
  local s <close>  = du.new_stream(config)
  log("du.new_stream() -> ", s)
  assert(s ~= nil)

  local patched_size = s:get_patched_size()
  log("s:get_patched_size()=", patched_size)
  assert(patched_size == dummy._expected_patched_size)

  errobj = s:apply()
  assert(errobj == nil)

  local ret = true
  local now = 0
  while ret do
    now = now + 1
    ret, errobj = du.resume_coroutines(now)
    log("- du.resume_coroutines() -> ", ret, errobj)
    assert(errobj == nil)    
  end
  assert(config.error == nil)
  log("(done)")
end
test()


local function test_skip(offset)
  local errobj
  -- set expected (offsetted) value
  config._expected_patched = {string.sub("new1aaaayyyyzzyybbxxxxyy", offset + 1)}
  local s <close>  = du.new_stream(config)
  log("du.new_stream() -> ", s)
  assert(s ~= nil)

  local patched_size = s:get_patched_size()
  log("s:get_patched_size()", patched_size)
  assert(patched_size == dummy._expected_patched_size)

  errobj = s:skip(offset)
  assert(errobj == nil)
  if errobj then
    log("!! got error", errobj)
    return errobj
  end

  assert(config.error == nil)
  log("s:skip(" .. offset ..") completed")
  errobj = s:apply()
  assert(errobj == nil)

  local ret = true
  local now = 0
  while ret do
    now = now + 1
    ret, errobj = du.resume_coroutines(now)
    log("- du.resume_coroutines() -> ", ret, errobj)
    assert(errobj == nil)    
  end
  assert(config.error == nil)
  log("(done)")
end
test_skip(1)
test_skip(3)


local function test_susreg()
  local errobj
  local o <close>  = du.new_stream(config)
  log("du.new_stream() -> ", o)
  assert(o ~= nil)

  local patched_size = o:get_patched_size()
  log("o:get_patched_size()", patched_size)
  assert(patched_size == dummy._expected_patched_size)

  local b = o:suspend()
  log("o:suspend() ->", string.len(b))
  
  local s <close>, errobj = du.resume_stream(config, b)
  log("du.resume_stream() -> ", s, errobj)
  assert(s ~= nil)

  errobj = s:apply()
  assert(errobj == nil)

  local ret = true
  local now = 0
  while ret do
    now = now + 1
    ret, errobj = du.resume_coroutines(now)
    log("- du.resume_coroutines() -> ", ret, errobj)
    assert(errobj == nil)    
  end
  assert(config.error == nil)
  log("(done)")
end
test_susreg()


function test_cancel()
  -- for '__gc()'
  local s = du.new_stream(config)
  assert(s ~= nil)

  local errobj = s:cancel()
  log("cancel ->", errobj)
  assert(errobj == nil)
  
  s = nil -- make 's' collect-able
  collectgarbage()
  collectgarbage()
  assert(config.errobj == nil)
end
test_cancel()


function test_resume_from_invalid()
  local r,e  = du.resume_stream(config, "...")
  log("resume_stream->", r,e)
  assert(r == nil)
  assert(e ~= nil)
end
test_resume_from_invalid()


local config_err = {
  feed_src = function (config, size, offset)
    return nil, 5 -- always return EIO
  end,
  feed_delta = function (config, size, offset)
    local ret = string.sub(dummy.delta, offset+1, size+offset) 
    log("config.feed_delta", config, size, offset, " -> ", offset+1, size+offset)
    return ret
  end,
  on_patched = function (config, blob, offset)
    log("config.on_patched(): ", "blob size=" ..string.len(blob), "offset=" .. offset, "'"..blob.."'")
  end,
  on_error = function (config, errobj)
    log("config.on_error(): ", errobj)
  end,
}
local function test_err_src()
  local errobj
  local s <close>  = du.new_stream(config_err)
  log("du.new_stream() -> ", s)
  assert(s ~= nil)

  local patched_size = s:get_patched_size()
  log("s:get_patched_size()", patched_size)
  assert(patched_size == dummy._expected_patched_size)

  errobj = s:apply()
  assert(errobj == nil)    

  local ret = true
  local now = 0
  while ret do
    now = now + 1
    ret, errobj = du.resume_coroutines(now)
    log("- du.resume_coroutines() -> ", ret, errobj)
    assert(errobj == nil)    
  end
  assert(config.error == nil)
  log("(done)")
end
-- assert(test_err_src() == nil)

-- force GC
collectgarbage()
collectgarbage()
