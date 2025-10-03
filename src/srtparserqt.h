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

#ifndef SRTPARSERQT_H
#define SRTPARSERQT_H

#include "parser.h"
#include "parserenginefactory.h"

enum srtReadState {
    SRT_READ_INDEX = 0,
    SRT_READ_TIMESTAMP,
    SRT_READ_TEXT,
    SRT_READ_STOP
};

class SrtParserQt : public Parser
{
public:
    SrtParserQt();

    // Parser interface
public:
    Subtitle *parseSubtitle(enum SubParseError *err);
    void updateFPS(Subtitle *subtitle);
    bool needFPSUpdate() { return false; };
    void initializeParser() { return; };

private:
    static ParserRegistrar<SrtParserQt> registrar;

};

#endif // SRTPARSERQT_H
