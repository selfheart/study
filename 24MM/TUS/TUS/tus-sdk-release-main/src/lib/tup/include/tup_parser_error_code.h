/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
#ifndef TUP_PARSER_ERROR_CODE_H
#define TUP_PARSER_ERROR_CODE_H

#define TUP_PARSER_ERROR_INVALID_ARGUMENT (0xF00F0001)
#define TUP_PARSER_ERROR_OUT_OF_RANGE (0xF00F0002)
#define TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY (0xF00F0003)
#define TUP_PARSER_ERROR_INTEGRITY_CHECK (0xF00F0004)
#define TUP_PARSER_ERROR_NOT_FOUND_TLV (0xF00F0005)
#define TUP_PARSER_ERROR_INVALID_ALGORITHM (0xF00F0006)
#define TUP_PARSER_ERROR_INVALID_KEY_INFORMATION (0xF00F0007)
#define TUP_PARSER_ERROR_NOT_FOUND_ICV_INFORMATION (0xF00F0008)
#define TUP_PARSER_ERROR_CANNOT_OPEN_STREAM (0xF00F0009)
#define TUP_PARSER_ERROR_CANNOT_READ_STREAM (0xF00F000A)
#define TUP_PARSER_ERROR_CANNOT_CLOSE_STREAM (0xF00F000B)
#define TUP_PARSER_ERROR_INVALID_ENDIAN (0xF00F000C)
#define TUP_PARSER_ERROR_INVALID_META_CONTEXT (0xF00F000D)
#define TUP_PARSER_ERROR_MISMATCH_TLV_ID (0xF00F000E)
#define TUP_PARSER_ERROR_SMALL_DATA_SIZE (0xF00F000F)
#define TUP_PARSER_ERROR_DECOMPRESS (0xF00F0010)
#define TUP_PARSER_ERROR_IMPORT_KEY (0xF00F0011)
#define TUP_PARSER_ERROR_VERIFY_SIGNATURE (0xF00F0012)
#define TUP_PARSER_ERROR_GENERATE_ICV (0xF00F0013)

#endif // TUP_PARSER_ERROR_CODE_H
