/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
/* TMC CONFIDENTIAL
* $JITDFLibId$
* Copyright (C) 2021 TOYOTA MOTOR CORPORATION
* All Rights Reserved.
*/
#ifndef TMC_PROTOBUF_MESSAGE_LITE_H__
#define TMC_PROTOBUF_MESSAGE_LITE_H__

#include <string>
#include <lua.hpp>

#include <google/protobuf/message_lite.h>
#include <google/protobuf/generated_message_util.h>

class MessageLiteLua : public google::protobuf::MessageLite
{
public:
    inline MessageLiteLua() {}
    ~MessageLiteLua();

    // MessageLite
    MessageLiteLua *New() const;
    MessageLiteLua *New(google::protobuf::Arena *arena) const override final;
    void CheckTypeAndMergeFrom(const google::protobuf::MessageLite &other) override final;
    int GetCachedSize() const override final;
    bool IsInitialized() const override final;
    std::string GetTypeName() const override final;
    void Clear() override final;
    size_t ByteSizeLong() const override final;
    const char *_InternalParse(const char *ptr, google::protobuf::internal::ParseContext *ctx) override final;
    google::protobuf::uint8 *_InternalSerialize(google::protobuf::uint8 *ptr, google::protobuf::io::EpsCopyOutputStream *stream) const override final;

    // Helper
    void SetCachedSize(int size);
    void SetTypeName(const char *type_name);
    const google::protobuf::internal::InternalMetadata *GetInternalMetadata() const;
    void SetLuaState(lua_State *L);
    void SetLuaSelfIndex(int index);

private:
    int CallError(int error_code, const char *msg) const;

private:
    std::string type_name;
    google::protobuf::internal::CachedSize cached_size;
    lua_State *L;
    int index;
};

#endif // TMC_PROTOBUF_MESSAGE_LITE_H__
