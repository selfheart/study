/* ***************************************************************************
 *
 *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 *
 *  The software and information contained herein is the proprietary and
 *  confidential property of Airbiquity Inc. and shall not be disclosed
 *  in whole or in part. Possession, use, reproduction or transfer of
 *  this software and information is prohibited without the express written
 *  permission of Airbiquity Inc.
 *
 *  Copyright (c) 2018 Airbiquity Inc.  All rights reserved.
 *
 * ***************************************************************************
 */

#include <spil_os/aqSpilRauc.h>
#include <ontrac/ontrac/status_codes.h>
#include <fcntl.h>
/// FIXME: remove <stdio.h>, not MISRA compilent
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* re: signal.h -- would it be possible to create a thread in the child process which will
 * listen to stdin for commands from the parent. The parent could then send a command to
 * the child's stdin which tells the child to terminate. At that point, the child could either
 * gracefully shutdown (exit the installation thread nicely), or just terminate via exit().
 * If that is doable, we could get rid fo the MISRA2012-RULE-21_5 violations.
 */
#include <signal.h> /* parasoft-suppress MISRA2012-RULE-21_5_a "Needed for backstop code that will terminate a running Rauc Installation process. No other mechanism on Linux to terminate a process." */

#include <string.h>

/* parasoft-begin-suppress MISRA2012-RULE-3_1_b "Do not embed c++ style comment within C-style comment." The 'comment(s)' in question is a URL which is informative. Visually it reads as a URL rather than beginning of a C++ style line comment. */
/**
 * TODO: support non-posix versions of windows with a CreateNamedPipe & CreateProcess
 * @ref1: http://www.ivorykite.com/winpipe.html
 * And either update the named pipe to be selectable via a socketpair:
 * @ref2: https://www.perlmonks.org/?node_id=869704
 * Or update the event-loop in spil_nio to handle named-pipes directly
 */
/* parasoft-end-suppress MISRA2012-RULE-3_1_b */

/** @private
 * RAUC installation progress as a percentage, from 0 to 100.
 */
static uint32_t installProgress = 0U;

/** @private
 * RAUC Installation status
 */
static aqSpilRaucStatus_t installStatus = aqSpilRaucStatusOK;

/** @private
 * RAUC pipe file descriptors, to monitor the stdout of the new process
 */
static int32_t aqSpilRauc_fd[2]; /* parasoft-suppress MISRA2012-RULE-8_9-4 "Global variable used by only one function so far" */

/** @private
 * RAUC child process pid
 */
static pid_t aqSpilRauc_child = 0;

int32_t aqSpilRaucTerminateInstall(void) {
    int32_t rvalue = 0;
    if (0 < aqSpilRauc_child) {
        SLOG_D("Terminating RAUC App process");
        rvalue = kill(aqSpilRauc_child, SIGTERM); /* parasoft-suppress MISRA2012-RULE-21_5_b "Needed for backstop code that will terminate a running Rauc Installation process. No other mechanism on Linux to terminate a process." */
        if (0 != rvalue) {
            rvalue = kill(aqSpilRauc_child, SIGKILL);
        }
        aqSpilRauc_child = 0;
    }
    return rvalue;
}

int32_t aqSpilRaucReadFd(fd_type fd, char *buffer, size_t length){
    return (int32_t)read((int32_t)fd, buffer, length);
}

int32_t aqSpilRaucCloseFd(fd_type fd){
    return close((int32_t)fd);
}

