// WAD02_26 -- zNPCGeneric::Type::CreateAnimTable, the sibling of
// zNPCUPGeneric's: 44 states, no owner, no callback, read from the
// image with tools/gen_animtables.py --calls and written in the
// spelling that unit already matches with.
//
// This file is a fragment of the WAD02 unity build; the generated
// header puts the whole string pool in front and the .rodata the
// files ahead contribute, so every offset comes out as retail has it.
#include "SB/GM/Engine/WAD02_26.pool.h"

class xAnimTable;
class xAnimPlay;
class xAnimState;
class xAnimSingle;
class xQuat;
class xVec3;

// Sixteen parameters, in the order the EABI put them: r3, r4, r5,
// r6, f1, r7, r8, f2, r9, r10, then four function pointers, an
// eight-byte-aligned unsigned long long, and one more word.
void xAnimTableNewState(xAnimTable* table, const char* name,
                        unsigned int a, unsigned int b, float c,
                        float* d, float* e, float f,
                        unsigned short* g, void* h,
                        void (*i)(xAnimPlay*, xAnimState*, void*),
                        void (*j)(xAnimPlay*, xAnimState*, void*),
                        void (*k)(xAnimState*, xAnimSingle*, void*),
                        void (*l)(xAnimPlay*, xQuat*, xVec3*, xVec3*,
                                  int),
                        unsigned long long m, unsigned int n);

class zNPCGeneric {
public:
    class Type {
    public:
        void CreateAnimTable(xAnimTable* table);
    };
};

// --------------------------------------------------------------------------

void zNPCGeneric::Type::CreateAnimTable(xAnimTable* table) {
    xAnimTableNewState(table, "IDLE", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "FACE", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "MOSEY", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "WALK", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "JOG", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "RUN", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "SPRINT", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "CHARGE", 16, 0x4000000, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "AWARE", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "JUMP", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "JUMP_START", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "JUMP_END", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "ORBIT_LEFT", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "ORBIT_RIGHT", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "HIT", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "HIT_SPIN", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "HIT_HAMMER", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "HIT_PUCK", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "HIT_KNOCKBACK", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "STUN_PREPARE", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "STUN", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "STUN_RECOVER", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "NOTICE_PLAYER", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "STRIKE_PREPARE", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "STRIKE_PREPARE2", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "STRIKE", 0, 0x4000000, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "STRIKE2", 0, 0x4000000, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "STRIKE_LOOP", 16, 0x4000000, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "STRIKE_RECOVER", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "STRIKE_RECOVER2", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "DEFEATED", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "MOVE_UP", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "MOVE_DOWN", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "SHAKE", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "SHAKE_DROP", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "SHAKE_SLAM", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "SHAKE_SLAMPREP", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "SPECIAL_LOOP", 16, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "SPECIAL_STOP", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "SPRING_RECOVER", 0, 0x4000000, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "SPAWN_PREPARE", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "SPAWN_END", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "GENERIC1", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
    xAnimTableNewState(table, "GENERIC2", 0, 0x0, 1.0f, 0, 0,
                       0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
}
