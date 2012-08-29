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
#include <libintl.h>

#include "eim.h"

static void *FcitxPyLoggerCreate(FcitxInstance *instance);
static void FcitxPyLoggerDestroy(void *arg);
static void FcitxPyLoggerReloadConfig(void *arg);

FCITX_DEFINE_PLUGIN(fcitx_pinyin_enhance, module, FcitxModule) = {
    .Create = FcitxPyLoggerCreate,
    .Destroy = FcitxPyLoggerDestroy,
    .SetFD = NULL,
    .ProcessEvent = NULL,
    .ReloadConfig = FcitxPyLoggerReloadConfig
};

CONFIG_DEFINE_LOAD_AND_SAVE(PyLogger, PyLoggerConfig, "fcitx-pylogger")

static boolean
FcitxPyLoggerPreHook(void *arg, FcitxKeySym sym, unsigned int state,
                     INPUT_RETURN_VALUE *retval)
{
    PyLogger *logger = (PyLogger*)arg;
    if (!retval)
        return false;
    return false;
}

static void*
FcitxPyLoggerCreate(FcitxInstance *instance)
{
    PyLogger* logger = fcitx_utils_new(PyLogger);
    FcitxKeyFilterHook key_hook = {
        .arg = logger,
        .func = FcitxPyLoggerPreHook,
    };
    logger->owner = instance;

    if (!PyLoggerLoadConfig(&logger->config)) {
        free(logger);
        return NULL;
    }

    FcitxInstanceRegisterPreInputFilter(logger->owner, key_hook);
    return logger;
}

static void
FcitxPyLoggerDestroy(void *arg)
{
    free(arg);
}

static void
FcitxPyLoggerReloadConfig(void* arg)
{
    PyLogger* logger = (PyLogger*)arg;
    PyLoggerLoadConfig(&logger->config);
}
