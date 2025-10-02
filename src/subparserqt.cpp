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
#include <QTime>
#include <Qt>

SubParserQt::SubParserQt()
{
    iSubtitleIndex = 0;
    iTimeStampPattern = QString("hh:mm:ss.z");
    iType = SUB_TYPE_UNSET;
    iNeedFPSUpdate = true;
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
    return iFps == 0.0 && iNeedFPSUpdate ? true : false;
}

void SubParserQt::initializeParser()
{
    iType = SUB_TYPE_UNSET;
    iNeedFPSUpdate = true;
}

subType SubParserQt::checkSubtitleType(QString &firstLine)
{
    QChar c;

    if (firstLine.isEmpty())
        return SUB_TYPE_UNSET;

    c = firstLine.at(0);

    switch (c.toLatin1()) {
    case '{':
        qDebug() << "MicroDVD subtitle";
        return SUB_TYPE_MICRODVD;
    case '[':
        qDebug() << "SubViewer 2.0 subtitle";
        iNeedFPSUpdate = false;
        return SUB_TYPE_SUBVIEWER;
    default:
        if (c.isDigit()) {
            qDebug() << "SubViewer 2.0 subtitle (assumed)";
            iNeedFPSUpdate = false;
            return SUB_TYPE_SUBVIEWER;
        }

        qDebug() << "unknown subtitle file";
        return SUB_TYPE_UNSET;
    }
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

Subtitle *SubParserQt::parseMicroDVD(QString &line, SubParseError *err)
{
    QRegularExpression regexMicroDVD(R"(\{(\d+(?:\.\d+)?)\}\{(\d+(?:\.\d+)?)\}(.*))");
    QString text;
    QStringList parts;
    int startTime;
    int endTime;

    QRegularExpressionMatch match = regexMicroDVD.match(line);

    if (!match.hasMatch()) {
        qDebug() << "failed to process line" << line;
        *err = SUB_PARSE_ERROR_INVALID_FILE;
        return nullptr;
    }

    // Some .sub files can have non-standard frames as floats caused by conversion
    int startFrame = static_cast<int>(match.captured(1).toFloat());
    int endFrame = static_cast<int>(match.captured(2).toFloat());
    text = match.captured(3).trimmed();

    text = cleanupText(text);

    if (!startFrame || !endFrame) {
        qDebug() << "Failed to parse frames on line:" << line;
        *err = SUB_PARSE_ERROR_INVALID_TIMESTAMP;
        return nullptr;
    }

    // FPS info
    if (startFrame == 1 && endFrame == 1) {
        qDebug() << "Override previously set FPS" << iFps;
        iFps = text.toDouble();
        iNeedFPSUpdate = false;

        qDebug() << "Read FPS from file" << iFps;
        return nullptr;
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

Subtitle *SubParserQt::parseSubtitleViewer(QString &line,SubParseError *err)
{
    QStringList parts;
    QString textLine;
    QString text;
    QTime startTime;
    QTime endTime;

    /* Ignore all lines starting with tags */
    if (line.at(0).toLatin1() == '[') {
        qDebug() << "ignoring tag" << line;
        *err = SUB_PARSE_ERROR_NONE;
        return nullptr;
    }

    parts = line.split(",");
    if (parts.size() != 2) {
        qDebug() << "invalid timestamp" << line;
        *err = SUB_PARSE_ERROR_INVALID_TIMESTAMP;
        return nullptr;
    }

    startTime = timeStrToQTime(parts[0]);
    endTime = timeStrToQTime(parts[1]);

    while (!iInStream->atEnd()) {
        textLine = iInStream->readLine().trimmed();

        if (textLine.isEmpty())
            break;

        if (!text.isEmpty())
            text.append(QStringLiteral("<br>").toUtf8());

        text.append(textLine.replace("[br]", QStringLiteral("<br>")));
    }

    return newSubtitle(++iSubtitleIndex, startTime, endTime, text);
}

Subtitle *SubParserQt::parseSubtitle(enum SubParseError *err)
{
    *err = SUB_PARSE_ERROR_NONE;

    if (iInStream->atEnd()) {
        iSubtitleIndex = 0;
        *err = SUB_PARSE_ERROR_EOF;
        return nullptr;
    }

    QString line = iInStream->readLine().trimmed();
    if (line.isEmpty() || line == QStringLiteral("\n")) {
        qDebug() << "skip empty line";
        return nullptr; // noerror
    }

    /* Should be done only once */
    if (iType == SUB_TYPE_UNSET)
        iType = checkSubtitleType(line);

    switch (iType) {
    case SUB_TYPE_UNSET:
        qWarning() << "cannot detect .sub file type";
        *err = SUB_PARSE_ERROR_INVALID_FILE;
        break;
    case SUB_TYPE_MICRODVD:
        return parseMicroDVD(line, err);
    case SUB_TYPE_SUBVIEWER:
        return parseSubtitleViewer(line, err);
    }

    return nullptr;
}

ParserRegistrar<SubParserQt> SubParserQt::registrar("sub");
