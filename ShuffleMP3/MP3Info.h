
#pragma once

#include "stdafx.h"

#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>

enum TagIndex
{
    TAG_TITLE = 0,
    TAG_ARTIST = 1,
    TAG_ALBUM = 2,
    TAG_COMMENT = 3,
    TAG_GENRE = 4
};

typedef struct _MP3Info
{
    wstring tag[5];
    UINT year;
    UINT track;
    INT bitrate;
    INT sample_rate;
    INT channels;
    INT length_minutes;
    INT length_seconds;

} MP3Info;

void getMp3Info(const WCHAR* fileName, MP3Info &info);

