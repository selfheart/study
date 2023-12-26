--[[ TMC CONFIDENTIAL
 $TUSLibId$
 Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 All Rights Reserved.
]]

--[[
/* TMC CONFIDENTIAL
 * $JITDFLibId$
 * Copyright (C) 2021 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 
]]
local grpc_const = {}

-- grpc_completion_type defined in grpc_types.h of gRPC core library.
grpc_const.grpc_completion_type = {
  GRPC_QUEUE_SHUTDOWN = 0,   -- queue already shutdown
  GRPC_QUEUE_TIMEOUT = 1, -- No event before timeout
  GRPC_OP_COMPLETE = 2, -- Operation completion
  GRPC_OP_COMPLETE_WITH_FAILURE = 3 -- Operation completion but failure. It is extended constant for lua apis.
}

-- grpc_status_code defined in status.h of gRPC core library
-- @field GRPC_STATUS_OK                Not an error; returned on success 
-- @field GRPC_STATUS_CANCELLED         The operation was cancelled (typically by the caller).
-- @field GRPC_STATUS_UNKNOWN 
--    <br>
--    Unknown error.  An example of where this error may be returned is
--    if a Status value received from another address space belongs to
--    an error-space that is not known in this address space.  Also
--    errors raised by APIs that do not return enough error information
--    may be converted to this error.
-- @field GRPC_STATUS_INVALID_ARGUMENT
--    <br>
-- Client specified an invalid argument.  Note that this differs
--    from FAILED_PRECONDITION.  INVALID_ARGUMENT indicates arguments
--    that are problematic regardless of the state of the system
--    (e.g., a malformed file name).
-- @field GRPC_STATUS_DEADLINE_EXCEEDED
--    <br>
--  Deadline expired before operation could complete.  For operations
--    that change the state of the system, this error may be returned
--    even if the operation has completed successfully.  For example, a
--    successful response from a server could have been delayed long
--    enough for the deadline to expire.
-- @field GRPC_STATUS_NOT_FOUND
--    <br>
-- Some requested entity (e.g., file or directory) was not found.
-- @field GRPC_STATUS_ALREADY_EXISTS
--    <br>
-- Some entity that we attempted to create (e.g., file or directory)
--    already exists.
-- @field GRPC_STATUS_PERMISSION_DENIED
--    <br>
-- The caller does not have permission to execute the specified
--    operation.  PERMISSION_DENIED must not be used for rejections
--    caused by exhausting some resource (use RESOURCE_EXHAUSTED
--    instead for those errors).  PERMISSION_DENIED must not be
--    used if the caller can not be identified (use UNAUTHENTICATED
--    instead for those errors).
-- @field GRPC_STATUS_UNAUTHENTICATED
--    <br>
-- The request does not have valid authentication credentials for the
  --    operation.
-- @field GRPC_STATUS_RESOURCE_EXHAUSTED
--    <br>
-- Some resource has been exhausted, perhaps a per-user quota, or
  --    perhaps the entire file system is out of space.
-- @field GRPC_STATUS_FAILED_PRECONDITION
--    <br>
-- Operation was rejected because the system is not in a state
--    required for the operation's execution.  For example, directory
--    to be deleted may be non-empty, a rmdir operation is applied to
--    a non-directory, etc.
--    A litmus test that may help a service implementor in deciding
--    between FAILED_PRECONDITION, ABORTED, and UNAVAILABLE:
--    <br>
--     (a) Use UNAVAILABLE if the client can retry just the failing call.
--    <br>
--     (b) Use ABORTED if the client should retry at a higher-level
--         (e.g., restarting a read-modify-write sequence).
--    <br>
--     (c) Use FAILED_PRECONDITION if the client should not retry until
--         the system state has been explicitly fixed.  E.g., if "rmdir"
--         fails because the directory is non-empty, FAILED_PRECONDITION
--         should be returned since the client should not retry unless
--         they have first fixed up the directory by deleting files from it.
--    <br>
--     (d) Use FAILED_PRECONDITION if the client performs conditional
--         REST Get/Update/Delete on a resource and the resource on the
--         server does not match the condition. E.g., conflicting
--         read-modify-write on the same resource.
-- @field GRPC_STATUS_ABORTED
--    <br>
--    The operation was aborted, typically due to a concurrency issue
--    like sequencer check failures, transaction aborts, etc.
--    See litmus test above for deciding between FAILED_PRECONDITION,
--    ABORTED, and UNAVAILABLE.
-- @field GRPC_STATUS_OUT_OF_RANGE
--    <br>
--    Operation was attempted past the valid range.  E.g., seeking or
--    reading past end of file.
--    <br>
--    Unlike INVALID_ARGUMENT, this error indicates a problem that may
--    be fixed if the system state changes. For example, a 32-bit file
--    system will generate INVALID_ARGUMENT if asked to read at an
--    offset that is not in the range [0,2^32-1], but it will generate
--    OUT_OF_RANGE if asked to read from an offset past the current
--    file size.
--    <br>
--    There is a fair bit of overlap between FAILED_PRECONDITION and
--    OUT_OF_RANGE.  We recommend using OUT_OF_RANGE (the more specific
--    error) when it applies so that callers who are iterating through
--    a space can easily look for an OUT_OF_RANGE error to detect when
--    they are done. 
-- @field GRPC_STATUS_UNIMPLEMENTED
--    <br>
--  Operation is not implemented or not supported/enabled in this service. 
-- @field GRPC_STATUS_INTERNAL
--    <br>
--  Internal errors.  Means some invariants expected by underlying
--    system has been broken.  If you see one of these errors,
--    something is very broken. 
-- @field GRPC_STATUS_UNAVAILABLE
--    <br>
--  The service is currently unavailable.  This is a most likely a
--    transient condition and may be corrected by retrying with
--    a backoff. Note that it is not always safe to retry non-idempotent
--    operations.
--    <br>
--    WARNING: Although data MIGHT not have been transmitted when this
--    status occurs, there is NOT A GUARANTEE that the server has not seen
--    anything. So in general it is unsafe to retry on this status code
--    if the call is non-idempotent.
--    <br>
--    See litmus test above for deciding between FAILED_PRECONDITION,
--    ABORTED, and UNAVAILABLE. 
-- @field GRPC_STATUS_DATA_LOSS
--    <br>
--  Unrecoverable data loss or corruption. 
-- @field GRPC_STATUS__DO_NOT_USE
--    <br>
--  Force users to include a default branch:
grpc_const.grpc_status_code = {
  GRPC_STATUS_OK = 0,
  GRPC_STATUS_CANCELLED = 1,
  GRPC_STATUS_UNKNOWN = 2,
  GRPC_STATUS_INVALID_ARGUMENT = 3,
  GRPC_STATUS_DEADLINE_EXCEEDED = 4, 
  GRPC_STATUS_NOT_FOUND = 5,  
  GRPC_STATUS_ALREADY_EXISTS = 6,  
  GRPC_STATUS_PERMISSION_DENIED = 7,  
  GRPC_STATUS_UNAUTHENTICATED = 16,  
  GRPC_STATUS_RESOURCE_EXHAUSTED = 8,  
  GRPC_STATUS_FAILED_PRECONDITION = 9, 
  GRPC_STATUS_ABORTED = 10,  
  GRPC_STATUS_OUT_OF_RANGE = 11, 
  GRPC_STATUS_UNIMPLEMENTED = 12,  
  GRPC_STATUS_INTERNAL = 13,  
  GRPC_STATUS_UNAVAILABLE = 14, 
  GRPC_STATUS_DATA_LOSS = 15,  
  GRPC_STATUS__DO_NOT_USE = -1  
}

return grpc_const
