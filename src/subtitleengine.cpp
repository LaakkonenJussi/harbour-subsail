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

#include "subtitleengine.h"
#include "parserenginefactory.h"

#include <math.h>

#include <QFileInfo>
#include <QTextCodec>

void SubtitleEngine::setupSubtitles()
{
    Subtitle *first = nullptr;

    if (iSubtitles.empty())
        return;

    qDebug() << iSubtitles.size() << "subtitle lines processed";

    first = iSubtitles.first();

    iTotalTime = iSubtitles.last()->end_time;
    qDebug() << "total duration" << iTotalTime << "ms";
    qDebug() << "start" << first->index << "time" << first->start_time;

    iCurrentIndex = 0;
    iState = SUB_STATE_DELAY;
    iPrevEndTime = 0;
    first->start_time += iTimeOffset;
    first->end_time += iTimeOffset;
    iDelay = first->start_time;
}

static QString parseErrorToStr(enum SubParseError err)
{
    switch (err) {
    case SUB_PARSE_ERROR_NONE:
    case SUB_PARSE_ERROR_EOF:
        return QString("No error");
    case SUB_PARSE_ERROR_INVALID_INDEX:
        return QString("Invalid indexes");
    case SUB_PARSE_ERROR_INVALID_TIMESTAMP:
        return QString("Invalid timestamps");
    case SUB_PARSE_ERROR_INVALID_FILE:
        return QString("Invalid file");
    }

    return QString("");
}

SubtitleEngine::SubtitleLoadStatus SubtitleEngine::loadSubtitle(QString file)
{
    Subtitle *newsub;
    enum SubParseError parseErr = SUB_PARSE_ERROR_NONE;

    qDebug() << "load " << file << "";

    freeSubtitles();

    QFileInfo fileInfo(file);
    QString suffix = fileInfo.suffix();

    iParser = ParserEngineFactory::instance().getEngine(suffix);
    if (!iParser) {
        qWarning() << "no parser available for type" << suffix;
        return SUBTITLE_LOAD_STATUS_NOT_SUPPORTED;
    }

    iParser->initializeParser();
    iParser->setFallbackCodec(iFallbackCodec);

    int err = iParser->openSubtitle(file);
    switch (err) {
    case -ENOTSUP:
        qWarning() << "cannot open subtitle" << strerror(-err);
        return SUBTITLE_LOAD_STATUS_FAILURE;
    case -ENOENT:
        qWarning() << "cannot open subtitle" << strerror(-err);
        return SUBTITLE_LOAD_STATUS_FILE_NOT_FOUND;
    case -EACCES:
        qWarning() << "cannot open subtitle" << strerror(-err);
        return SUBTITLE_LOAD_STATUS_ACCESS_DENIED;
    case 0:
        break;
    }

    do {
        newsub = iParser->loadSubtitle(&parseErr);
        if (newsub) /* Skip empty lines */
            iSubtitles.append(newsub);
    } while (parseErr == SUB_PARSE_ERROR_NONE);

    iParser->closeSubtitle();

    switch (parseErr) {
    case SUB_PARSE_ERROR_NONE:
    case SUB_PARSE_ERROR_EOF:
        break;
    case SUB_PARSE_ERROR_INVALID_INDEX:
    case SUB_PARSE_ERROR_INVALID_TIMESTAMP:
    case SUB_PARSE_ERROR_INVALID_FILE:
        qWarning() << "cannot parse subtitle file" << parseErrorToStr(parseErr);
        return SUBTITLE_LOAD_STATUS_PARSE_FAILURE;
    }

    setupSubtitles();

    if (iParser->needFPSUpdate())
        return SUBTITLE_LOAD_STATUS_OK_NEED_FPS;

    return SUBTITLE_LOAD_STATUS_OK;
}

void SubtitleEngine::unloadSubtitle()
{
    qDebug() << "unload subtitle and reset";
    freeSubtitles();
}

void SubtitleEngine::updateFps(double fps)
{
    Subtitle *subtitle;

    if (!iParser)
        return;

    qDebug() << "updating FPS to" << fps;
    iParser->setFps(fps);

    foreach (subtitle, iSubtitles)
        iParser->updateFPS(subtitle);

    // Update time from the last one
    iTotalTime = iSubtitles.last()->end_time;
    qDebug() << "total time updated to" << iTotalTime;
}

void SubtitleEngine::increaseTime(unsigned int time)
{
    iCurrentTime += time;

    switch(iState) {
    case SUB_STATE_INIT:
        break;
    case SUB_STATE_DELAY:
        iDelay -= time;
        break;
    case SUB_STATE_DURATION:
        iDuration -= time;
        break;
    case SUB_STATE_END:
        iDuration = 0;
        iDelay = 0;
        break;
    default:
        break;
    }
}

int SubtitleEngine::findPosition(unsigned int time, int min, int max) {
    Subtitle *tmp;
    double halfpoint = (max - min) / 2;
    int half = min + floor(halfpoint);

    if (min >= max)
        return half;

    tmp = iSubtitles.at(half);

    if (tmp->start_time < time && tmp->end_time < time)
        return findPosition(time, half + 1, max);
    else if (tmp->start_time < time && tmp->end_time > time)
        return half;
    else
        return findPosition(time, min, half - 1);
}

