// C:/branches/SB09/main/GM/Engine/Game/zPlayerContainer.cpp
//
// MATCHED. This file spent a while at 87.86% and the reason is worth
// keeping, because it was not a source problem at all.
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class zPlayerContainer  /* 0x14 bytes */
//   {
//       /* +0x0  */ zPlayer* playerArray[4];
//       /* +0x10 */ int numPlayers;
//   };
//
// At -O4,p the loop came out one word short: mwcc strength-reduced it into
// a moving base pointer and clobbered `this`, where retail keeps `this` in
// r3 and walks a separate byte offset in r5 with `lwzx`. TEN spellings were
// compiled and scored -- index against pointer arithmetic, cast placement,
// inlined accessor, count hoisted to a local, while loop, this-relative
// index, unsigned index, found-flag-and-break -- and eight of them landed
// on exactly 87.86%. Eight ways of writing one function producing one
// object is what said the lever was not in this file.
//
// It was the OPTIMISATION SETTING. The game code is built for size:
// `-O4,s`, not `-O4,p`. See the note on `cflags_game` in configure.py.

class xEnt;
class zPlayer;

class zPlayerContainer {
public:
    bool ContainsEnt(xEnt* ent) const;

    zPlayer* playerArray[4];
    int numPlayers;
};

bool zPlayerContainer::ContainsEnt(xEnt* ent) const {
    for (int i = 0; i < numPlayers; i++) {
        if ((xEnt*)playerArray[i] == ent) {
            return true;
        }
    }

    return false;
}