aqSpilRaucStatus_t aqSpilRaucUpdateInstallProgress(char * buf){
    char post_str[2048] = {0};
    char * tmp = buf;
    const char *success_str = "succeeded";
    const char *fail_str = "fail";
    int32_t number = 0;
    int32_t bytes = 0;
    int32_t stop = 0;
    if (NULL != tmp) {
        SLOG_D("Analyzing:");
        SLOG_D(tmp);
        /* Replace the following with a state machine which monitors for targetted messages. This will
         * allow for getting rid of calls to sscanf, getting rid of the MISRA2012-RULE-21_6-2
         * violations.
         */
        while ('\0' != *tmp) {
            if (1 == sscanf(tmp, "%254[^ ]%n", post_str, &bytes)) { /* parasoft-suppress MISRA2012-RULE-21_6-2
                                                                    * "sscanf() function is not allowed. Being used for rauc implementation until it can
                                                                    * be replaced by MISRA compliant function or different implementation." */
                if ((bytes > (int32_t) strlen(success_str))
                        && (0 == strncmp( post_str, success_str, strlen(success_str)))) {
                    installProgress = 100;
                    installStatus = aqSpilRaucStatusSucceeded;
                    SLOG_D("RAUC App installation finished. Need to reboot!");
                    stop = 1;
                } else if ((bytes > (int32_t) strlen(fail_str))
                        && (0 == strncmp( post_str, fail_str, strlen(fail_str)))) {
                    installProgress = 100;
                    installStatus = aqSpilRaucStatusFailed;
                    SLOG_E("RAUC App installation failed.");
                    stop = 1;
                } else {
                    if (1 == sscanf(post_str, "%d%%", &number)) { /* parasoft-suppress MISRA2012-RULE-21_6-2
                                                                   * "sscanf() function is not allowed. Being used for rauc implementation until it can
                                                                   * be replaced by MISRA compliant function or different implementation." */
                        if (0 < number) {
                            installProgress = (uint32_t) number;
                        }
                    }
                }
                if (0 != stop) {
                    break;
                }
                tmp = &tmp[bytes];
            }
            while (' ' == *tmp) {
                ++tmp;
            }
        }
        if ((aqSpilRaucStatusSucceeded != installStatus) && (aqSpilRaucStatusFailed != installStatus)) {
            installStatus = aqSpilRaucStatusInProgress;
        }
    }
    return installStatus;
}

aqSpilRaucStatus_t aqSpilRaucInstallBundle(char * bundle, int32_t * fd){
    aqSpilRaucStatus_t ret = aqSpilRaucStatusOK;
    pid_t r_pid;
    const char * cmd_args[5] = {"/usr/bin/rauc", "install", "bundle", "2>&1", NULL};
    SLOG_D("About to install:");
    SLOG_D(bundle);
    /** aqSpilRauc_fd[0] = pipe exit
     *  aqSpilRauc_fd[1] = pipe_entry */
    if ((NULL == bundle) || (NULL == fd)) {
        SLOG_E("Missing parameters");
        ret = aqSpilRaucStatusError;
    } else {
        if (-1 == pipe(aqSpilRauc_fd)) {
            SLOG_E("Unable to open pipe for RAUC");
            ret = aqSpilRaucStatusError;
        } else {
            r_pid = fork();
            if (0 == r_pid) {
                /* Child */
                (void) dup2(aqSpilRauc_fd[1], STDOUT_FILENO);
                (void) dup2(aqSpilRauc_fd[1], STDERR_FILENO);
                (void) close(aqSpilRauc_fd[1]);
                (void) close(aqSpilRauc_fd[0]);
                (void) execv(cmd_args[0], (char * const *) cmd_args);
                SLOG_E("RAUC execv failed");
            } else if (0 < r_pid) {
                // parent
                (void) close(aqSpilRauc_fd[1]);
                *fd =aqSpilRauc_fd[0];
                aqSpilRauc_child = r_pid;
            } else {
                ret = aqSpilRaucStatusError;
                SLOG_E(" spilRauc Fork failed");
            }
        }
    }
    installProgress = 0U;
    installStatus = ret;
    return ret;
}

aqSpilRaucStatus_t aqSpilRaucCheckInstallProgress(uint32_t * percent){
    *percent = installProgress;
    return installStatus;
}

aqSpilRaucStatus_t aqSpilRaucTriggerReboot(void){
    char cmd[] = {"/sbin/reboot 2>&1"};
    FILE *fp = NULL;
    aqSpilRaucStatus_t err = aqSpilRaucStatusOK;

    /* maybe:
     * sync[]
     * reboot [ LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART, NULL ]
     *
     * instead? would no longer need popen/pclose
     */

    /* Open the command for reading. */
    fp = popen(cmd, "r"); /* parasoft-suppress MISRA2012-RULE-21_6-2
                           * "popen() function is not allowed. Being used for rauc implementation until it can
                           * be replaced by MISRA compliant function or different implementation." */
    if (fp == NULL) {
        SLOG_E("Failed to run command:" );
        SLOG_E(cmd);
        err = aqSpilRaucStatusError;
    } else {
        (void) pclose(fp); /* parasoft-suppress MISRA2012-RULE-21_6-2
                            * "pclose() function is not allowed. Being used for rauc implementation until it can
                            * be replaced by MISRA compliant function or different implementation." */
    }
    return err;
}