void SubtitleEngine::setTime(unsigned int time)
{
    Subtitle *tmp = nullptr;
    int position = 0;
    int size;
    int diff;

    size = iSubtitles.size();

    if (time) {
        position = findPosition(time, 0, size - 1);
        if (position > 0)
            /* Just to be sure in case the set time overlaps */
            --position;
    }

    iCurrentTime = time;

    for (; position < size ; position++) {

        tmp = iSubtitles.at(position);

        // delay
        if (tmp->start_time >= time && tmp->end_time <= time) {
            iState = SUB_STATE_DELAY;
            iDelay = tmp->start_time - time;
            break;
        }

        // duration
        if (tmp->start_time <= time && tmp->end_time >= time) {
            iState = SUB_STATE_DURATION;
            iDuration = tmp->end_time - time;
            break;
        }

        if (tmp->start_time <= time && tmp->end_time > time &&
                ((position + 1) < size && iSubtitles.at(position + 1)->start_time > time)) {
            iState = SUB_STATE_DURATION;
            diff = time - tmp->start_time;
            iDuration = tmp->end_time - tmp->start_time + diff;
            break;
        }

        if (tmp->start_time >= time) {
            iState = SUB_STATE_DELAY;
            iDelay = tmp->start_time - time;
            break;
        }
    }

    if (tmp)
        iCurrentIndex = tmp->index - 1; // Subtitles start from 1
    else
        qDebug() << "not found, tmp null";

    return;
}

QString SubtitleEngine::getSubtitle(unsigned int time)
{
    Subtitle *current;

    if (!iParser)
        return QString("no parser");

    current = getSubtitleNow();
    if (!current)
        return QString("<subtitles end>");

    if (time)
        increaseTime(time);

    switch(iState) {
    case SUB_STATE_INIT:
        break;
    case SUB_STATE_DELAY:
        if (iDelay <= 0) {
            iState = SUB_STATE_DURATION;
            iDuration = current->end_time - current->start_time;
            iDelay = 0;
            return iParser->getSubtitleText(current);
        } else {
            return QString("");
        }

        break;
    case SUB_STATE_DURATION:
        if (iDuration <= 0) {
            iDuration = 0;
            iPrevEndTime = current->end_time;

            if (iCurrentIndex + 1 >= iSubtitles.size()) {
                iState = SUB_STATE_END;
                return QString("<subtitles end>");
            }

            iCurrentIndex++;
            current = getSubtitleNow();
            if (!current) {
                iState = SUB_STATE_END;
                return QString("<subtitles end>");
            }

            current->start_time += iTimeOffset;
            current->end_time += iTimeOffset;

            if (iPrevEndTime >= 0 && current->start_time >= iPrevEndTime) {
                iDelay = current->start_time - iPrevEndTime;
                iState = SUB_STATE_DELAY;
            }

            return QString("");
        }

        return iParser->getSubtitleText(current);
    case SUB_STATE_END:
        iDuration = 0;
        iDelay = 0;
    default:
        break;
    }

    return QString("");
}

void SubtitleEngine::setOffset(int offset)
{
    iTimeOffset = offset;
}

unsigned int SubtitleEngine::getTotalTime()
{
    return iTotalTime;
}

int SubtitleEngine::setFallbackCodec(const QString fallbackCodec)
{
    if (fallbackCodec.isEmpty())
        return -ENOENT;

    QTextCodec* codec = QTextCodec::codecForName(fallbackCodec.toStdString().c_str());
    if (!codec) {
        qDebug() << "invalid fallback codec name" << fallbackCodec;
        return -EINVAL;
    }

    if (iFallbackCodec == fallbackCodec) {
        qDebug() << "codec not changed";
        return -EALREADY;
    }


    iFallbackCodec = QString(fallbackCodec);
    qDebug() << "new codec set" << iFallbackCodec;

    return 0;
}

QString SubtitleEngine::getFallbackCodec()
{
    return iFallbackCodec;
}

void SubtitleEngine::freeSubtitles()
{
    Subtitle *subtitle;

    if (iSubtitles.empty())
        return;

    qDebug() << "free subtitle list";

    if (iParser) {
        foreach (subtitle, iSubtitles)
            iParser->freeSubtitle(subtitle);
    }

    iSubtitles.clear();

    resetEngine();
}

void SubtitleEngine::resetEngine()
{
    iCurrentIndex = -1;
    iParser = nullptr;

    iCurrentTime = 0;
    iTotalTime = 0;
    iPrevEndTime = 0;
    iDelay = 0;
    iDuration = 0;
    iState = SUB_STATE_INIT;
    iTimeOffset = 0;

    if (iFallbackCodec.isEmpty())
        iFallbackCodec = QString("Windows-1252");
}

Subtitle *SubtitleEngine::getSubtitleNow()
{
    if (iCurrentIndex < 0 || iCurrentIndex > iSubtitles.size() - 1)
        return nullptr;

    return iSubtitles.at(iCurrentIndex);
}

SubtitleEngine* SubtitleEngine::iEngine = nullptr;

SubtitleEngine::SubtitleEngine(QObject *parent) :
    QObject(parent)
{
    qDebug() << "engine init";

    resetEngine();
}


SubtitleEngine* SubtitleEngine::initEngine() {
    if (!iEngine)
        iEngine = new SubtitleEngine;

    return iEngine;
}

SubtitleEngine::~SubtitleEngine()
{
    qDebug() << "engine die";

    freeSubtitles();
}
