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

#include "parser.h"
#include "parserenginefactory.h"

#include <QMimeType>
#include <QMimeDatabase>
#include <QTextCodec>
#include <QTime>

Parser::Parser()
{
    iSubfile = nullptr;
    iInStream = nullptr;
    iFps = 0.0;
}

Parser::~Parser()
{
    if (iInStream)
        delete(iInStream);

    if (iSubfile) {
        if (iSubfile->isOpen())
            iSubfile->close();

        delete(iSubfile);
    }

}
QTextCodec *Parser::useFallbackCodec()
{
    qDebug() << "using fallback codec" << iFallbackCodec;
    return QTextCodec::codecForName(iFallbackCodec.toStdString().c_str());
}

QTextCodec* Parser::detectEncoding(QFile* file)
{
    QByteArray bom;

    if (!file)
        return useFallbackCodec();

    bom = file->peek(4);  // read first up to 4 bytes = BOM

    if (bom.startsWith("\xEF\xBB\xBF")) {
        qDebug() << "using UTF-8 codec";
        return QTextCodec::codecForName("UTF-8");
    } else if (bom.startsWith("\xFF\xFE")) {
        qDebug() << "using UTF-16 LE codec";
        return QTextCodec::codecForName("UTF-16LE");
    } else if (bom.startsWith("\xFF\xFE\x00\x00")) {
        qDebug() << "using UTF-32 LE codec";
        return QTextCodec::codecForName("UTF-32LE");
    }

    // No BOM → fallback
    return useFallbackCodec();
}

bool Parser::checkFileMIME(const QString &filepath)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(filepath, QMimeDatabase::MatchContent);
    QStringList list = mime.parentMimeTypes();
    bool plaintext = false;

    for (int i = 0; i < list.size(); i++) {
        qDebug() << "mimetype:" << list.at(i);
        QString mimeTypeName = QString(list.at(i));

        if (mimeTypeName == "text/plain" ||
                mimeTypeName == QStringLiteral("application/x-subrip") ||
                mimeTypeName == QStringLiteral("application/x-subviewer") ||
                // Some .sub files with converters are detected as octet-stream
                mimeTypeName == QStringLiteral("application/octet-stream"))
            plaintext = true;
    }

    return plaintext;
}

QTime Parser::timeStrToQTime(const QString &str)
{
    return QTime::fromString(str, iTimeStampPattern);
}

unsigned int Parser::timestampToMs(const char *timestamp)
{
    int h, m, s, ms;

    if (sscanf(timestamp, "%d:%d:%d,%d", &h, &m, &s, &ms) != 4) {
        qDebug() << "invalid timestamp format:" << timestamp;
        return 0;
    }

    // Allow normal ranges for ms, s, m, limit hours to 100 - could be lower, though.
    if (ms > 999 || ms < 0 || s > 59 || s < 0 || m > 59 || m < 0 || h < 0 || h > 100) {
        qDebug() << "invalid timestamp values:" << timestamp;
        return 0;
    }

    return static_cast<unsigned int>((h * 3600 + m * 60 + s) * 1000 + ms);
}

unsigned int Parser::timestampToMs(QTime &time)
{
    return timestampToMs(time.toString("hh:mm:ss,zzz").toStdString().c_str());
}

unsigned int Parser::frameToTimestampMs(const unsigned int frame)
{
    if (iFps <= 0.0)
        return 0;

    return static_cast<unsigned int>((frame / iFps) * 1000.0);
}

int Parser::openSubtitle(const QString &filePath)
{
    QTextCodec *codec;

    if (!checkFileMIME(filePath)) {
        qDebug() << "cannot use file";
        return -ENOTSUP;
    }

    iSubfile = new QFile(filePath);
    if (!iSubfile->exists()) {
        qDebug() << "file %s does not exist" << filePath;
        return -ENOENT;
    }

    if (!iSubfile->open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error opening file";
        return -EACCES;
    }

    codec = Parser::detectEncoding(iSubfile);
    iInStream = new QTextStream(iSubfile);

    if (codec)
        iInStream->setCodec(codec);

    return 0;
}

Subtitle *Parser::loadSubtitle(enum SubParseError *err)
{
    Subtitle *newsub = nullptr;

    if (!iSubfile) {
        qDebug() << "subtitle file not set";
        *err = SUB_PARSE_ERROR_NO_FILE;
        return nullptr;
    }

    if (iSubfile->isOpen() && iSubfile->isReadable())
        newsub = parseSubtitle(err);

    return newsub;
}

void Parser::closeSubtitle()
{
    if (iSubfile && iSubfile->isOpen())
        iSubfile->close();
}

QString Parser::getSubtitleText(Subtitle *subtitle)
{
    if (!subtitle || subtitle->text.isEmpty())
        return QString("");

    return subtitle->text;
}

Subtitle *Parser::newSubtitle(int index, unsigned int startTime,
                              unsigned int endTime,
                              const QString &text)
{
    Subtitle *sub = new Subtitle();

    sub->index = index;
    sub->start_time = startTime;
    sub->end_time = endTime;
    sub->text = text;

    return sub;
}

Subtitle *Parser::newSubtitle(int index, unsigned int startTime,
                              unsigned int endTime, unsigned int startFrame,
                              unsigned int endFrame,  const QString &text)
{
    Subtitle *sub = newSubtitle(index, startTime, endTime, text);

    sub->start_frame = startFrame;
    sub->end_frame = endFrame;

    return sub;
}

Subtitle* Parser::newSubtitle(int index, const QTime &startTime,
                                   const QTime &endTime, const QString &text)
{
    unsigned int start_time = timestampToMs(const_cast<QTime&>(startTime));
    unsigned int end_time = timestampToMs(const_cast<QTime&>(endTime));

    return newSubtitle(index, start_time, end_time, text);
}

void Parser::freeSubtitle(Subtitle *subtitle)
{
    if (!subtitle)
        return;

    delete subtitle;
}

void Parser::setFps(double fps)
{
    iFps = fps;
}

void Parser::setFallbackCodec(const QString &fallbackCodec)
{
    iFallbackCodec = QString(fallbackCodec);
}
