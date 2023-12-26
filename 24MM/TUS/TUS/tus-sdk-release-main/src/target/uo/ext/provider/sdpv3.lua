--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

local config_util = require('config_util')
local libsdpv3 = require("libsdpv3")


local function make_logger(prefix, level)
  return function (...)
    local s = ""
    for i = 1, select('#', ...) do
      s = s .. "\t" .. tostring(select(i, ...))
    end
    local di = nil
    pcall(function () di = debug.getinfo(level + 2) end)
    if di and di.name then
      print(prefix .. di.source .. ":" .. di.currentline .. " " .. di.name .. "() " .. s)
    else
      print(prefix .. s)
    end
  end
end

-- will say " (...): @./FILE:LINE FUNC() ..."
local log = make_logger(" (provider.sdpv3): ", 2)

-- values from provider configuration file (ex. provider/config)
local config = {}


------------------------------------------------------------
local function read_rootca()
  local rootca = ""
  if config.config_dir and config.rootca then
    local path = config_util:concat_path(config.config_dir, config.rootca)
    local f = io.open(path)
    rootca = f:read("*all")
    f:close()
  end
  return rootca
end

-- update 'config' by reading (config.config_dir)/config
local function reload_provider_config()
  local dir = config.config_dir
  local path = config_util:concat_path(dir, "./config")
  log(dir, path)
  config = config_util:parse_config_file(path)
  config.config_dir = dir
  log(config)
end

-- create a new file from 'old_path' by replacing keys using 'edits'
-- name of new file will be 'new_path_base' .. "_" .. N
-- where N is a next safe (non existent) number next to postfix
local function prepare_config_file(old_path, new_path_base, postfix, edits)
  local f_new = nil
  local f_old = io.open(config_util:concat_path(config.config_dir, old_path), 'r')
  if not f_old then
    log("missing", old_path)
    return nil -- note: {} is truthy in Lua
  end
  local new_path = nil
  
  -- mimic mkstemp() 
  -- note: using io.tempfile() may not be safe since
  --  /tmp and old_path may not be on a same device
  --  (which prevents rename())
  while new_path == nil do
    postfix = (postfix + 1)
    local candidate = new_path_base .. "_" .. tostring(postfix)
    -- log("candidate", candidate)
    local effective = config_util:concat_path(config.config_dir, candidate)
    local f_test = io.open(effective, 'r')
    if f_test == nil then
      -- fixme: has a race-window, to be provided as a native implementation
      f_new = io.open(effective, 'w')
      if f_new then
        new_path = candidate
        log("made", candidate, effective)
      end
    else
      f_test:close()
    end
  end

  local line = f_old:read()
  while line do
    local orig = line .. '\n'
    local pre, post = string.match(line, '([^#]*)(#.*)')
    line = string.gsub(line, '^%s+', '')
    if post then line = pre end
    line = string.gsub(line, '%s+$', '')
    local k, v = string.match(line, '([^\t =]+)[ \t]*=[ \t]*(.*)')
    -- log(k, v, line)
    if line > '' then
      if edits[k] ~= nil then
        if edits[k] then
          f_new:write(k .. " = " .. edits[k] .. '\n')
        else
          -- just remove the line if edits[k] == false
          -- (note: to write "k=false", edits[k] shall be "false")
        end
        edits[k] = nil -- remove the applied edit
      else
        f_new:write(orig)
      end
    else
      f_new:write(orig)
    end
    line = f_old:read()
  end
  f_old:close()

  -- add new lines
  for k, v in pairs(edits) do
    log("append", k, v)
    f_new:write(k .. " = " .. edits[k] .. '\n')    
  end
  f_new:close()
  return new_path
end

