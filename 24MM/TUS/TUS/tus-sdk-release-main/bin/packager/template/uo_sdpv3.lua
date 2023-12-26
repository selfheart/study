for _,p in ipairs(uo.providers) do
  if p.update_config then
    p:update_config {
      targets = {
        ###TARGETS
      },
      rxswin_infos = {
        ###RXSWINS
      }
    }
  end
end
