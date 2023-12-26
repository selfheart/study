#!/usr/bin/env lua5.4

--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

-- for testing: 'cmake .. && cmake --build  . && ./src/target/uo/main.lua'
local debug = true

-- add this script's dir to Lua module load path
--  and let modules see 'basedir'
if arg and arg[0] then
  local head, len
  head, len, basedir = string.find(arg[0], "(.*/)")
end
if not basedir then
    basedir = "./"
end
package.path = package.path..';' .. basedir .. '/?.lua;'
package.cpath = package.cpath..';' .. basedir .. '/?.so'

local tup = require('tup_parser')
local uo = require('uo')

--------------------------------------------------------------------------

local function setup(states)
  uo.log("init UO main states")
  uo:build_domains(false)
  states.alive = true
end

local function get_uo_script(parser)
  -- get a metadata (== TUP UO script, shall have been validated by the parser)
  local tlv_objs, error_obj = parser:get_tlv({id=tup.TLV_ID.UPDATEFLOW})
  if error_obj then
    uo.log("failed to get metadata?", tostring(error_obj))
    return nil, error_obj
  end
  if 1 ~= #tlv_objs then
      uo.log("unexpected numberof metadata TLVs?", #tlv_objs)
      return nil, uo.erros.EINVAL
  end
  local metadata, error_obj = tlv_objs[1]:get_value()
    if error_obj then
    return nil, error_obj
  end
  uo.log("extracted global TUP metadata (UO script)", "size =", string.len(metadata))
  return metadata
end

local function process_tup(prefix_dir, parser, url)
  -- make data in the TUP accessible through (embedded) HTTP server
  -- ToDo: support non "file:"
  local error_obj = uo.tus_config('HTTPD_SET_TUP_URL', url);
  if error_obj then return error_obj end

  -- UO script is now can be fetched from runner using HTTP
  -- (like "http://localhost:7681/tus/tup/meta/updateflow)

  local metadata_url = uo:get_data_url(0, "UPDATEFLOW");

  local p = io.popen("(cd " .. basedir .. ";./runner"
		      .. " -s " .. metadata_url
		      .. " -p " .. prefix_dir
		      .. " )", 'w')
  local success, cause, code = p:close()

  uo.log("detach the TUP from httpd", success, cause, code)
  uo.tus_config('HTTPD_SET_TUP_URL','');

  if success ~= true then
    return code -- fixme: reflect 'cause', which shall be either 'exit' or 'signal'
  end
end

local function main()
  local error_obj
  -- uo.log("!started:" .. arg[0])

  -- boot sequence
  local states = {}
  error_obj = setup(states)
  if error_obj then return error_obj end

  -- connect to update source(s)
  while states.alive do
    local applied = 0;
    for _, provider in ipairs(uo.providers) do
      -- try to find an applicable campaign
      uo.log("checking available updates", provider.id)
      local updates, error_obj = provider:check_update(uo)
      if error_obj then goto next_provider end -- ToDo: retry?

      for _, update in ipairs(updates) do
        uo.log("found a update: ", update)
        local results, error_obj = provider:download_package(update)
        if error_obj then return error_obj end -- ToDo: retry?

        -- send update-completion
        error_obj = provider:send_notification("DOWNLOADED", update)
        local parser = results.parser
        local url
        if parser == nil then
          -- try to get a parser from 'results'
          if results.path then
            uo.log("downloaded a pkg file at: ", results.path)
            parser, error_obj = tup.from_file(results.path)
          end

          if parser == nil then
            uo.log("failed to init TUP parser for", results.path)
            return error_obj
          end
          error_obj = parser:validate()
          if error_obj then
            uo.log("TUP validation failure")
            return error_obj
          end
          url = "file://" .. results.path
          uo.log("validated TUP: ", url)
        else
          -- expect to return results.url with results.parser
          url = results.url
        end

        error_obj = process_tup(applied, parser, url)
        if error_obj then
	  uo.log("UO script has reported an error: ", error_obj)
	  provider:send_notification("ABORTED", update)
	  return error_obj
	end
        -- applied update, shall re-check applicable packages again

        -- send update-completion
        applied = applied + 1
        error_obj = provider:send_notification("COMPLETED", update)
        break
      end
      if applied == 0 then
        uo.log("no more update from ", provider.id)
      end
      ::next_provider::
    end
    if applied == 0 then
      uo.log("exhausted providers, retry_interval=", uo.config.retry_interval)
      if uo.config.retry_interval > 0 then
        local ret = uo.sleep(uo.config.retry_interval)
        if ret ~= nil then
          uo.log("sleep() ->", ret, "interrupted")
          states.alive = false -- interrupted
        end
      else
        states.alive = false -- fixme: gather updated DC states
      end
    else
      uo.log("applied", applied, "update(s)")
      -- states.alive = false -- uncomment to one-by-one
    end
  end
  uo.log("(done)")
end

return main()
