uo.log("\n\t!!!!! starting TUP UO script !!!!!")
-- ToDO; place infos under 'uo'?
uo.log(" - global[PREFIX]", PREFIX)
uo.log(" - global[TUP_PATH]", TUP_PATH)
local downloaded_path = TUP_PATH
local prefix_dir = PREFIX

local dc_urls={}
local dc_list={}
local ipkg_data_urls = {}

local ipkg_count = ###IPKG_COUNT

for i = 1,ipkg_count do
uo.log("process innerpkg#", i)
-- get a metadata for inner package #1
local ipkgidx = i
local ipkg_dir = prefix_dir .. "/ipkg" .. ipkgidx

local dc_md_url, error_obj = uo:get_data_url(ipkgidx, "UPDATEFLOW")
uo.log("URL for DC script", dc_md_url)
dc_urls[i] = dc_md_url

local dc
local args = {
    index = 1,
    flags = tup.FLAG.DECOMPRESS | tup.FLAG.ICV_VERIFY
}
uo.log("########",i,args.index,args.flags)

-- first innerpkg of inner TUP[ipkgidx]
local ipkg_data_url, error_obj = uo:get_data_url(0, "TUP/DATA", ipkgidx)
if error_obj then return error_obj end

ipkg_data_urls[i] = ipkg_data_url
uo.log("ipkg_data_url =", ipkg_data_url)

###GET_DOMAIN

if error_obj then return error_obj end
dc_list[i] = dc
local vehicle_config, err_obj = dc:get_version("vehicle_config")
uo.log("domain vehicle_config ", vehicle_config, err_obj)
end


--------------------------------------------------------
-- construct UO-side FSM
local fsm = uo.fsm

###FSM_TRANSITIONS

-- construct DC-side FSM
-- (== let DC download & run TUP-script through HTTP)

local execution_tag_list = {}
for i=1,ipkg_count do
    local tag, error_obj = dc_list[i]:execute_from_url(dc_urls[i])
    if error_obj then return error_obj end
    execution_tag_list[i] = tag
end

for i=1,ipkg_count do
    local tag = execution_tag_list[i]
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
end

--------------------------------------------------------

###MONITOR_ENV_GLOBAL_STATE_START
###MONITOR_ENV_PROGRESS_START

-- UO is now ready to process events from DC, trigger DC FSM actions
for i=1,ipkg_count do
error_obj = dc_list[i].fsm:set_environment("url", ipkg_data_urls[i])
uo.log("set_environmen_url(" .. ipkg_data_urls[i] ..") ->", tag, error_obj)
if error_obj then return error_obj end
end

-- wait for UO FSM state
-- ToDo: handle errors
while uo.fsm.phase ~= "fini" do
###EXIT_ABNORMAL_STATUS
uo.log("!!!!! wait !!!!!", uo.fsm.phase)
error_obj = uo.fsm:get_events()
uo.log("->", error_obj)
end

###MONITOR_ENV_GLOBAL_STATE_END
###MONITOR_ENV_PROGRESS_END