local function read_rxswin()
  local rxswin = {}
  if config.config_dir and config.rxswin then
    local path = config_util:concat_path(config.config_dir, config.rxswin)
    local from_file = config_util:parse_config_file(path)
    if from_file then
      for k, v in pairs(from_file) do
        log("- RXSWIN", k, v)
        -- let 'rxswin' be an array ('k' is ignored here)
        table.insert(rxswin, {rxswin=v})
      end
    end
  end
  log(" ->", #rxswin)
  return rxswin
end

------------------------------------------------------------


local provider = {
  id = 'sdpv3',
}

local client_cached = nil

-- return {'xxx', N} from "xxx_N"
local function split_versioned_name(orig)
  local stem, postfix = string.match(orig, '([^_]*)_(.*)')
  local start = 0
  if stem == nil then
    -- no trailing "_nn"
    stem = orig
  else
    start = tonumber(string.sub(postfix, 1))
  end
  log(orig, "->", "stem", stem, "start", start)
  return stem, start
end


local function init_client(uo)
  if client_cached then
    return client_cached
  end
  local root_ca = read_rootca()
  local server_info = {
    cdn_url = config.cdn_url,
    cdn_port = config.cdn_port,
    sdp_url = config.sdp_url,
    sdp_port = config.sdp_port,
    root_ca = root_ca,
  }
  local dcm_info = {}
  for _, key in ipairs({
    "vin",
    "dcm_serial_num"})
  do
    dcm_info[key] = config[key]
  end

  client_cached = libsdpv3:new(server_info, dcm_info)
  return client_cached
end

local function read_ecu_info()
  local ecu_info = {}
  local ecu_software_part_a_id = {}
  local ecu_software_part_b_id = {}

  for k, v in pairs(config) do
    if string.sub(k, 1, 4) == "ecu/" then
      local target_id = string.sub(k, 5)
      local old_path = config_util:concat_path(config.config_dir, config[k])
      local stem, start = split_versioned_name(old_path)
      local path = config_util:concat_path(config.config_dir, config[k])
      local per_ecu = config_util:parse_config_file(path)
      per_ecu.ecu_target_id = target_id
      per_ecu.rewrite_a_count = tonumber(per_ecu.rewrite_a_count)
      per_ecu.rewrite_b_count = tonumber(per_ecu.rewrite_b_count)

      -- ecu_software_part_X_id/yyy = zzz => sub_target_id=yyy, software_id=zzz
      for key, val in pairs(per_ecu) do
        sw_part, sub_tid = string.match(key, '(ecu_software_part_%a_id)/(%w*)')
        if sub_tid then
          if sw_part == 'ecu_software_part_a_id' then
            table.insert(ecu_software_part_a_id,
                        {sub_target_id=sub_tid, software_id=val})
          elseif sw_part == 'ecu_software_part_b_id' then
            table.insert(ecu_software_part_b_id,
                        {sub_target_id=sub_tid, software_id=val})
          end
          per_ecu[key] = nil  -- remove 'ecu_software_part_%a_id/%w*' element
        end
      end
      per_ecu['ecu_software_part_a_id'] = ecu_software_part_a_id
      ecu_software_part_a_id = {}
      per_ecu['ecu_software_part_b_id'] = ecu_software_part_b_id
      ecu_software_part_b_id = {}
      log("read per ECU info from", path)

      for pk, pv in pairs(per_ecu) do
        if type(pv) == 'table' then
          for sw_k, sw_v in pairs(pv) do
            for sub_k, sub_v in pairs(sw_v) do
              log("-- "..pk, sub_k, sub_v)
            end
          end
        else
          log("-", pk, pv)
        end
      end

      table.insert(ecu_info, per_ecu)
    end
  end
  return ecu_info
end

local function make_vehicle_config()
  local vehicle_config = {
    last_completed_uid = tonumber(config.last_completed_uid),
    campaign_id = config.campaign_id,
    ecu_info = read_ecu_info(),
    rxswin_infos = read_rxswin(),
  }
  for k, v in pairs(vehicle_config) do
    log(k, v)
  end
  return vehicle_config
end


local function verify_vehicle_config(uo, vehicle_config)
  -- check vehicle_config against states read from DCs
  -- ToDo: return an errobj if something was inconsistent
  local reconstructed = {} -- 'vehicle_config from DC'
  for i, dc in ipairs(uo.domains) do
    local vehicle_config_dc, errobj = dc:get_version('vehicle_config')
    if vehicle_config_dc then
      log("DC vehicle_config[" .. dc.id .. "] =", vehicle_config_dc)
      -- ToDo: update 'reconstructed' using 'vehicle_config_dc'
    else
      -- the DC is not yet initialized/updated?
      log("DC vehicle_config[" .. dc.id .. "] is not set", errobj)
    end
  end
  if vehicle_config.ecu_info then
    log("UO vehicle_config.ecu_info:")
    for i, per_ecu in ipairs(vehicle_config.ecu_info) do
      for k, v in pairs(per_ecu) do
	if type(v) == "table" then
	  for k2, v2 in pairs(v) do
	    if type(v2) == "table" then
	      for k3, v3 in pairs(v2) do
		log("[" .. i .."] - (", k, k2, k3, v3, ")")
	      end
	    else
	      log("[" .. i .."] - (", k, k2, v2, ")")
	    end
	  end
	  -- ToDo: check whether the info matches 'reconstructed'
	else
	  log("[" .. i .."] - (", k, v, ")")
	end
      end
    end
  else
    log("no UO vehicle_config.ecu_info")
  end
end

function provider:check_update(uo)
  log("config dir:", config.config_dir)

  local client = init_client(uo)
  log("CLIENT:", client)

  local vehicle_config = make_vehicle_config()
  log(vehicle_config)

  local errobj = verify_vehicle_config(uo, vehicle_config)
  if errobj then return nil, errobj end

  local ret, campaign = client:check_campaigns(vehicle_config)
  if ret ~= 0 then
    print("check_campaigns: "..ret)
    return nil, ret
  end
  local updates = {}
  if campaign ~= nil then
    log("num_change_events=", campaign.num_change_events)
    for i = 1, campaign.num_change_events do
      log(campaign.campaign_id, "event_id[" .. i .. "]", campaign.change_event_id[i])
      table.insert(updates, {
                     campaign_id = campaign.campaign_id,
                     change_event_id = campaign.change_event_id[i],
      })
    end
  end
  return updates
end

function provider:download_package(update, progress, options)
  local client = init_client(uo)
  log("CLIENT:", client)

  if progress then
    progress(0, 100)
  end

  local ret, results = client:download_tup(update, progress, 0)
  if progress then
    progress(100, 100)
  end

  if ret ~= 0 then
    log("download failed", ret)
    return nil, ret
  end
  for k, v in pairs(results) do
    -- ex. download_file	/data/t1_app/downloads/784/702+784
    log(k, v)
  end
  if results.download_file then
    return {
      path = results.download_file
    }
  end
  log("unexpected results?")
  return nil, -1
end


function provider:send_notification(event_type, parameters)
  log(event_type, parameters)
  local client = init_client(uo)
  log("CLIENT:", client)

  local phase = 0
  local vehicle_config = nil

  local old_config = "./config"
  local new_config = nil

  if event_type == "DOWNLOADED" then
    phase = 1
  elseif event_type == "COMPLETED" then
    phase = 2
  elseif event_type == "ABORTED" then
    phase = 3
  end

  if event_type == "COMPLETED" or event_type == "ABORTED" then
    local err = reload_provider_config()
    if err then return nil, err end
    for k, v in pairs(config) do
      log("reloaded config:", k, v)
    end
    -- read ECU states using paths from new provider/config
    vehicle_config = make_vehicle_config()
    vehicle_config.campaign_id = parameters.campaign_id

    local errobj = verify_vehicle_config(uo, vehicle_config)
    if errobj then return nil, errobj end

    new_config = prepare_config_file(old_config, old_config, 0,
                                     {campaign_id = vehicle_config.campaign_id})
  end
  local num_errors = 0
  if parameters then
    -- todo: set num_errors from parameters if needed
  end
  local err = client:send_notification(
    vehicle_config,
    parameters.campaign_id,
    num_errors,
    phase)
  if err ~= 0 then
    return nil, err
  end
  if new_config then
    -- update 'campaign_id' to be used next sync (persistently)
    config.campaign_id = vehicle_config.campaign_id
    os.execute("mv " .. config_util:concat_path(config.config_dir, new_config) .. " "
               .. config_util:concat_path(config.config_dir, old_config))
  end
  log("notified", event_type)
end


-- caller is expected to pass new vehicle config in pkg_info.json v3 format
-- ex.)
-- {
--   targets = {
--     {
--       ecu_target_id = 7001,
--       new_ecu_software_part_id = "2.0"
--     }
--   },
--   targets = {
--     {
--       ecu_target_id = "01000",
--       new_ecu_software_part_id = {
--         {
--           subtarget_id = "59eef",
--           product_number = "1.1"
--         },
--         {
--           subtarget_id = "21da8",
--           product_number = "1.0"
--         },
--       }
--     },
--     {
--       ecu_target_id = "01001",
--       new_ecu_software_part_id = {
--         {
--           subtarget_id = "47f69",
--           product_number = "3.2"
--         },
--       }
--     }
--   },
--   rxswin_infos = {
--     {
--       old_rxswin = "RX AAAAA",
--       new_rxswin = "rx ccccc"
--     }
--   }
-- }
function provider:update_config(pkg_info)
  local edited = false -- update provider/config if true
  local edits = {} -- for rxswin & ecu/XXX in provider/config

  -- re-generate ecu_info for each target id
  if pkg_info.targets then
    for _, t in ipairs(pkg_info.targets) do
      local target_id = t.ecu_target_id
      local config_key = "ecu/" .. target_id
      local old_path = config[config_key]
      -- find known config
      if old_path then
        log("update ECU info target_id=".. t.ecu_target_id, "key", config_key, old_path)
        local cur = config_util:parse_config_file(config_util:concat_path(config.config_dir, old_path))
        for k, v in pairs(cur) do
          log("old:", k, v)
        end
        local e = {} -- edit info for the ECU
        if cur.active_bank == 'A' then
          e.active_bank = 'B'
          e.rewrite_b_count = (cur.rewrite_b_count or 0) + 1
          for _, s in ipairs(t.new_ecu_software_part_id) do
            e['ecu_software_part_b_id/'..s.subtarget_id] = s.product_number
          end
        elseif cur.active_bank == 'B' then
          e.active_bank = 'A'
          e.rewrite_a_count = (cur.rewrite_a_count or 0) + 1
          for _, s in ipairs(t.new_ecu_software_part_id) do
            e['ecu_software_part_a_id/'..s.subtarget_id] = s.product_number
          end
        else
          -- ToDo: single bank config?
          for _, s in ipairs(t.new_ecu_software_part_id) do
            e['ecu_software_part_a_id/'..s.subtarget_id] = s.product_number
          end
        end
        -- update per-ECU config
        local stem, start = split_versioned_name(old_path)
        local new_ecu_config_path = prepare_config_file(old_path, stem, start, e)
        edits[config_key] = new_ecu_config_path
        log("update ".. config_key, new_ecu_config_path)
        edited = true
      else
        log("!!! missing", config_key)
      end
    end
    -- ToDo: allow to remove existing target id?

    -- sync provider.config
    -- config.rxswin = edits.rxswin
  else
    log("no ecu SW version update?")
  end
  if pkg_info.rxswin_infos then
    -- rxswin_infos = { {old_rxswin = "RX AAAAA", new_rxswin = "rx ccccc"} }
    log("pkg_info.rxswin_infos", pkg_info.rxswin_infos, config.config_dir, config.rxswin)

    local old_path = config.rxswin
    log("old_path" , old_path)
    local edit_rxswin = {}

    -- expect "dataN=RX xxxxx" format
    local from_file = config_util:parse_config_file(config_util:concat_path(config.config_dir, old_path)    )

    local reverse = {}
    for k,v in pairs(from_file) do
      reverse[v] = k
    end

    for i,v in ipairs(pkg_info.rxswin_infos) do
      -- update defined RXSWINs keeping their order
      local old_key = reverse[v.old_rxswin]
      if old_key then
        log("update rxswin '".. old_key .."'", v.old_rxswin, v.new_rxswin)
        edit_rxswin[old_key] = v.new_rxswin
      else
        -- ToDo: treat as an error?
        log("skip pkg_info.rxswin_infos[" .. i .. "]",
            v.old_rxswin, v.new_rxswin)
      end
    end
    -- update rxswin-... in 'config'
    local stem, start = split_versioned_name(old_path)
    edits.rxswin = prepare_config_file(old_path, stem, start, edit_rxswin)
    log("re-gen rxswin conf", edits.rxswin)
    -- sync provider.config
    config.rxswin = edits.rxswin
    
    edited = true
  end

  if edited then
    local old_config = "./config"
    local new_config = prepare_config_file(old_config, old_config, 0, edits)
    -- rename(new_config, old_config)
    log("REMANE:", new_config, old_config)
    os.execute("mv " .. config_util:concat_path(config.config_dir, new_config)
               .. " ".. config_util:concat_path(config.config_dir, old_config))
  else
    log("KEEP: no-edits to apply")
  end
end

function provider:initialize(params)
  assert(params.type == self.id)
  -- make parameters (from config file) visible as 'config'
  for k, v in pairs(params) do
    log(" - ", k, v)
    config[k] = v
  end
end

return provider

