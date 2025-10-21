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

#ifndef SUBTITLEENGINE_H
#define SUBTITLEENGINE_H

#include <QObject>
#include "types.h"
#include "parser.h"
#include "parserenginefactory.h"

class SubtitleEngine : public QObject
{
    Q_OBJECT
public:
    SubtitleEngine(QObject *parent = nullptr);

    enum SubtitleLoadStatus {
        SUBTITLE_LOAD_STATUS_OK = 0,
        SUBTITLE_LOAD_STATUS_OK_NEED_FPS,
        SUBTITLE_LOAD_STATUS_FILE_NOT_FOUND,
        SUBTITLE_LOAD_STATUS_ACCESS_DENIED,
        SUBTITLE_LOAD_STATUS_NOT_SUPPORTED,
        SUBTITLE_LOAD_STATUS_PARSE_FAILURE,
        SUBTITLE_LOAD_STATUS_FAILURE
    };
    Q_ENUM(SubtitleLoadStatus);

    Q_INVOKABLE SubtitleEngine::SubtitleLoadStatus loadSubtitle(QString str);
    Q_INVOKABLE void unloadSubtitle();
    Q_INVOKABLE void updateFps(double fps);
    Q_INVOKABLE void increaseTime(unsigned int time);
    Q_INVOKABLE void setTime(unsigned int time);
    Q_INVOKABLE QString getSubtitle(unsigned int time_increase);
    Q_INVOKABLE bool setOffset(int offset);
    Q_INVOKABLE static SubtitleEngine* initEngine();
    Q_INVOKABLE unsigned int getTotalTime();
    Q_INVOKABLE int setFallbackCodec(const QString fallbackCodec);
    Q_INVOKABLE QString getFallbackCodec();

    ~SubtitleEngine();

private:
    void freeSubtitles(void);
    void setupSubtitles();
    void resetEngine();
    Subtitle *getSubtitleNow();
    int findPosition(unsigned int time, int min, int max);
    void setEngineSubTime(unsigned int time);
    unsigned int getSubtitleStart(Subtitle *subtitle);
    unsigned int getSubtitleStart(int position);
    unsigned int getSubtitleEnd(Subtitle *subtitle);
    unsigned int getSubtitleEnd(int position);
    unsigned int calcCurrentDuration();
    unsigned int calcCurrentDelay();

    static SubtitleEngine* iEngine;

    Parser* iParser;

    QList<Subtitle*> iSubtitles;
    QString iPath;
    QString iFallbackCodec;
    unsigned int iCurrentTime;
    unsigned int iTotalTime;
    int iTimeOffset;
    unsigned int iTimeOffsetUnsigned;
    bool iTimeOffsetAdd;
    unsigned int iPrevEndTime;
    unsigned int iInitDelay;
    unsigned int iDelay;
    unsigned int iDuration;
    SubState iState;
    int iCurrentIndex;
};

#endif // SUBTITLEENGINE_H
