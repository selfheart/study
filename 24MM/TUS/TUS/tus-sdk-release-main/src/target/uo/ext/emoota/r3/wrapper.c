/* TMC CONFIDENTIAL
* $TUSLibId$
* Copyright (C) 2022 TOYOTA MOTOR CORPORATION
* All Rights Reserved.
*/

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "t1_layer/t1_layer.h"
#include "cli_lib.h"

#define SDPV3_CLIENT_META_CONTEXT "sdpv3_client.context"
#define STACK_TOP_INDEX (-1)


typedef struct {
    lua_State *L;
    t1_ctx_t t1_ctx;
    sdpv3_cfg_t sdpv3_cfg;
    void *upt_ctx;
    uint16_t qty;
    util_json_t *mem;
} context;

static context *context_check(lua_State *L, int index)
{
    return (context *)luaL_checkudata(L, index, SDPV3_CLIENT_META_CONTEXT);
}

// Get a ephemeral string from Lua and copy it to 'buf' if it was large enough.
// 'buf' shall be able to hold 'limit' bytes (including a terminator).
static int copy_lua_string(lua_State *L, const char *key, char *buf, size_t limit)
{
    if (lua_getfield(L, -1, key) != LUA_TSTRING) {
        printf("%s: field_type != LUA_TSTRING\n", key);
        return -1;
    }
    size_t len;
    const char *value = luaL_checklstring(L, -1, &len);
    if (limit <= len) {
        printf("%s: len(%s) cannot be >= %zu\n", key, value, limit);
        return -1;
    }
    // note: '+1" is to copy '\0' at tail
    memcpy(buf, value, len + 1);
    lua_remove(L, -1);

    return 0;
}

static int parse_server_config(lua_State *L, sdpv3_cfg_t *tmp_sdpv3_cfg,
                                const char **root_ca)
{
    int index = STACK_TOP_INDEX;

    /* parse dcm_info */
    luaL_checktype(L, index, LUA_TTABLE);

    if (copy_lua_string(L, "vin", tmp_sdpv3_cfg->campaign_vin,
        sizeof(tmp_sdpv3_cfg->campaign_vin)) < 0) {
        return -1;
    }

    if (copy_lua_string(L, "dcm_serial_num", tmp_sdpv3_cfg->primary_serial,
        sizeof(tmp_sdpv3_cfg->primary_serial)) < 0) {
        return -1;
    }

    lua_pop(L, 1);

    /* parse server_info */
    luaL_checktype(L, index, LUA_TTABLE);

    if (copy_lua_string(L, "cdn_port", tmp_sdpv3_cfg->cdn_port,
        sizeof(tmp_sdpv3_cfg->cdn_port)) < 0) {
        return -1;
    }

    if (copy_lua_string(L, "cdn_url", tmp_sdpv3_cfg->cdn_url,
        sizeof(tmp_sdpv3_cfg->cdn_url)) < 0) {
        return -1;
    }
    if (copy_lua_string(L, "sdp_port", tmp_sdpv3_cfg->sdp_port,
        sizeof(tmp_sdpv3_cfg->sdp_port)) < 0) {
        return -1;
    }

    if (copy_lua_string(L, "sdp_url", tmp_sdpv3_cfg->sdp_url,
        sizeof(tmp_sdpv3_cfg->sdp_url)) < 0) {
        return -1;
    }

    if (lua_getfield(L, index, "root_ca") != LUA_TSTRING) {
        printf("root_ca: field_type != LUA_TSTRING\n");
        return -1;
    }
    *root_ca = luaL_checkstring(L, index);
    lua_remove(L, index);

    lua_pop(L, 1);

    return 0;
}

