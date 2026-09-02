// xFMV.cpp -- two functions, read from the image with tools/disasm.py.
// DefaultFMVPauseSoundCB hands its answer to the game's pause query.
// xFMVGetBinkCompliantVolume scales the sound module's volume at +0xA0
// to Bink's 0..32768 range, truncates it to an integer, and applies a
// function-local static scale of 1.0f to the truncated value -- the
// int-to-float goes through the 2^52 + 2^31 trick the compiler emits
// for a signed conversion. The local static is `@LOCAL@...@volumeScale@0`
// in the image and lives in .data.

bool zGameIsPaused();

class xSoundModule {
public:
    unsigned char _pad0[0xA0];
    float volume;
};

xSoundModule* GlobalGetSoundModule();

bool DefaultFMVPauseSoundCB(bool paused) { return zGameIsPaused(); }

int xFMVGetBinkCompliantVolume() {
    static float volumeScale = 1.0f;
    int volume = (int)(32768.0f * GlobalGetSoundModule()->volume);

    return (int)(volumeScale * (float)volume);
}
