--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]


-- used as 'uo.fsm' on UO script env (not to be loaded for UO main)

return {
  uo = nil, -- to be set from runner.c
  logger = nil,
  log = function(self, ...)
    if not self.loger then
      self.logger = uo.make_logger(" (uo.fsm.log): ", 3)
    end
    if self.logger then
      self.logger(...)
    end
  end,

  get_events = function (self, params)
    local ev, err_obj = self.uo.get_events_from_lua()
    self:log(params, ev, err_obj)
    return ev, err_obj
  end,

  -- FSM transitions
  transisions = {
  },
  add_transition = function(self, params)
    -- params = {from =, to=, action=, condition=, }
    -- self:log("new transition", self, params)
    table.insert(self.transisions, params)
    repeat
      -- self:log("tryng FSM transitions ...")
    until not self:run()
  end,

  run = function (self)
    self._in_transit = true
    for _, tr in ipairs(self.transisions) do
      local may_run = false
      if tr.from == self.phase then
        if tr.condition == nil then
          -- self:log("alwyas allowed")
          may_run = true
        else
          may_run = tr.condition(self, tr)
          if may_run then
            self:log("judged", may_run)
          end
        end
      else
        -- self:log("skip: ", tr.from, "!=", self.phase)
      end
      if may_run then
        self:log("", "found an allowed transision:", tr.from,  " ==>", tr.to)
        local errobj = tr.action(self, tr)
        if not errobj then
          self.phase = tr.to
          -- self.set_environment_native("phase", self.phase)
          self:log("", "new phase:", self.phase)
          self._in_transit = false
          return true -- may retry for new phase
        end
      end
    end
    self:log("=> keep phase:", self.phase)
    self._in_transit = false
    return false -- no ransition has been executed
  end,

  register_rollback = function(self)
  end,
}
