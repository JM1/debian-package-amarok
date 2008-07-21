/***************************************************************************
 *   Copyright (c) 2008  Casey Link <unnamedrambler@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "Mp3tunesHarmonyDownload.h"

Mp3tunesHarmonyDownload::Mp3tunesHarmonyDownload()
{}
Mp3tunesHarmonyDownload::Mp3tunesHarmonyDownload( mp3tunes_harmony_download_t *download )
{

    m_fileKey = download->file_key;
    m_fileName = download->file_name;
    m_fileFormat = download->file_format;
    m_fileSize = download->file_size;
    m_artistName = download->artist_name ;
    if( download->album_title )
        m_albumTitle = download->album_title;
    else
        m_albumTitle = QString();
    m_trackTitle = download->track_title;
    m_trackNumber = download->track_number;
    m_deviceBitrate = download->device_bitrate;
    m_fileBitrate = download->file_bitrate;
    if( download->url )
        m_url = download->url;
    else
        m_url = QString();
}

Mp3tunesHarmonyDownload::~Mp3tunesHarmonyDownload()
{}

QString
Mp3tunesHarmonyDownload::fileKey() const
{
    return m_fileKey;
}

QString
Mp3tunesHarmonyDownload::fileName() const
{
    return m_fileName;
}

QString
Mp3tunesHarmonyDownload::fileFormat() const
{
    return m_fileFormat;
}

unsigned int
Mp3tunesHarmonyDownload::fileSize() const
{
    return m_fileSize;
}

QString
Mp3tunesHarmonyDownload::artistName() const
{
    return m_artistName;
}

QString
Mp3tunesHarmonyDownload::albumTitle() const
{
    return m_albumTitle;
}

QString
Mp3tunesHarmonyDownload::trackTitle() const
{
    return m_trackTitle;
}

int
Mp3tunesHarmonyDownload::trackNumber() const
{
    return m_trackNumber;
}

QString
Mp3tunesHarmonyDownload::deviceBitrate() const
{
    return m_deviceBitrate;
}

QString
Mp3tunesHarmonyDownload::fileBitrate() const
{
    return m_fileBitrate;
}

QString
Mp3tunesHarmonyDownload::url() const
{
    return m_url;
}
