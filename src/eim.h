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

#ifndef EIM_H
#define EIM_H

#include <fcitx/ime.h>
#include <fcitx/instance.h>

typedef struct {
    FcitxGenericConfig gconfig;
} PyLoggerConfig;

typedef struct {
    char *before;
    char *after;
} PyLoggerEdit;

typedef struct {
    UT_array edit;
    const char *commit;
    char *raw_buff;
} PyLoggerLog;

typedef struct {
    PyLoggerConfig config;
    FcitxInstance* owner;
    boolean busy;
    boolean edited;
    PyLoggerLog log;
    FILE *log_file;
} PyLogger;

CONFIG_BINDING_DECLARE(PyLoggerConfig);

#endif
