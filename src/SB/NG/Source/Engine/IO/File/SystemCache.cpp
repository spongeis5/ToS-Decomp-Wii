// SystemCache.cpp -- five functions, read from the image with
// tools/disasm.py. The Wii system cache parks blocks of memory in NAND
// files: CacheMemory takes a 32-byte-aligned block and size, finds a
// free one of four slots, names the file from the cache file name and
// the slot number, creates it (an existing file is fine), opens it for
// reading and writing, writes the block and records it, and answers
// the slot number plus one, or zero at any refusal. RestoreMemory by
// block finds the slot and restores by number; by number it seeks the
// file to its start, reads the block back, closes the file and frees
// the slot. The module's constructor builds the System::Module base,
// stores the vtable and sets the third event stage to 67;
// GetSystemCache hands back the cache.
//
// Layouts from the DWARF (tools/dwarf_types.py): SystemCacheWii is four
// CachedMemory entries of 0x94, each a NANDFileInfo (0x8C) then the
// block and its size; System::Module's vptr follows its name and event
// set at +0x14. The file name format is the unity unit's pool, hence
// the generated header first; the cache object is data of the unit's
// own, so it matches and does not link.

#include "SB/NG/Source/Engine/IO/File/SystemCache.pool.h"

extern "C" {
struct NANDFileInfo {
    unsigned char _pad0[0x8C];
};

int snprintf(char* dst, unsigned long size, const char* fmt, ...);
int NANDCreate(const char* path, unsigned char perm, unsigned char attr);
int NANDOpen(const char* path, NANDFileInfo* info, unsigned char access);
int NANDClose(NANDFileInfo* info);
int NANDRead(NANDFileInfo* info, void* buffer, unsigned int length);
int NANDWrite(NANDFileInfo* info, const void* buffer, unsigned int length);
int NANDSeek(NANDFileInfo* info, int offset, int whence);
}

namespace System {

class EventSet {
public:
    int stage[4];
};

// The vptr follows the two members declared ahead of the first virtual
// (+0x14); slot 0 is left undefined here so the vtable's home stays in
// the unity unit's data.
class Module {
public:
    Module();

    char* name;
    EventSet events;

    virtual void _v0();

    int contextFlags;
    short eventBindingIndices[60];
    bool enabled;
};

}  // namespace System

class SystemCacheModule : public System::Module {
public:
    SystemCacheModule();
};

namespace IO {

extern const char* wiiSystemCacheFile;

class CachedMemory {
public:
    NANDFileInfo fileInfo;
    void* cachedMemory;
    int cachedSize;
};

class SystemCacheWii {
public:
    static SystemCacheWii* GetSystemCache();

    int CacheMemory(void* memory, int size);
    void RestoreMemory(void* memory);
    void RestoreMemory(int handle);

    CachedMemory cachedMemory[4];
};

SystemCacheWii wiiSystemCache;

}  // namespace IO

SystemCacheModule::SystemCacheModule() {
    events.stage[2] = 67;
}

IO::SystemCacheWii* IO::SystemCacheWii::GetSystemCache() {
    return &wiiSystemCache;
}

int IO::SystemCacheWii::CacheMemory(void* memory, int size) {
    if ((unsigned int)memory & 31) {
        return 0;
    }

    if (size & 31) {
        return 0;
    }

    int slot = 0;

    while (cachedMemory[slot].cachedMemory != 0 && slot < 4) {
        slot++;
    }

    if (slot >= 4) {
        return 0;
    }

    char path[256];

    snprintf(path, 256, "%s%02d", wiiSystemCacheFile, slot);

    int result = NANDCreate(path, 48, 0);

    if (result != 0 && result != -6) {
        return 0;
    }

    if (NANDOpen(path, &cachedMemory[slot].fileInfo, 3) != 0) {
        return 0;
    }

    int written = NANDWrite(&cachedMemory[slot].fileInfo, memory, size);

    if (written != size) {
        NANDClose(&cachedMemory[slot].fileInfo);
        return 0;
    }

    cachedMemory[slot].cachedMemory = memory;
    cachedMemory[slot].cachedSize = size;

    return slot + 1;
}

void IO::SystemCacheWii::RestoreMemory(void* memory) {
    for (int slot = 0; slot < 4; slot++) {
        if (memory == cachedMemory[slot].cachedMemory) {
            RestoreMemory(slot + 1);
            return;
        }
    }
}

void IO::SystemCacheWii::RestoreMemory(int handle) {
    int slot = handle - 1;

    if (cachedMemory[slot].cachedMemory) {
        NANDSeek(&cachedMemory[slot].fileInfo, 0, 0);
        NANDRead(&cachedMemory[slot].fileInfo, cachedMemory[slot].cachedMemory,
                 cachedMemory[slot].cachedSize);
        NANDClose(&cachedMemory[slot].fileInfo);

        cachedMemory[slot].cachedMemory = 0;
    }
}
