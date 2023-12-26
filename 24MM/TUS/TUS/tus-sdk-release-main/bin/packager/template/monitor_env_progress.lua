function(self, k, v, old_v)
      if k == "###KEY_NAME" then
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
end
