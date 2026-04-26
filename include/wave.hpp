/***********************************************
 * Sound related stuff, I dunno
************************************************/

#pragma once

#include <dsound.h>

void LoadWave(HINSTANCE hinst, int ResourceID,
              LPDIRECTSOUND lpds,
              LPDIRECTSOUNDBUFFER &lpDSB);
