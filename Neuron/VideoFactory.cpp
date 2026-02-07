#include "pch.h"

#include "VideoFactory.h"

#include "VideoDX9.h"
#include "SoundD3D.h"



VideoFactory::VideoFactory(HWND h)
: hwnd(h), video(0), audio(0)
{ }

VideoFactory::~VideoFactory()
{ }



Video*
VideoFactory::CreateVideo(VideoSettings* vs)
{
    if (!video) {
        video = (Video*) NEW VideoDX9(hwnd, vs);
        
        if (!video) {
            delete video;
            video = 0;
        }
    }

    return video;
}



void
VideoFactory::DestroyVideo(Video* v)
{
    if (v == video) {
        delete video;
        video = 0;
    }
}



SoundCard*
VideoFactory::CreateSoundCard()
{
    if (!audio) {
        audio = NEW SoundCardD3D(hwnd);
        Sound::UseSoundCard(audio);
    }

    return audio;
}


