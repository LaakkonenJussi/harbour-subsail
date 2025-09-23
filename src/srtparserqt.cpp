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

#include "srtparserqt.h"

#include <QRegularExpression>
#include <QTime>

SrtParserQt::SrtParserQt()
{
}

Subtitle* SrtParserQt::parseSubtitle(enum SubParseError *err)
{
    QString text;
    QStringList parts;
    QTime startTime;
    QTime endTime;
    QRegularExpression controlCode(QStringLiteral(R"(\s*(<(?:i|b|u)>))"));
    bool result;
    int index = -1;
    enum srtReadState state = SRT_READ_INDEX;

    if (iInStream->atEnd())
        return nullptr;

    while (!iInStream->atEnd() && state < SRT_READ_STOP) {
        QString line = iInStream->readLine().trimmed();
        if (line.isEmpty() && state == SRT_READ_INDEX)
            continue;

        switch (state) {
        case SRT_READ_INDEX:
            index = line.toInt(&result);
            if (!result) {
                qDebug() << "invalid index" << line;
                *err = SUB_PARSE_ERROR_INVALID_INDEX;
                return nullptr;
            }

            state = SRT_READ_TIMESTAMP;
            break;
        case SRT_READ_TIMESTAMP:
            parts = line.split(" --> ");
            if (parts.size() != 2) {
                qDebug() << "invalid timestamp" << line;
                *err = SUB_PARSE_ERROR_INVALID_TIMESTAMP;
                return nullptr;
            }

            startTime = timeStrToQTime(parts[0]);
            endTime   = timeStrToQTime(parts[1]);

            state = SRT_READ_TEXT;
            break;
        case SRT_READ_TEXT:
            if (line.isEmpty()) {
                state = SRT_READ_STOP;
                break;
            }

            if (!text.isEmpty())
                text.append(QStringLiteral("<br>").toUtf8());

            // Some srt files can have tags without space, add them
            line = line.replace(controlCode, " \\1").toUtf8();

            // And add appropriate newline markers
            text.append(line.replace("\n", QStringLiteral("<br>")));

            break;
        case SRT_READ_STOP:
            break;
        }
    }

    if (index < 0) {
        qDebug() << "cannot parse subtitle line";
        *err = SUB_PARSE_ERROR_INVALID_FILE;
        return nullptr;
    }

    return newSubtitle(index, startTime, endTime, text);
}

void SrtParserQt::updateFPS(Subtitle *subtitle)
{
    if (!subtitle) // Avoid warn
        return;
    return;
}

ParserRegistrar<SrtParserQt> SrtParserQt::registrar("srt");

