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
    iPrevEndTime = 0;

    if (iTimeOffset < 0) {
        iState = SUB_STATE_INIT_DELAY;
        iInitDelay = -iTimeOffset;
        iDelay = 0;
    } else {
        iState = SUB_STATE_DELAY;
        iDelay = getSubtitleStart(first);
    }

    iPrevEndTime = 0;
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
    case SUB_PARSE_ERROR_NO_FILE:
        return QString("No file");
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
    case SUB_PARSE_ERROR_NO_FILE:
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
    iTotalTime = iSubtitles.isEmpty() ? 0 : iSubtitles.last()->end_time;
    qDebug() << "total time updated to" << iTotalTime;
}

void SubtitleEngine::increaseTime(unsigned int time)
{
    iCurrentTime += time;

    switch(iState) {
    case SUB_STATE_INIT:
        break;
    case SUB_STATE_INIT_DELAY:
        if (time >= iInitDelay) {
            time -= iInitDelay;
            iInitDelay = 0;
            iState = SUB_STATE_DELAY;
            if (!iSubtitles.isEmpty())
                setEngineSubTime(getSubtitleStart(iSubtitles.first()));

            increaseTime(time);
            break;
        }

        iInitDelay -= time;
        break;
    case SUB_STATE_DELAY:
        if (time >= iDelay) {
            time -= iDelay;
            iDelay = 0;
            iState = SUB_STATE_DURATION;
            iDuration = calcCurrentDuration();
            increaseTime(time);
            break;
        }

        setEngineSubTime(iDelay - time);
        break;
    case SUB_STATE_DURATION:
        if (time >= iDuration) {
            time -= iDuration;
            iDuration = 0;
            iPrevEndTime = getSubtitleEnd(getSubtitleNow());

            if (iCurrentIndex + 1 < iSubtitles.size()) {
                iCurrentIndex++;
                unsigned int nextStart = getSubtitleStart(getSubtitleNow());
                if (nextStart > iCurrentTime) {
                    iState = SUB_STATE_DELAY;
                    setEngineSubTime(calcCurrentDelay());
                } else {
                    iState = SUB_STATE_DURATION;
                    setEngineSubTime(calcCurrentDuration());
                }

                increaseTime(time);
                break;
            }

            iState = SUB_STATE_END;
            break;
        }

        setEngineSubTime(iDuration - time);
        break;
    case SUB_STATE_END:
        iDuration = 0;
        iDelay = 0;
        break;
    default:
        break;
    }
}

void SubtitleEngine::setEngineSubTime(unsigned int time)
{
    switch (iState) {
    case SUB_STATE_INIT:
    case SUB_STATE_INIT_DELAY:
        iInitDelay = time;
        break;
    case SUB_STATE_DELAY:
        iDelay = time;
        break;
    case SUB_STATE_DURATION:
        iDuration = time;
        break;
    case SUB_STATE_END:
        break;
    default:
        break;
    }
}

static unsigned int getUnsigned(int value, bool *add)
{
    if (value < 0) {
        *add = false;

        if (value == INT_MIN)
            return static_cast<unsigned int>(static_cast<unsigned int>(INT_MAX) + 1u);

        return static_cast<unsigned int>(-value);
    } else {
        *add = true;
        return static_cast<unsigned int>(value);
    }
}

unsigned int SubtitleEngine::getSubtitleStart(Subtitle *subtitle)
{
    if (!subtitle)
        return 0;

    if (iTimeOffsetAdd) {
        return subtitle->start_time + iTimeOffsetUnsigned;
    } else {
        // In case the subtraction would undeflow.
        if (subtitle->start_time <= iTimeOffsetUnsigned)
            return 0;

        return subtitle->start_time - iTimeOffsetUnsigned;
    }
}

unsigned int SubtitleEngine::getSubtitleStart(int position)
{
    if (position < 0 || position >= iSubtitles.size())
        return 0;

    return getSubtitleStart(iSubtitles.at(position));
}

unsigned int SubtitleEngine::getSubtitleEnd(Subtitle *subtitle)
{
    if (!subtitle)
        return 0;

    if (iTimeOffsetAdd) {
        return subtitle->end_time + iTimeOffsetUnsigned;
    } else {
        // In case the subtraction would undeflow.
        if (subtitle->end_time <= iTimeOffsetUnsigned)
            return 0;

        return subtitle->end_time - iTimeOffsetUnsigned;
    }
}

unsigned int SubtitleEngine::getSubtitleEnd(int position)
{
    if (position < 0 || position >= iSubtitles.size())
        return 0;

    return getSubtitleEnd(iSubtitles.at(position));
}

unsigned int SubtitleEngine::calcCurrentDuration()
{
    Subtitle *current = getSubtitleNow();
    if (!current)
        return 0;

    return getSubtitleEnd(current) - getSubtitleStart(current);
}