#define MAX_RXSWIN_NUMBER (8)
static int parse_rxswins(lua_State *L, t1_ctx_t *t1_ctx, sdpv3_cfg_t *sdpv3_cfg)
{
    int index = STACK_TOP_INDEX;
    const char *rxswin[MAX_RXSWIN_NUMBER];
    lua_Unsigned tmp;
    uint8_t i, num_rxswins;
    int j, rxswin_len;
    unsigned int k = 0;

    if (lua_getfield(L, index, "rxswin_infos") != LUA_TTABLE) {
        printf("rxswin_infos: field_type != LUA_TTABLE\n");
        return -1;
    }
    luaL_checktype(L, index, LUA_TTABLE);
    tmp = lua_rawlen(L, index);
    if (tmp > MAX_RXSWIN_NUMBER) {
        printf("num_rxswins is too many (> %d)\n", MAX_RXSWIN_NUMBER);
        return -1;
    }
    num_rxswins = tmp;

    for (i = 0; i < num_rxswins; i++) {
        lua_pushnumber(L, i + 1);
        lua_gettable(L, -2);
        if (lua_type(L, -1) != LUA_TTABLE) {
            printf("rxswin[%d]: field_type != LUA_TTABLE\n", i + 1);
            return -1;
        }
        if (lua_getfield(L, index, "rxswin") != LUA_TSTRING) {
            printf("rxswin[%d]: field_type != LUA_TSTRING\n", i + 1);
            return -1;
        }
        rxswin[i] = luaL_checkstring(L, index);
        if (strlen(rxswin[i]) > 255) {
            printf("rxswin[%d]: is too long (> 255)\n", i + 1);
            return -1;
        }
        lua_remove(L, index);
    }
    lua_pop(L, 2);

    // <str_len><base16>: "RX AAAAA" -> "085258204141414141"
    for (i = 0; i < num_rxswins; i++) {
        rxswin_len = strlen(rxswin[i]);
        if (k + 2 > MAX_RXSWIN_SZ - 1) {
            printf("RXSWINS is too long: %u (>%u)\n", k + 2, MAX_RXSWIN_SZ);
            return -1;
        }
        sprintf(&sdpv3_cfg->rxswins[k], "%02x", rxswin_len);
        k += 2;

        // base16 encode
        for (j = 0; j < rxswin_len; j++) {
            if (k + 2 > MAX_RXSWIN_SZ - 1) {
                printf("RXSWINS is too long: %u (>%u)\n", k + 2, MAX_RXSWIN_SZ);
                return -1;
            }
            sprintf(&sdpv3_cfg->rxswins[k], "%02x", rxswin[i][j]);
            k += 2;
        }
    }
    sdpv3_cfg->rxswins[k] = '\0';
    t1_ctx->rxswinInfo = sdpv3_cfg->rxswins;

    return 0;
}

/* Sample ECU Information:
 *
 * ecuInfo_t ecusInfo[NUM_ECUS] = {
 *      {
 *          .targetId = TARGET_ID,
 *          .serialNum = ECU_NAME,
 *          .active_bank = {
 *              .bank = BANK_A,
 *              .swDetailNum = 1,
 *              .software_details = {
 *                  {
 *                      .subTargetId = SUBTARGET_ID,
 *                      .softwareId = "1.0",
 *                  }
 *              },
 *              .hardware_id = ECU_HW_ID,
 *              .rewrite_count = 1},
 *          .inactive_bank = {
 *              .bank = BANK_B,
 *              .swDetailNum = 1,
 *              .software_details = {
 *                  {
 *                      .subTargetId = SUBTARGET_ID,
 *                      .softwareId = "0.0"
 *                  },
 *              },
 *              .hardware_id = ECU_HW_ID,
 *              .rewrite_count = 0}
 *      },
 * };
 */
