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
    Q_INVOKABLE void increaseTime(int time);
    Q_INVOKABLE void setTime(int time);
    Q_INVOKABLE QString getSubtitle(int time_increase);
    Q_INVOKABLE void setOffset(int offset);
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

    static SubtitleEngine* iEngine;

    Parser* iParser;

    QList<Subtitle*> subtitles;
    QString path;
    QString fallback_codec;
    unsigned int current_time;
    unsigned int total_time;
    int time_offset;
    int prev_end_time;
    int delay;
    int duration;
    SubState state;
    int current_index;
};

#endif // SUBTITLEENGINE_H
