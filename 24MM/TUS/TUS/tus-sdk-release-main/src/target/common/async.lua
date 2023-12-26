--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

return {
  parent = nil, -- to be set after loaded
  logger = nil,
  log = function(self, ...)
    if not self.loger then
      self.logger = self.parent.make_logger(" (.async.log): ", 3)
    end
    if self.logger then
      self.logger(...)
    end
  end,

  wait_from_lua = nil, -- to be set from UO/runner.c & DC/main.c
  wait = function(self, tags, timeout)
    -- 'timeout' may be omitted(or be nil), otherwise return without waiting
    local completed, error_obj
    if self.wait_from_lua then
      -- self:log("try:", tags)
      completed, error_obj = self.wait_from_lua(tags, timeout)
      if false then -- dump for debugging
        self:log("wait_from_lua() -> ", type(completed), "got:", completed, error_obj)
        for k, v in pairs(completed) do
          self:log("- " .. k)
          for k2, v2 in pairs(v) do
            self:log("--", k2, v2)
          end
        end
      end
    else
      self:log("missing wait_from_lua()?")
      completed = nil
      error_obj = self.parent.errors.EINVAL
    end
    return completed, error_obj
  end,

  drop_from_lua = nil, -- to be set from runner.c
  drop = function(self, tag)
    error_obj = self.drop_from_lua(tag)
    self:log(tag, error_obj)
    return error_obj
  end,

}
 
