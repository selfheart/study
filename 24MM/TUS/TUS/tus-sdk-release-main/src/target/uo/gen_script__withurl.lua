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

  local version, err_obj = dc_ubuntu:get_version("domain")
  uo.log("domain verison", verison, err_obj)
  local version, err_obj = dc_ubuntu:get_version("")
  uo.log(" verison", verison, err_obj)

  --------------------------------------------------------
  -- construct UO-side FSM
  local fsm = uo.fsm

  error_obj = uo.fsm:add_transition {
    from='updating', to='fini',
    condition = function (fsm, tr)
      local cur = dc_ubuntu.fsm:get_environment('phase')
      local done = (cur == "fini")
      if not done then
        uo.log("waiting-DC(s) ", cur, ", block transition to", tr.to )
      end
      return done
    end,
    action = function (fsm, tr)
      uo.log("done")
    end,
  }

  error_obj = fsm:add_transition {
    from=nil, to='updating',
    action = function (fsm, tr)
      uo.log("started")
    end,
  }

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
  local outer_index = 0 -- 'pkg index' of outer TUP is 0
  -- let DC know outer tup's first data
  error_obj = dc_ubuntu.fsm:set_environment("url", uo:get_data_url(outer_index, "TUP/DATA", 1))
  if error_obj then return error_obj end

  -- UO is now ready to process events from DC, trigger DC FSM actions
  error_obj = dc_ubuntu.fsm:set_environment("unblocked", "yes")
  if error_obj then return error_obj end

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