static int parse_ecu_info(lua_State *L, sdpv3_cfg_t *sdpv3_cfg)
{
    int i, j;
    const char *active_bank;
    targetBank_t *bank_a, *bank_b;
    int index = STACK_TOP_INDEX;

    lua_Unsigned tmp1;
    lua_Integer tmp2;
    ecuInfo_t *ecus_info;
    uint8_t *num_ecus = &sdpv3_cfg->numEcusInfo;

    if (lua_getfield(L, index, "ecu_info") != LUA_TTABLE) {
        printf("ecu_info: field_type != LUA_TTABLE (%d)\n",
                lua_getfield(L, index, "ecu_info"));
        return -1;
    }
    luaL_checktype(L, index, LUA_TTABLE);
    tmp1 = lua_rawlen(L, index);
    if (tmp1 < 1 || tmp1 > MAX_TOTAL_ECUS) {
        printf("invalid num_ecus count: %llu (0 < num_ecus <= %u)\n",
                tmp1, MAX_TOTAL_ECUS);
        return -1;
    }
    *num_ecus = tmp1;

    if (sdpv3_cfg->ecusInfo == NULL) {
        sdpv3_cfg->ecusInfo = (ecuInfo_t *)calloc(*num_ecus, sizeof(ecuInfo_t));
        if (sdpv3_cfg->ecusInfo == NULL) {
            printf("ecusInfo allocation failed\n");
            return -1;
        }
    }
    ecus_info = sdpv3_cfg->ecusInfo;

    const int top = lua_gettop(L);
    for (i = 0; i < *num_ecus; i++) {
        int sub_top;

        lua_settop(L, top);
        lua_pushnumber(L, i + 1);
        lua_gettable(L, -2);
        if (lua_type(L, -1) != LUA_TTABLE) {
            printf("ecu_info[%d]: field_type != LUA_TTABLE\n", i);
            return -1;
        }

        /* ecusInfo[i].targetId */
        if (lua_getfield(L, index, "ecu_target_id") != LUA_TSTRING) {
            printf("ecu_info[%d].ecu_target_id: field_type != LUA_TSTRING\n", i);
            return -1;
        }
        strncpy(ecus_info[i].targetId,
                luaL_checkstring(L, index), MAX_TARGET_ID_SZ);
        lua_remove(L, index);

        /* ecusInfo[i].serialNum */
        if (lua_getfield(L, index, "serial_num") != LUA_TSTRING) {
            printf("ecu_info[%d].serial_num: field_type != LUA_TSTRING\n", i);
            return -1;
        }
        strncpy(ecus_info[i].serialNum,
                luaL_checkstring(L, index), MAX_SERIAL_NUM_SZ);
        lua_remove(L, index);

        /* ecusInfo[i].(in)active_bank.bank */
        if (lua_getfield(L, index, "active_bank") != LUA_TSTRING) {
            printf("ecu_info[%d].active_bank: field_type != LUA_TSTRING\n", i);
            return -1;
        }
        active_bank = luaL_checkstring(L, index);
        if (0 == strncasecmp(active_bank, "A", 2)) {
            bank_a = &(ecus_info[i].active_bank);
            bank_b = &(ecus_info[i].inactive_bank);
        } else if (0 == strncasecmp(active_bank, "B", 2)) {
            bank_b = &(ecus_info[i].active_bank);
            bank_a = &(ecus_info[i].inactive_bank);
        } else {
            printf("ecu_info[%d].active_bank: neither A nor B\n", i + 1);
            return -1;
        }
        bank_a->bank = BANK_A;
        bank_b->bank = BANK_B;
        lua_remove(L, index);

        /* ecusInfo[i].(in)active_bank.swDetailNum (#A) */
        if (lua_getfield(L, index, "ecu_software_part_a_id") != LUA_TTABLE) {
            printf("ecu_software_part_a_id: field_type != LUA_TTABLE (%d)\n",
                    lua_getfield(L, index, "ecu_info"));
            return -1;
        }
        luaL_checktype(L, index, LUA_TTABLE);
        tmp1 = lua_rawlen(L, index);
        if (tmp1 < 1 || tmp1 > MAX_SW_DETAILS_NUM) {
            printf("invalid swDetailNum(#A) count: %llu (0 < swDetailNum <= %u)"
                    "\n", tmp1, MAX_SW_DETAILS_NUM);
            return -1;
        }
        bank_a->swDetailNum = tmp1;

        /* ecusInfo[i].(in)active_bank.software_details[j] (#A) */
        sub_top = lua_gettop(L);
        for (j = 0; j < bank_a->swDetailNum; j++) {
            lua_settop(L, sub_top);
            lua_pushnumber(L, j + 1);
            lua_gettable(L, -2);

            /* ecusInfo[i].(in)active_bank.software_details[j].subTargetId (#A) */
            if (lua_getfield(L, index, "sub_target_id") != LUA_TSTRING) {
                printf("ecu_info[%d].software_details[%d].subTargetId:"
                       " field_type != LUA_TSTRING\n", i, j);
                return -1;
            }
            strncpy(bank_a->software_details[j].subTargetId,
                    luaL_checkstring(L, index), MAX_SUB_TARGET_ID_SZ);
            lua_remove(L, index);

            /* ecusInfo[i].(in)active_bank.software_details[j].softwareId (#A) */
            if (lua_getfield(L, index, "software_id") != LUA_TSTRING) {
                printf("ecu_info[%d].software_details[%d].softwareId:"
                       " field_type != LUA_TSTRING\n", i, j);
                return -1;
            }
            strncpy(bank_a->software_details[j].softwareId,
                    luaL_checkstring(L, index), MAX_ECU_SW_ID_SZ);
            lua_remove(L, index);
        }
        lua_pop(L, 2);

        /* ecusInfo[i].(in)active_bank.swDetailNum (#B) */
        if (lua_getfield(L, index, "ecu_software_part_b_id") != LUA_TTABLE) {
            printf("ecu_software_part_b_id: field_type != LUA_TTABLE (%d)\n",
                    lua_getfield(L, index, "ecu_info"));
            return -1;
        }
        luaL_checktype(L, index, LUA_TTABLE);
        tmp1 = lua_rawlen(L, index);
        if (tmp1 < 1 || tmp1 > MAX_SW_DETAILS_NUM) {
            printf("invalid swDetailNum(#B) count: %llu (0 < swDetailNum <= %u)"
                    "\n", tmp1, MAX_SW_DETAILS_NUM);
            return -1;
        }
        bank_b->swDetailNum = tmp1;

        /* ecusInfo[i].(in)active_bank.software_details[j] (#B) */
        sub_top = lua_gettop(L);
        for (j = 0; j < bank_b->swDetailNum; j++) {
            lua_settop(L, sub_top);
            lua_pushnumber(L, j + 1);
            lua_gettable(L, -2);

            /* ecusInfo[i].(in)active_bank.software_details[j].subTargetId (#B) */
            if (lua_getfield(L, index, "sub_target_id") != LUA_TSTRING) {
                printf("ecu_info[%d].software_details[%d].subTargetId:"
                       " field_type != LUA_TSTRING\n", i, j);
                return -1;
            }
            strncpy(bank_b->software_details[j].subTargetId,
                    luaL_checkstring(L, index), MAX_SUB_TARGET_ID_SZ);
            lua_remove(L, index);

            /* ecusInfo[i].(in)active_bank.software_details[j].softwareId (#B) */
            if (lua_getfield(L, index, "software_id") != LUA_TSTRING) {
                printf("ecu_info[%d].software_details[%d].softwareId:"
                       " field_type != LUA_TSTRING\n", i, j);
                return -1;
            }
            strncpy(bank_b->software_details[j].softwareId,
                    luaL_checkstring(L, index), MAX_ECU_SW_ID_SZ);
            lua_remove(L, index);
        }
        lua_pop(L, 2);

        /* ecusInfo[i].(in)active_bank.hardware_id (#1) */
        if (lua_getfield(L, index, "ecu_hardware_part_a_id") != LUA_TSTRING) {
            printf("ecu_info[%d].ecu_hardware_part_a_id: field_type != LUA_TSTRING\n", i);
            return -1;
        }
        strncpy(bank_a->hardware_id,
                luaL_checkstring(L, index), MAX_PART_NUM_SZ);
        lua_remove(L, index);

        /* ecusInfo[i].(in)active_bank.hardware_id (#2) */
        if (lua_getfield(L, index, "ecu_hardware_part_b_id") != LUA_TSTRING) {
            printf("ecu_info[%d].ecu_hardware_part_b_id: field_type != LUA_TSTRING\n", i);
            return -1;
        }
        strncpy(bank_b->hardware_id,
                luaL_checkstring(L, index), MAX_PART_NUM_SZ);
        lua_remove(L, index);

        /* ecusInfo[i].(in)active_bank.rewrite_count (#1) */
        if (lua_getfield(L, index, "rewrite_a_count") != LUA_TNUMBER) {
            printf("ecu_info[%d].rewrite_a_count: field_type != LUA_TNUMBER\n", i);
            return -1;
        }
        tmp2 = luaL_checkinteger(L, index);
        if ((tmp2 < 0) || (tmp2 > UINT32_MAX)) {
            printf("rewrite_a_count: out of range\n");
            return -1;
        }
        bank_a->rewrite_count = tmp2;
        lua_remove(L, index);

        /* ecusInfo[i].(in)active_bank.rewrite_count (#2) */
        if (lua_getfield(L, index, "rewrite_b_count") != LUA_TNUMBER) {
            printf("ecu_info[%d].rewrite_b_count: field_type != LUA_TNUMBER\n", i);
            return -1;
        }
        tmp2 = luaL_checkinteger(L, index);
        if ((tmp2 < 0) || (tmp2 > UINT32_MAX)) {
            printf("rewrite_b_count: out of range\n");
            return -1;
        }
        bank_b->rewrite_count = tmp2;
        lua_remove(L, index);
    }
    lua_pop(L, 2);

    return 0;
}

