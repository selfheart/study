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

#ifndef TMC_POSIX_ERRNO_H
#define TMC_POSIX_ERRNO_H

/* POSIX 1003.1-2017 */

#define TMC_POSIX_E2BIG           0xF0010001 /* Argument list too long */
#define TMC_POSIX_EACCESS         0xF0010002 /* Permission denied */
#define TMC_POSIX_EADDRINUSE      0xF0010003 /* Address in use */
#define TMC_POSIX_EADDRNOTAVAIL   0xF0010004 /* Address family not supported */
#define TMC_POSIX_EAGAIN          0xF0010005 /* Resource unavailable try again */
#define TMC_POSIX_WOULDBLOCK      0xF0010005 /* Operation would block */
#define TMC_POSIX_EALREADY        0xF0010006 /* Connection already in progress */
#define TMC_POSIX_EBADF           0xF0010007 /* Bad file descriptor */
#define TMC_POSIX_EBADMSG         0xF0010008 /* Bad message */
#define TMC_POSIX_EBUSY           0xF0010009 /* Device or resource busy */
#define TMC_POSIX_ECANCELED       0xF001000A /* Operation canceled */
#define TMC_POSIX_ECHILD          0xF001000B /* No child processes */
#define TMC_POSIX_ECONNABORTED    0xF001000C /* Connection aborted */
#define TMC_POSIX_ECONNREFUSED    0xF001000D /* Connection refused */
#define TMC_POSIX_ECONNRESET      0xF001000E /* Connection reset */
#define TMC_POSIX_EDEADLK         0xF001000F /* Resource deadlock would occur */
#define TMC_POSIX_EDESTADDRREQ    0xF0010010 /* Destination address required */
#define TMC_POSIX_EDOM            0xF0010011 /* Mathematics argument out of domain of function */
#define TMC_POSIX_EDQUOT          0xF0010012 /* Reserved */
#define TMC_POSIX_EEXIST          0xF0010013 /* File exists */
#define TMC_POSIX_EFAULT          0xF0010014 /* Bad address */
#define TMC_POSIX_EFBIG           0xF0010015 /* File too large */
#define TMC_POSIX_EHOSTUNREACH    0xF0010016 /* Host is unreachable */
#define TMC_POSIX_EIDRM           0xF0010017 /* Identifier removed */
#define TMC_POSIX_EILSEQ          0xF0010018 /* Illegal byte sequence */
#define TMC_POSIX_EINPROGRESS     0xF0010019 /* Operation in progress */
#define TMC_POSIX_EINTR           0xF001001A /* Interrupted, function */
#define TMC_POSIX_EINVAL          0xF001001B /* Invalid argument */
#define TMC_POSIX_EIO             0xF001001C /* I/O error */
#define TMC_POSIX_EISCONN         0xF001001D /* Socket is connected */
#define TMC_POSIX_EISDIR          0xF001001E /* Is a directory */
#define TMC_POSIX_ELOOP           0xF001001F /* Too many levels of symbolic links */
#define TMC_POSIX_EMFILE          0xF0010020 /* File descriptor value too large */
#define TMC_POSIX_EMLINK          0xF0010021 /* Too many links */
#define TMC_POSIX_EMSGSIZE        0xF0010022 /* Message too large */
#define TMC_POSIX_EMULTIHOP       0xF0010023 /* Reserved */
#define TMC_POSIX_ENAMETOOLONG    0xF0010024 /* Filename too long */
#define TMC_POSIX_ENETDOWN        0xF0010025 /* Network is down */
#define TMC_POSIX_ENETRESET       0xF0010026 /* Connection aborted by network */
#define TMC_POSIX_ENETUNREACH     0xF0010027 /* Network unreachable */
#define TMC_POSIX_ENFILE          0xF0010028 /* Too many files open in system */
#define TMC_POSIX_ENOBUFS         0xF0010029 /* No buffer space available */
#define TMC_POSIX_ENODATA         0xF001002A /* No message is available on the STREAM head read queue */
#define TMC_POSIX_ENODEV          0xF001002B /* No such device */
#define TMC_POSIX_ENOENT          0xF001002C /* No such file or directory */
#define TMC_POSIX_ENOEXEC         0xF001002D /* Executable file format error */
#define TMC_POSIX_ENOLCK          0xF001002E /* No locks available */
#define TMC_POSIX_ENOLINK         0xF001002F /* Reserved */
#define TMC_POSIX_ENOMEM          0xF0010030 /* Not enough space */
#define TMC_POSIX_ENOMSG          0xF0010031 /* No message of the desired type */
#define TMC_POSIX_ENOPROTOOPT     0xF0010032 /* Protocol not available */
#define TMC_POSIX_ENOSPC          0xF0010033 /* No space left on device */
#define TMC_POSIX_ENOSR           0xF0010034 /* No STREAM resources */
#define TMC_POSIX_ENOSTR          0xF0010035 /* Not a STREAM */
#define TMC_POSIX_ENOSYS          0xF0010036 /* Functionality not supported */
#define TMC_POSIX_ENOTCONN        0xF0010037 /* The socket is not connected */
#define TMC_POSIX_ENOTDIR         0xF0010038 /* Not a directory or a symbolic link to a directory */
#define TMC_POSIX_ENOTEMPTY       0xF0010039 /* Directory not empty */
#define TMC_POSIX_ENOTRECOVERABLE 0xF001003A /* State not recoverable */
#define TMC_POSIX_ENOTSOCK        0xF001003B /* Not a socket */
#define TMC_POSIX_ENOTSUP         0xF001003C /* Not supported */
#define TMC_POSIX_EOPNOTSUPP      0xF001003C /* Operation not supported on socket */
#define TMC_POSIX_ENOTTY          0xF001003D /* Inappropriate I/O control operation */
#define TMC_POSIX_ENXIO           0xF001003E /* No such device or address */
#define TMC_POSIX_EOVERFLOW       0xF001003F /* Value too large to be stored in data type */
#define TMC_POSIX_EOWNERDEAD      0xF0010040 /* Previous owner died */
#define TMC_POSIX_EPERM           0xF0010041 /* Operation not permitted */
#define TMC_POSIX_EPIPE           0xF0010042 /* Broken pipe */
#define TMC_POSIX_EPROTO          0xF0010043 /* Protocol error */
#define TMC_POSIX_EPROTONOSUPPORT 0xF0010044 /* Protocol not supported */
#define TMC_POSIX_EPROTOTYPE      0xF0010045 /* Protocol wrong type of socket */
#define TMC_POSIX_ERANGE          0xF0010046 /* Result too large */
#define TMC_POSIX_EROFS           0xF0010047 /* Read-only file system */
#define TMC_POSIX_ESPIPE          0xF0010048 /* Invalid seek */
#define TMC_POSIX_ESRCH           0xF0010049 /* No such process */
#define TMC_POSIX_ESTALE          0xF001004A /* Reserved */
#define TMC_POSIX_ETIME           0xF001004B /* Stream ioctl() timeout */
#define TMC_POSIX_ETIMEOUT        0xF001004C /* Connection timed out */
#define TMC_POSIX_ETXTBSY         0xF001004D /* Text file buy */
#define TMC_POSIX_EXDEV           0xF001004E /* Cross-device link */

#endif // TMC_POSIX_ERRNO_H
