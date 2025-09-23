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

#include "subparserqt.h"

#include <QRegularExpression>
#include <Qt>

SubParserQt::SubParserQt()
{
    iSubtitleIndex = 0;
}

void SubParserQt::updateFPS(Subtitle *subtitle)
{
    if (!subtitle)
        return;

    subtitle->start_time = frameToTimestampMs(subtitle->start_frame);
    subtitle->end_time = frameToTimestampMs(subtitle->end_frame);
}

bool SubParserQt::needFPSUpdate()
{
    return iFps == 0.0 ? true : false;
}

QString SubParserQt::cleanupText(QString &text)
{
    QRegularExpression replaceControlCode("\\{y:([ibu]+)\\}");
    QRegularExpression removeControlCode(R"(\{[^\}]+\})");
    QRegularExpressionMatch match = replaceControlCode.match(text);
    QString openTags;
    QString closeTags;

    if (match.hasMatch()) {
        QString codes = match.captured(1);

        if (codes.contains('i')) {
           openTags.append("<i>");
           closeTags.prepend("</i>");
        }

        if (codes.contains('b')) {
           openTags.append("<b>");
           closeTags.prepend("</b>");
        }

        if (codes.contains('u')) {
           openTags.append("<u>");
           closeTags.prepend("</u>");
        }
    }

    // Remove all control codes as they can contain colors etc.
    text.remove(removeControlCode);

    // Then add the tags Label can understand to text
    if (match.hasMatch()) {
        text.prepend(openTags);
        text.append(closeTags);
    }

    // And finally appropriate newline markers
    //text = text.trimmed().replace('|', '\n');
    text = text.trimmed().replace("|", QStringLiteral("<br>"));

    return text;
}

Subtitle *SubParserQt::parseSubtitle()
{
    QString text;
    QStringList parts;
    int startTime;
    int endTime;

    if (iInStream->atEnd()) {
        iSubtitleIndex = 0;
        return nullptr;
    }

    QString line = iInStream->readLine().trimmed();
    QRegularExpression rx(R"(\{(\d+)\}\{(\d+)\}(.*))");
    QRegularExpressionMatch match = rx.match(line);

    if (!match.hasMatch()) {
        qDebug() << "failed to process line" << line;
        return nullptr;
    }

    int startFrame = match.captured(1).toInt();
    int endFrame = match.captured(2).toInt();
    text = match.captured(3).trimmed();

    text = cleanupText(text);

    // FPS info
    if (startFrame == 1 && endFrame == 1) {
        qDebug() << "Override previously set FPS" << iFps;
        iFps = text.toDouble();
        qDebug() << "Read FPS from file" << iFps;
        text = QString("");
    }

    // Last one, reset the index counter
    if (text.compare("[END]", Qt::CaseInsensitive) == 0) {
        iSubtitleIndex = 0;
        return nullptr;
    }

    startTime = frameToTimestampMs(startFrame);
    endTime = frameToTimestampMs(endFrame);

    return newSubtitle(++iSubtitleIndex, startTime, endTime, startFrame, endFrame, text);
}

ParserRegistrar<SubParserQt> SubParserQt::registrar("sub");
