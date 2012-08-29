/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcitx/ime.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-config/xdg.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/utils.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/instance.h>
#include <fcitx/context.h>
#include <fcitx/module.h>
#include <fcitx/hook.h>
#include <fcitx/keys.h>
#include <libintl.h>

#include "eim.h"

static void *FcitxPyLoggerCreate(FcitxInstance *instance);
static void FcitxPyLoggerDestroy(void *arg);
static void FcitxPyLoggerReloadConfig(void *arg);
static void PyLoggerReset(PyLogger *logger);

FCITX_DEFINE_PLUGIN(fcitx_pylogger, module, FcitxModule) = {
    .Create = FcitxPyLoggerCreate,
    .Destroy = FcitxPyLoggerDestroy,
    .SetFD = NULL,
    .ProcessEvent = NULL,
    .ReloadConfig = FcitxPyLoggerReloadConfig
};

CONFIG_DEFINE_LOAD_AND_SAVE(PyLogger, PyLoggerConfig, "fcitx-pylogger")

static void PyLoggerEditFree(void *arg)
{
    PyLoggerEdit *edit = arg;
    fcitx_utils_free(edit->before);
    fcitx_utils_free(edit->after);
}

const UT_icd pyedit_icd = {
    sizeof(PyLoggerEdit),
    NULL,
    NULL,
    PyLoggerEditFree
};

static int
check_im_type(PyLogger *logger)
{
    FcitxIM *im = FcitxInstanceGetCurrentIM(logger->owner);
    if (!im)
        return 0;
    if (!strcmp(im->uniqueName, "pinyin"))
        return 1;
    return 0;
}

static char *
PyLoggerGetPreedit(PyLogger *logger)
{
    FcitxInputState *input;
    input = FcitxInstanceGetInputState(logger->owner);
    return FcitxUIMessagesToCString(FcitxInputStateGetPreedit(input));
}

static void
PyLoggerEditPush(PyLogger *logger, char *before, char *after)
{
    UT_array *edits = &logger->log.edit;
    PyLoggerEdit new_edit;
    PyLoggerEdit *last_edit = (PyLoggerEdit*)utarray_back(edits);
    if (!before) {
        before = strdup("");
    }
    if (!after) {
        after = strdup("");
    }
    if (last_edit && last_edit->after &&
        !strcmp(last_edit->after, before)) {
        free(last_edit->after);
        last_edit->after = after;
        fcitx_utils_free(before);
        return;
    }
    new_edit.before = before;
    new_edit.after = after;
    utarray_push_back(edits, &new_edit);
}

static boolean
FcitxPyLoggerPreHook(void *arg, FcitxKeySym sym, unsigned int state,
                     INPUT_RETURN_VALUE *retval)
{
    PyLogger *logger = (PyLogger*)arg;
    if (logger->busy)
        return false;
    if (!check_im_type(logger)) {
        PyLoggerReset(logger);
        return false;
    }
    if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE) ||
        FcitxHotkeyIsHotKey(sym, state, FCITX_DELETE)) {
        char *before;
        char *after;
        before = PyLoggerGetPreedit(logger);
        if (!before)
            return false;
        if (!*before) {
            free(before);
            return false;
        }
        logger->edited = true;
        logger->busy = true;
        *retval = FcitxInstanceProcessKey(logger->owner, FCITX_PRESS_KEY,
                                          time(NULL), sym, state);
        logger->busy = false;
        if (!*retval || (*retval & IRV_FLAG_FORWARD_KEY)) {
            *retval = IRV_FLAG_FORWARD_KEY;
        } else {
            *retval = IRV_FLAG_BLOCK_FOLLOWING_PROCESS;
        }
        if (!logger->edited) {
            fcitx_utils_free(before);
            return true;
        }
        after = PyLoggerGetPreedit(logger);
        PyLoggerEditPush(logger, before, after);
        return true;
    }
    return false;
}

static void
PyLoggerWriteLog(PyLogger *logger)
{
    UT_array *edits = &logger->log.edit;
    PyLoggerEdit *edit;
    if (!logger->log.commit) {
        logger->log.commit = "";
    }
    fprintf(logger->log_file, "EDIT:");
    for (edit = (PyLoggerEdit*)utarray_front(edits);
         edit;
         edit = (PyLoggerEdit*)utarray_next(edits, edit)) {
        fprintf(logger->log_file, "%s -> %s\t", edit->before, edit->after);
    }
    fprintf(logger->log_file, "COMMIT: %s\n", logger->log.commit);
    fflush(logger->log_file);
}

static char*
PyLoggerCommitHook(void *arg, const char *commit)
{
    PyLogger *logger = (PyLogger*)arg;
    if (logger->edited) {
        logger->log.commit = commit;
        PyLoggerWriteLog(logger);
    }
    PyLoggerReset(logger);
    return NULL;
}

static void
PyLoggerResetHook(void *arg)
{
    PyLogger *logger = (PyLogger*)arg;
    PyLoggerReset(logger);
}

static void*
FcitxPyLoggerCreate(FcitxInstance *instance)
{
    PyLogger *logger = fcitx_utils_new(PyLogger);
    FcitxKeyFilterHook key_hook = {
        .arg = logger,
        .func = FcitxPyLoggerPreHook
    };
    FcitxStringFilterHook commit_hook = {
        .arg = logger,
        .func = PyLoggerCommitHook
    };
    FcitxIMEventHook reset_hook = {
        .arg = logger,
        .func = PyLoggerResetHook
    };
    logger->owner = instance;
    logger->busy = false;
    logger->edited = false;

    utarray_init(&logger->log.edit, &pyedit_icd);
    logger->log.commit = NULL;
    logger->log_file = FcitxXDGGetFileUserWithPrefix("pylog", "pyedit.log",
                                                     "a", NULL);

    if (!(logger->log_file && PyLoggerLoadConfig(&logger->config))) {
        FcitxPyLoggerDestroy(logger);
        return NULL;
    }


    FcitxInstanceRegisterPreInputFilter(logger->owner, key_hook);
    FcitxInstanceRegisterCommitFilter(logger->owner, commit_hook);
    FcitxInstanceRegisterResetInputHook(logger->owner, reset_hook);
    return logger;
}

static void
PyLoggerReset(PyLogger *logger)
{
    logger->edited = false;
    utarray_clear(&logger->log.edit);
    logger->log.commit = NULL;
}

static void
FcitxPyLoggerDestroy(void *arg)
{
    PyLogger *logger = (PyLogger*)arg;
    PyLoggerReset(logger);
    fclose(logger->log_file);
    utarray_done(&logger->log.edit);
    free(arg);
}

static void
FcitxPyLoggerReloadConfig(void* arg)
{
    PyLogger *logger = (PyLogger*)arg;
    PyLoggerLoadConfig(&logger->config);
}
