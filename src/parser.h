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

#ifndef PARSER_H
#define PARSER_H

#include <QString>
#include <QTextStream>
#include <QFile>
#include <QtDebug>
#include "types.h"

class Parser
{
public:
    int openSubtitle(const QString &filePath);
    Subtitle *loadSubtitle(enum SubParseError *err);
    void closeSubtitle();
    QString getSubtitleText(Subtitle *subtitle);
    Subtitle *newSubtitle(int index,
                          unsigned int startTime,
                          unsigned int endTime,
                          const QString &text);
    Subtitle *newSubtitle(int index,
                          unsigned int startTime,
                          unsigned int endTime,
                          unsigned int startFrame,
                          unsigned int endFrame,
                          const QString &text);
    Subtitle *newSubtitle(int index,
                          const QTime &startTime,
                          const QTime &endTime,
                          const QString &text);
    void freeSubtitle(Subtitle *subtitle);
    void setFps(double fps);
    void setFallbackCodec(const QString &fallbackCodec);

    virtual Subtitle *parseSubtitle(enum SubParseError *err) = 0;
    virtual void updateFPS(Subtitle *subtitle) = 0;
    virtual bool needFPSUpdate() = 0;
    virtual void initializeParser() = 0;

    Parser();
    ~Parser();

protected:
    QTextCodec *detectEncoding(QFile* file);
    bool checkFileMIME(const QString &filepath);
    QTime timeStrToQTime(const QString &str);
    unsigned int frameToTimestampMs(const unsigned int frame);

    QFile* iSubfile;
    QTextStream* iInStream;
    double iFps;
    QString iFallbackCodec;
    QString iTimeStampPattern;

private:
    unsigned int timestampToMs(const char *timestamp);
    unsigned int timestampToMs(QTime &time);
    QTextCodec *useFallbackCodec();
};

#endif // PARSER_H
