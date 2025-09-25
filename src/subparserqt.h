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

#ifndef SUBPARSERQT_H
#define SUBPARSERQT_H

#include "parser.h"
#include "parserenginefactory.h"

class SubParserQt : public Parser
{
public:
    SubParserQt();
    Subtitle *parseSubtitle(enum SubParseError *err);
    void updateFPS(Subtitle *subtitle);
    bool needFPSUpdate();

private:
    int iSubtitleIndex;

    QString cleanupText(QString &text);

    static ParserRegistrar<SubParserQt> registrar;
};

#endif // SUBPARSERQT_H
