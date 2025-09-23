/*
 * This file is part of SubSail application.
 *
 * Copyright (C) 2025 Jussi Laakkonen <jussi.laakkonen@jolla.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TYPES_H
#define TYPES_H

#include <QString>

// TODO: make c++ class
typedef struct _Subtitle {
    int index;
    int start_time; // In milliseconds
    int end_time;   // In milliseconds
    int start_frame;
    int end_frame;
    QString QText;
    _Subtitle *next;
} Subtitle;

enum SubState {
    SUB_STATE_INIT = 0,
    SUB_STATE_DELAY,
    SUB_STATE_DURATION,
    SUB_STATE_END
};

enum SubParseError {
    SUB_PARSE_ERROR_NONE = 0,
    SUB_PARSE_ERROR_INVALID_INDEX,
    SUB_PARSE_ERROR_INVALID_TIMESTAMP,
    SUB_PARSE_ERROR_INVALID_FILE,
};

#endif // TYPES_H
