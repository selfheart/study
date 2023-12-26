/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
/* TMC CONFIDENTIAL
* $JITDFLibId$
* Copyright (C) 2022 TOYOTA MOTOR CORPORATION
* All Rights Reserved.
*/
#ifndef TMC_PROTOBUF_INTERNAL_METADATA_WRAPPER_H__
#define TMC_PROTOBUF_INTERNAL_METADATA_WRAPPER_H__

#include <lua.hpp>

int tmc_protobuf_internal_metadata_clear(lua_State *L) noexcept;
int tmc_protobuf_internal_metadata_have_unknown_fields(lua_State *L) noexcept;
int tmc_protobuf_internal_metadata_unknown_fields(lua_State *L) noexcept;
int tmc_protobuf_internal_metadata_set_unknown_fields(lua_State *L) noexcept;

#endif // TMC_PROTOBUF_INTERNAL_METADATA_WRAPPER_H__
