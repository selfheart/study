-- Generate By protoc-gen-grpc-lua Do not Edit

local pkg = {}
pkg["dc_pb_grpc"] = {}
pkg["dc_pb"] = require("dc_pb")

-- Class Definition (TUS.DomainController)
pkg["dc_pb_grpc"].DomainControllerStub = {}

pkg["dc_pb_grpc"].DomainControllerStub.new = function(channel)
  local obj = {}
  obj.GetVersions = channel:unary_unary(
    "/TUS.DomainController/GetVersions",
    pkg["dc_pb"].Names,
    pkg["dc_pb"].Versions
  )
  obj.Execute = channel:unary_unary(
    "/TUS.DomainController/Execute",
    pkg["dc_pb"].Script,
    pkg["dc_pb"].Response
  )
  obj.PollEnv = channel:unary_unary(
    "/TUS.DomainController/PollEnv",
    pkg["dc_pb"].Session,
    pkg["dc_pb"].Updates
  )
  obj.ExecuteAsync = channel:unary_unary(
    "/TUS.DomainController/ExecuteAsync",
    pkg["dc_pb"].ExecuteAsyncParams,
    pkg["dc_pb"].Response
  )
  obj.SetEnvironment = channel:unary_unary(
    "/TUS.DomainController/SetEnvironment",
    pkg["dc_pb"].SetEnvironmentParams,
    pkg["dc_pb"].Response
  )
  return obj
end

return pkg["dc_pb_grpc"]