static int
parse_vehicle_config(lua_State *L, t1_ctx_t *t1_ctx, sdpv3_cfg_t *sdpv3_cfg)
{
    int index = STACK_TOP_INDEX;

    t1_ctx->upd = "5"; // "ignition"

    luaL_checktype(L, index, LUA_TTABLE);

    if (copy_lua_string(L, "campaign_id", t1_ctx->lastCmpId,
        sizeof(t1_ctx->lastCmpId)) < 0) {
        return -1;
    }

    if (lua_getfield(L, index, "last_completed_uid") != LUA_TNUMBER) {
        printf("last_completed_uid: field_type != LUA_TNUMBER\n");
        return -1;
    }
    t1_ctx->lastUid = luaL_checknumber(L, index);
    lua_remove(L, index);

    /* parse rxswins */
    if (parse_rxswins(L, t1_ctx, sdpv3_cfg) != 0) {
        printf("parse_rxswins() failed\n");
        return -1;
    }

    /* parse ecu_info */
    if (parse_ecu_info(L, sdpv3_cfg) != 0) {
        printf("parse_ecu_info() failed\n");
        return -1;
    }

    lua_pop(L, 2);

    return 0;
}


static int sdpv3_new(lua_State *L)
{
    int ret = -1;
    const char *root_ca = NULL;
    size_t root_ca_len;
    sdpv3_cfg_t tmp_sdpv3_cfg;

    ret = parse_server_config(L, &tmp_sdpv3_cfg, &root_ca);
    if (ret != 0) {
        return 0;
    }

    context *ctx;
    ctx = (context *)lua_newuserdata(L, sizeof(context));
    ctx->L = L;
    memset(&ctx->t1_ctx, 0, sizeof(t1_ctx_t));
    ctx->upt_ctx = NULL;
    ctx->sdpv3_cfg = tmp_sdpv3_cfg;
    ctx->sdpv3_cfg.ecusInfo = NULL;

    root_ca_len = strlen(root_ca);
    if (root_ca_len > 0) {
        ctx->sdpv3_cfg.rootca = calloc(1U, root_ca_len + 1);
        if (NULL == ctx->sdpv3_cfg.rootca) {
            ret = -1;
            goto finish;
        }
        memcpy((void *)ctx->sdpv3_cfg.rootca, root_ca, root_ca_len);
    } else {
        ctx->sdpv3_cfg.rootca = NULL;
    }

    ctx->t1_ctx.requestBuffer = calloc(1U, T1_BUF_SZ);
    if (NULL == ctx->t1_ctx.requestBuffer) {
        ret = -1;
        goto finish;
    }
    ctx->t1_ctx.requestBuffer_sz = T1_BUF_SZ;
    ctx->t1_ctx.responseBuffer = calloc(1U, T1_BUF_SZ);
    if (NULL == ctx->t1_ctx.responseBuffer) {
        ret = -1;
        goto finish;
    }
    ctx->t1_ctx.responseBuffer_sz = T1_BUF_SZ;
    ctx->t1_ctx.mpuInitResponse =
                    (mpuInitResponse_t *) calloc(1U, sizeof(mpuInitResponse_t));
    if (NULL == ctx->t1_ctx.mpuInitResponse) {
        ret = -1;
        goto finish;
    }
    ctx->qty = 4096;   // 4KiB
    ctx->mem = (util_json_t *) calloc(ctx->qty, sizeof(util_json_t));
    if (NULL == ctx->mem) {
        ret = -1;
        goto finish;
    }

    /* initialize */
    ret = sdpv3_client_init(ctx->sdpv3_cfg.rootca, root_ca_len);
    if (ret != 0) {
        printf("sdpv3_client_init() failed (%d)\n", ret);
    }

 finish:
    //lua_settop(L, 0);
    if (ret != 0) {
        if(NULL != ctx->t1_ctx.mpuInitResponse){
            free(ctx->t1_ctx.mpuInitResponse);
        }
        if(NULL != ctx->t1_ctx.responseBuffer){
            free(ctx->t1_ctx.responseBuffer);
        }
        if(NULL != ctx->t1_ctx.requestBuffer){
            free(ctx->t1_ctx.requestBuffer);
        }
        if(NULL != ctx->sdpv3_cfg.rootca){
            free((void *)ctx->sdpv3_cfg.rootca);
        }
        lua_pushnil(L);
    } else {
        luaL_getmetatable(L, SDPV3_CLIENT_META_CONTEXT);
        lua_setmetatable(L, -2);
    }
    //lua_settop(L, 0);
    //luaL_getmetatable(L, SDPV3_CLIENT_META_CONTEXT);
    //lua_setmetatable(L, -2);
    //lua_pushinteger(L, ret);
    //lua_insert(L, -2);

    return 1;
}

