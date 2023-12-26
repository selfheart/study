--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

local function test_download(arg)
  dc.log("!!!!!!!!!!!!!! running 'test_download' for:", arg)
  local fsm = dc.fsm
  fsm:add_transition {
    from=nil, to='fini',
    condition = function (fsm, tr)
      local cond = fsm:get_environment('unblocked')
      local passed = (cond == "yes")
      dc.log("check cond:", cond, passed)
      return passed
    end,
    action = function (fsm, tr)
      local data_url = fsm:get_environment('url')
      local base = "/tmp/"
      local txt_path
      local error_obj

      txt_path = base .. "dc_downloaded_whole.txt"
      error_obj = dc:download(data_url, txt_path)
      if error_obj then
        dc.log("failed", txt_path, error_obj)
      end

      -- first byte will be '\0', total size will be unaffected
      txt_path = base .. "dc_downloaded_offs1.txt"
      error_obj = dc:download(data_url, txt_path, {offset=1})
      if error_obj then
        dc.log("failed", txt_path, error_obj)
        return err_obj
      end

      -- limit size by 1
      txt_path = base .. "dc_downloaded_sz1.txt"
      error_obj = dc:download(data_url, txt_path, {size=1})
      if error_obj then
        dc.log("failed", txt_path, error_obj)
        return err_obj
      end

      -- first two byte will be '\0', total size will be 11 (8+3)
      txt_path = base .. "dc_downloaded_off8_sz3.txt"
      error_obj = dc:download(data_url, txt_path, {offset=8, size=3})
      if error_obj then
        dc.log("failed", txt_path, error_obj)
        return err_obj
      end

      -- total size will be 3 (no padding at the beggining of a file)
      txt_path = base .. "dc_downloaded_off8_sz3_woff0.txt"
      error_obj = dc:download(data_url, txt_path, {offset=8, size=3, woffset=0})
      if error_obj then
        dc.log("failed", txt_path, error_obj)
        return err_obj
      end

      dc.log("(done)", error_obj)
  end}
end

return {
  run = test_download
}
