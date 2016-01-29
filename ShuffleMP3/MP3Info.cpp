
#include "stdafx.h"

#include "MP3Info.h"

void getMp3Info(const WCHAR* fileName, MP3Info &info)
{
    TagLib::FileRef f(fileName);

    if (!f.isNull() && f.tag())
    {
        TagLib::Tag *tag = f.tag();

        info.tag[0] = tag->title().toWString();
        info.tag[1] = tag->artist().toWString();
        info.tag[2] = tag->album().toWString();
        info.tag[3] = tag->comment().toWString();
        info.tag[4] = tag->genre().toWString();
        info.year = tag->year();
        info.track = tag->track();

        TagLib::PropertyMap tags = f.file()->properties();

        if (!f.isNull() && f.audioProperties()) 
        {
            TagLib::AudioProperties *properties = f.audioProperties();

            int seconds = properties->length() % 60;
            int minutes = (properties->length() - seconds) / 60;

            info.bitrate = properties->bitrate();
            info.sample_rate = properties->sampleRate();
            info.channels = properties->channels();
            info.length_minutes = minutes;
            info.length_seconds = seconds;
        }
    }
}

