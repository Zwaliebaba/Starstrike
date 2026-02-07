#include "pch.h"

#include "SoundCard.h"
#include "Sound.h"



DWORD WINAPI SoundCardUpdateProc(LPVOID link);



SoundCard::SoundCard()
: status(SC_UNINITIALIZED), hthread(0), shutdown(false)
{
    DWORD thread_id = 0;
    hthread = CreateThread(0, 4096, SoundCardUpdateProc,
    (LPVOID) this, 0, &thread_id);
}



SoundCard::~SoundCard()
{
    shutdown = true;

    WaitForSingleObject(hthread, 500);
    CloseHandle(hthread);
    hthread = 0;

    sounds.destroy();
    status = SC_UNINITIALIZED;
}



DWORD WINAPI SoundCardUpdateProc(LPVOID link)
{
    SoundCard* card = (SoundCard*) link;

    if (card)
    return card->UpdateThread();

    return (DWORD) E_POINTER;
}



DWORD
SoundCard::UpdateThread()
{
    while (!shutdown) {
        Update();
        Sleep(50);
    }

    return 0;
}



void
SoundCard::Update()
{
    std::unique_lock a(sync);

    ListIter<Sound> iter = sounds;
    while (++iter) {
        Sound* s = iter.value();

        s->Update();

        if (s->GetStatus() == Sound::DONE &&
                !(s->GetFlags()  &  Sound::LOCKED)) {

            delete iter.removeItem();
        }
    }
}



void
SoundCard::AddSound(Sound* s)
{
    std::unique_lock a(sync);

    if (!sounds.contains(s))
    sounds.append(s);
}