static int sdpv3_delete(lua_State *L)
{
    context *ctx = (context *)luaL_checkudata(L, 1, SDPV3_CLIENT_META_CONTEXT);

    if (ctx == NULL) {
        return 0;
    }
    if (ctx->mem != NULL) {
        free(ctx->mem);
        ctx->mem = NULL;
    }
    if (ctx->sdpv3_cfg.rootca != NULL) {
        free((void *)ctx->sdpv3_cfg.rootca);
        ctx->sdpv3_cfg.rootca = NULL;
    }
    if (ctx->t1_ctx.requestBuffer != NULL) {
        free(ctx->t1_ctx.requestBuffer);
        ctx->t1_ctx.requestBuffer = NULL;
    }
    if (ctx->t1_ctx.responseBuffer != NULL) {
        free(ctx->t1_ctx.responseBuffer);
        ctx->t1_ctx.responseBuffer = NULL;
    }
    if (ctx->t1_ctx.mpuInitResponse != NULL) {
        free(ctx->t1_ctx.mpuInitResponse);
        ctx->t1_ctx.mpuInitResponse = NULL;
    }
    if (ctx->sdpv3_cfg.ecusInfo != NULL) {
        free(ctx->sdpv3_cfg.ecusInfo);
        ctx->sdpv3_cfg.ecusInfo = NULL;
    }

    return 0;
}