unsigned int SubtitleEngine::calcCurrentDelay()
{
    Subtitle *current = getSubtitleNow();
    if (!current)
        return 0;

    unsigned int start = getSubtitleStart(current);
    if (start <= iPrevEndTime)
        return 0;

    return start - iPrevEndTime;
}

int SubtitleEngine::findPosition(unsigned int time, int min, int max) {
    unsigned int start_time;
    unsigned int end_time;
    int mid;

    if (min < 0)
        min = 0;

    if (max >= static_cast<int>(iSubtitles.size()))
        max = static_cast<int>(iSubtitles.size()) - 1;

    if (min >= max)
        return min;

    mid = min + (max - min) / 2;
    start_time = getSubtitleStart(mid);
    end_time = getSubtitleEnd(mid);

    if (start_time < time && end_time < time)
        return findPosition(time, mid + 1, max);
    else if (start_time < time && end_time > time)
        return mid;
    else
        return findPosition(time, min, mid - 1);
}

void SubtitleEngine::setTime(unsigned int time)
{
    Subtitle *tmp = nullptr;
    int position = 0;
    int size;

    if (iSubtitles.isEmpty())
        return;

    size = iSubtitles.size();
    iCurrentTime = time;

    if (iTimeOffset < 0 && time <= static_cast<unsigned int>(INT_MAX)) {
        int signedTime = static_cast<int>(time);

        if (signedTime + iTimeOffset < 0) {
            iState = SUB_STATE_INIT_DELAY;
            iInitDelay = -iTimeOffset - signedTime;
            qDebug() << "set init delay with offset" << iTimeOffset << "to" << iInitDelay;
            return;
        }
    }

    if (time) {
        position = findPosition(time, 0, size - 1);
        if (position > 0)
            /* Just to be sure in case the set time overlaps */
            position--;
    }

    for (; position < size ; position++) {
        tmp = iSubtitles.at(position);
        unsigned int start_time = getSubtitleStart(tmp);
        unsigned int end_time = getSubtitleEnd(tmp);

        // delay
        if (start_time > time) {
            iState = SUB_STATE_DELAY;
            setEngineSubTime(start_time - time);
            iPrevEndTime = 0;
            break;
        }

        // duration
        if (start_time <= time && end_time >= time) {
            iState = SUB_STATE_DURATION;
            setEngineSubTime(end_time - time);
            iPrevEndTime = end_time;
            break;
        }

        /*if (start_time <= time && end_time > time &&
                ((position + 1) < size && getSubtitleStart(position + 1) > time)) {
            iState = SUB_STATE_DURATION;
            setEngineSubTime(end_time - (time - start_time));
            iPrevEndTime = end_time;
            break;
        }*/
    }

    if (tmp)
        iCurrentIndex = tmp->index - 1; // Subtitles start from 1
    else
        qDebug() << "not found, tmp null";

    return;
}

QString SubtitleEngine::getSubtitle(unsigned int time)
{
    if (!iParser)
        return QString("no parser");

    if (!iSubtitles.size())
        return QString("<subtitles end>");

    if (time)
        increaseTime(time);

    Subtitle* current = getSubtitleNow();
    if (!current)
        return QString("<subtitles end>");

    switch(iState) {
    case SUB_STATE_INIT:
    case SUB_STATE_INIT_DELAY:
    case SUB_STATE_DELAY:
        return QString("");
    case SUB_STATE_DURATION:
        return iParser->getSubtitleText(current);
    case SUB_STATE_END:
        return QString("<subtitles end>");
    default:
        return QString("");
    }
}

bool SubtitleEngine::setOffset(int offset)
{
    int diff = iTimeOffset - offset;
    iTimeOffset = offset;

    iTimeOffsetUnsigned = getUnsigned(iTimeOffset, &iTimeOffsetAdd);

    // TODO possible int overflow...
    if (iPrevEndTime < static_cast<unsigned int>(INT_MAX) &&
                static_cast<int>(iPrevEndTime) + diff < 0)
        iPrevEndTime = 0;
    else if (iTimeOffsetAdd)
        iPrevEndTime += iTimeOffsetUnsigned;
    else if (iPrevEndTime)
        iPrevEndTime -= iTimeOffsetUnsigned;

    setTime(iCurrentTime);

    return true;
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
    iTimeOffsetUnsigned = 0;
    iTimeOffsetAdd = true;

    if (iFallbackCodec.isEmpty())
        iFallbackCodec = QString("Windows-1252");
}

Subtitle *SubtitleEngine::getSubtitleNow()
{
    if (iCurrentIndex < 0 || iSubtitles.isEmpty() || iCurrentIndex > iSubtitles.size() - 1)
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
