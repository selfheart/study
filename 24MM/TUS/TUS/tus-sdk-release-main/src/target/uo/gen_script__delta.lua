--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]



-- to be the TUP global metadata 'WORKFLOW'

function run(arg)
  uo.log("\n\t!!!!! starting TUP UO script !!!!!")
  -- ToDO; place infos under 'uo'?
  uo.log(" - global[PREFIX]", PREFIX)
  uo.log(" - global[TUP_PATH]", TUP_PATH)
  local downloaded_path = TUP_PATH
  local prefix_dir = PREFIX

  -- get a metadata for inner package #1
  local ipkgidx = 1

  local dc_md_url, error_obj = uo:get_data_url(ipkgidx, "UPDATEFLOW")
  uo.log("URL for DC script", dc_md_url)

  -- ToDo: better to derive from TUP's TLV 'DOMAIN'?
  local dc_ubuntu, err_obj = uo:get_domain("Ubuntu")
  if error_obj then return error_obj end

  --------------------------------------------------------
  -- construct UO-side FSM
  local fsm = uo.fsm

  error_obj = fsm:add_transition {
    from=nil, to='wait',
    action = function (fsm, tr)
      uo.log("started")
    end,
  }
  if error_obj then return error_obj end

  error_obj = uo.fsm:add_transition {
    from='wait', to='fini',
    condition = function (fsm, tr)
      local cur = dc_ubuntu.fsm:get_environment('phase')
      if cur == 'fini' then
	return true
      else
        uo.log("waiting state of dc_ubuntu be 'partial', still: ", cur)
	return false
      end
    end,
    action = function (fsm, tr)
      uo.log("done")
    end,
  }
  if error_obj then return error_obj end

  -- construct DC-side FSM
  -- (== let DC download & run TUP-script through HTTP)
  local tag, error_obj = dc_ubuntu:execute_from_url(dc_md_url)
  uo.log("execute_from_url(" .. dc_md_url ..") ->", tag, error_obj)
  repeat
    local completed, error_obj = uo.async:wait({tag})
    uo.log("wait", tag, "->", completed[tag], error_obj)
    if completed[tag] then
      error_obj = uo.async:drop(tag)
      if not error_obj then
        uo.log("dropped", tag)
        tag = nil
      end
    end
  until tag == nil

  --------------------------------------------------------

  -- wait for UO FSM state
  -- ToDo: handle errors
  while uo.fsm.phase ~= "fini" do
    uo.log("!!!!! wait !!!!!", uo.fsm.phase)
    error_obj = uo.fsm:get_events()
    uo.log("->", error_obj)
  end
  uo.log("!!!!! end of TUP UO script !!!!!")
end

return {
  run = run,
}