static void dump_ecu_info(ecuInfo_t *ecu_info)
{
    printf("dump ecu_info:\n");
    printf(".targetId = %s\n", ecu_info->targetId);
    printf(".serialNum = %s\n", ecu_info->serialNum);
    printf(" *** active ***\n");
    printf(".bank = %u\n", ecu_info->active_bank.bank);
    printf(".subTargetId = %s\n", ecu_info->active_bank.software_details[0].subTargetId);
    printf(".softwareId = %s\n", ecu_info->active_bank.software_details[0].softwareId);
    printf(".swDetailNum = %hu\n", ecu_info->active_bank.swDetailNum);
    printf(".hardware_id = %s\n", ecu_info->active_bank.hardware_id);
    printf(".rewrite_count = %u\n", ecu_info->active_bank.rewrite_count);

    printf(" *** inactive ***\n");
    printf(".bank = %u\n", ecu_info->inactive_bank.bank);
    printf(".subTargetId = %s\n", ecu_info->inactive_bank.software_details[0].subTargetId);
    printf(".softwareId = %s\n", ecu_info->inactive_bank.software_details[0].softwareId);
    printf(".swDetailNum = %hu\n", ecu_info->inactive_bank.swDetailNum);
    printf(".hardware_id = %s\n", ecu_info->inactive_bank.hardware_id);
    printf(".rewrite_count = %u\n", ecu_info->inactive_bank.rewrite_count);

    return;
}

