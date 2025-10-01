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

#include <QFileInfo>
#include <QTextCodec>

void SubtitleEngine::setupSubtitles()
{
    Subtitle *first = nullptr;

    if (subtitles.empty())
        return;

    qDebug() << subtitles.size() << "subtitle lines processed";

    first = subtitles.first();

    total_time = subtitles.last()->end_time;
    qDebug() << "total duration" << total_time << "ms";
    qDebug() << "start" << first->index << "time" << first->start_time;

    current_index = 0;
    state = SUB_STATE_DELAY;
    prev_end_time = 0;
    first->start_time += time_offset;
    first->end_time += time_offset;
    delay = first->start_time;
}

static QString parseErrorToStr(enum SubParseError err)
{
    switch (err) {
    case SUB_PARSE_ERROR_NONE:
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

    iParser->setFallbackCodec(fallback_codec);

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

    while ((newsub = iParser->loadSubtitle(&parseErr)) &&
            (parseErr == SUB_PARSE_ERROR_NONE))
        subtitles.append(newsub);

    iParser->closeSubtitle();

    switch (parseErr) {
    case SUB_PARSE_ERROR_NONE:
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
    QListIterator<Subtitle*> iter(subtitles);
    Subtitle *subtitle;

    if (!iParser)
        return;

    qDebug() << "updating FPS to" << fps;
    iParser->setFps(fps);

    foreach (subtitle, subtitles)
        iParser->updateFPS(subtitle);

    // Update time from the last one
    total_time = subtitles.last()->end_time;
    qDebug() << "total time updated to" << total_time;
}

void SubtitleEngine::increaseTime(int time)
{
    current_time += time;

    switch(state) {
    case SUB_STATE_INIT:
        break;
    case SUB_STATE_DELAY:
        delay -= time;
        break;
    case SUB_STATE_DURATION:
        duration -= time;
        break;
    case SUB_STATE_END:
        duration = 0;
        delay = 0;
        break;
    default:
        break;
    }
}

void SubtitleEngine::setTime(int time)
{
    Subtitle *tmp;
    int diff;

    current_time = time;

    foreach (tmp, subtitles) {

        // delay
        if (tmp->start_time >= time && tmp->end_time <= time) {
            state = SUB_STATE_DELAY;
            delay = tmp->start_time - time;
            break;
        }

        // duration
        if (tmp->start_time <= time && tmp->end_time >= time) {
            state = SUB_STATE_DURATION;
            duration = tmp->end_time - time;
            break;
        }

        if (tmp->start_time <= time && tmp->end_time > time &&
                (tmp->next && tmp->next->start_time > time)) {
            state = SUB_STATE_DURATION;
            diff = time - tmp->start_time;
            duration = tmp->end_time - tmp->start_time + diff;
            break;
        }

        if (tmp->start_time >= time) {
            state = SUB_STATE_DELAY;
            delay = tmp->start_time - time;
            break;
        }
    }

    current_index = tmp->index;

    return;
}

QString SubtitleEngine::getSubtitle(int time)
{
    Subtitle *current = getSubtitleNow();

    if (!iParser)
        return QString("no parser");

    if (!current)
        return QString("<subtitles end>");

    if (time)
        increaseTime(time);

    switch(state) {
    case SUB_STATE_INIT:
        break;
    case SUB_STATE_DELAY:
        if (delay <= 0) {
            state = SUB_STATE_DURATION;
            duration = current->end_time - current->start_time;
            delay = 0;
            return iParser->getSubtitleText(current);
        } else {
            return QString("");
        }

        break;
    case SUB_STATE_DURATION:
        if (duration <= 0) {
            duration = 0;
            prev_end_time = current->end_time;

            if (current_index >= subtitles.size()) {
                state = SUB_STATE_END;
                return QString("<subtitles end>");
            }

            current_index++;
            current = getSubtitleNow();

            current->start_time += time_offset;
            current->end_time += time_offset;

            if (prev_end_time >= 0 && current->start_time >= prev_end_time) {
                delay = current->start_time - prev_end_time;
                state = SUB_STATE_DELAY;
            }

            return QString("");
        }

        return iParser->getSubtitleText(current);
    case SUB_STATE_END:
        duration = 0;
        delay = 0;
    default:
        break;
    }

    return QString("");
}

void SubtitleEngine::setOffset(int offset)
{
    time_offset = offset;
}

unsigned int SubtitleEngine::getTotalTime()
{
    return total_time;
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

    if (fallback_codec == fallbackCodec) {
        qDebug() << "codec not changed";
        return -EALREADY;
    }


    fallback_codec = QString(fallbackCodec);
    qDebug() << "new codec set" << fallback_codec;

    return 0;
}

QString SubtitleEngine::getFallbackCodec()
{
    return fallback_codec;
}

void SubtitleEngine::freeSubtitles()
{
    Subtitle *subtitle;

    if (subtitles.empty())
        return;

    qDebug() << "free subtitle list";

    if (iParser) {
        foreach (subtitle, subtitles)
            iParser->freeSubtitle(subtitle);
    }

    subtitles.clear();

    resetEngine();
}

void SubtitleEngine::resetEngine()
{
    current_index = -1;
    iParser = nullptr;

    current_time = 0;
    total_time = 0;
    prev_end_time = 0;
    delay = 0;
    duration = 0;
    state = SUB_STATE_INIT;
    time_offset = 0;

    if (fallback_codec.isEmpty())
        fallback_codec = QString("Windows-1252");
}

Subtitle *SubtitleEngine::getSubtitleNow()
{
    if (current_index > subtitles.size() || current_index < 0)
        return nullptr;

    return subtitles.at(current_index);
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
