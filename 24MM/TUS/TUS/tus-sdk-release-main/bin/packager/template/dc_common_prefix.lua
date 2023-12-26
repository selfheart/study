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

dc.log("!!!!!!!!!!!!!! running a (generated) script for:", dc.id)

local fsm = dc.fsm
dc.log("FSM:", fsm)

-- TODO: WA feed_delta config nil access error
local inner_tup_parser
local delta_tup_parser 
local delta_patch_size