static int check_campaigns(lua_State *L)
{
    int ret = -1;
    context *ctx = context_check(L, 1);
    if (ctx == NULL) {
        return 1;
    }

    ret = parse_vehicle_config(L, &ctx->t1_ctx, &ctx->sdpv3_cfg);
    if (ret != 0) {
        goto finish;
    }
    //dump_ecu_info(ctx->sdpv3_cfg.ecusInfo);

    /* check available campaigns */
    int n_campaigns = sdpv3_check_available_campaigns(&ctx->t1_ctx,
                                                        &ctx->sdpv3_cfg,
                                                        &ctx->upt_ctx,
                                                        ctx->mem, ctx->qty);
    if (n_campaigns < 0) {
        printf("sdpv3_check_available_campaigns() failed (%d)\n", n_campaigns);
        ret = -1;
    } else if (n_campaigns == 0) {
        printf("No applicable campaign found\n");
        ret = 0;
    } else {
        printf("Applicable campaign found\n");
        ret = 0;
    }

 finish:
    lua_settop(L, 0);
    lua_pushinteger(L, ret);
    if (ret != 0) {
        lua_pushnil(L);
    } else {
        int i;
        if ((n_campaigns > 0) && (ctx->t1_ctx.selected_campaign[0].ceNum > 0)) {
            lua_newtable(L);
            lua_pushstring(L, "campaign_id");
            lua_pushstring(L, ctx->t1_ctx.selected_campaign[0].campaignId);
            lua_settable(L, -3);
            lua_pushstring(L, "num_change_events");
            lua_pushinteger(L, ctx->t1_ctx.selected_campaign[0].ceNum);
            lua_settable(L, -3);
            lua_pushstring(L, "change_event_id");
            lua_newtable(L);
            for (i = 0; i < ctx->t1_ctx.selected_campaign[0].ceNum; i++) {
                lua_pushinteger(L, i + 1);
                lua_pushstring(L, ctx->t1_ctx.selected_campaign[0].changeEvents[i].elementId);
                lua_settable(L, -3);
            }
            lua_settable(L, -3);
        } else {
            lua_pushnil(L);     // No Campaigns Available
        }
    }

    return 2;
}

static int download_tup(lua_State *L)
{
    int ret = -1;
    context *ctx = context_check(L, 1);
    if (ctx == NULL) {
        return 1;
    }

    int index = STACK_TOP_INDEX;
    size_t range = 0;

    if (lua_type(L, index) != LUA_TNUMBER) {
        printf("set range: %zu\n", range);
        range = luaL_checkinteger(L, index);
    }
    lua_pop(L, 1);

    if (lua_type(L, index) != LUA_TFUNCTION) {
        /* TODO: set callback function */
        // callback = 
    }
    lua_pop(L, 1);

    if (lua_type(L, index) != LUA_TTABLE) {
        printf("campaign_info != LUA_TTABLE\n");
        goto finish;
    }

    if (lua_getfield(L, index, "campaign_id") != LUA_TSTRING) {
        printf("campaign_id: field_type != LUA_TSTRING\n");
        goto finish;
    }
    // already assigned, so not used for now
    const uint8_t *campaign_id = (const uint8_t *)luaL_checkstring(L, index);
    lua_remove(L, index);

    if (lua_getfield(L, index, "change_event_id") != LUA_TSTRING) {
        printf("change_event_id: field_type != LUA_TSTRING\n");
        goto finish;
    }
    // already assigned, so not used for now
    const uint8_t *change_event_id = (const uint8_t *)luaL_checkstring(L, index);
    lua_remove(L, index);

    /* FIXME: should campaignID and changeID be overwritten? */

    ret = sdpv3_download_tup(&ctx->t1_ctx, &ctx->sdpv3_cfg, ctx->upt_ctx);
    if (ret != 0) {
        printf("sdpv3_download_tup() failed (%d)\n", ret);
        goto finish;
    } else {
        printf("downloaded package: [%s]\n", ctx->t1_ctx.dld_pkg_path);
    }

 finish:
    lua_settop(L, 0);
    lua_pushinteger(L, ret);
    if (ret != 0) {
        lua_pushnil(L);
    } else {
        lua_newtable(L);
        lua_pushstring(L, ctx->t1_ctx.dld_pkg_path);
        lua_setfield(L, -2, "download_file");
    }

    return 2;
}

static int send_notification(lua_State *L)
{
    int ret = -1;
    context *ctx = context_check(L, 1);
    if (ctx == NULL) {
        return 1;
    }

    int index = STACK_TOP_INDEX;
    lua_Integer tmp;

    if (lua_type(L, index) != LUA_TNUMBER) {
        printf("phase: ltype != LUA_TNUMBER (%d)\n", lua_type(L, index));
        goto finish;
    }
    tmp = luaL_checkinteger(L, index);
    if ((tmp < PHASE_DOWNLOADED) || (tmp > PHASE_ABORTED)) {
        printf("phase: out of range\n");
        goto finish;
    }
    uint8_t phase = (uint8_t)tmp;
    lua_pop(L, 1);

    if (lua_type(L, index) != LUA_TNUMBER) {
        printf("num_errors: ltype != LUA_TNUMBER\n");
        goto finish;
    }
    tmp = luaL_checkinteger(L, index);
    if ((tmp > MAX_EVENT_OEM_ERR_NUM) || (tmp < 0)) {
        printf("num_errors: out of range\n");
        goto finish;
    }
    uint8_t num_errors = (uint8_t)tmp;
    lua_pop(L, 1);

    if (lua_type(L, index) != LUA_TSTRING) {
        printf("campaign_id: ltype != LUA_TSTRING\n");
        goto finish;
    }
    // already assigned, so not used for now
    const uint8_t *campaign_id = (const uint8_t *)luaL_checkstring(L, index);
    lua_pop(L, 1);

    if (num_errors == 0) {
        ctx->sdpv3_cfg.nty_err_code = EVENT_STATUS_SUCCESS;
    } else {
        ctx->sdpv3_cfg.nty_err_code = EVENT_STATUS_FAILURE;
    }

    // set status of event notification to failure when abort is requested
    if (phase == PHASE_ABORTED) {
        ctx->sdpv3_cfg.nty_err_code = EVENT_STATUS_FAILURE;
    }

    if (phase == PHASE_COMPLETED) {
        ret = parse_vehicle_config(L, &ctx->t1_ctx, &ctx->sdpv3_cfg);
        if (ret != 0) {
            goto finish;
        }
    }

    ret = sdpv3_notify_update_result(&ctx->t1_ctx, &ctx->sdpv3_cfg,
                                        ctx->upt_ctx, phase);
    if (ret != 0) {
        printf("sdpv3_notify_update_result() failed (%d)\n", ret);
    }

 finish:
    lua_settop(L, 0);
    lua_pushinteger(L, ret);
    return 1;
}


static int luaopen_sdpv3_lib(lua_State *L)
{
    static const struct luaL_Reg lib_sdpv3[] = {
        {"new", sdpv3_new},
        {NULL, NULL}
    };
    luaL_newlib(L, lib_sdpv3);
    return 1;
}

static const struct luaL_Reg context_methods[] = {
    {"check_campaigns", check_campaigns},
    {"download_tup", download_tup},
    {"send_notification", send_notification},
    {"__gc", sdpv3_delete},
    {NULL, NULL}
};

int luaopen_libsdpv3(lua_State* L) {
    luaL_newmetatable(L, SDPV3_CLIENT_META_CONTEXT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, context_methods, 0);
    luaopen_sdpv3_lib(L);

    return 1;
}
