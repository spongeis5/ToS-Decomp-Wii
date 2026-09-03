// zNPCUpdateLOD.cpp -- five functions, read from the image with
// tools/disasm.py. The level-of-detail governor of an NPC: it decides,
// each frame, whether the NPC has to be running at all and whether it has
// to be paused, and returns the transitions the caller must act on.
// Reset keeps the NPC, takes the LOD type out of its asset and, for a
// cylinder, points at the asset's cylinder parameters.
// IsInsideUpdateRange answers true outright for an always-update NPC, and
// otherwise tests one of the NPC's LOD positions against one camera or
// player position: the squared XZ distance against the cylinder's squared
// radius scaled twice, then the height difference against the cylinder's
// downward and upward extents scaled once each. DoesNPCNeedToBeActive
// refuses an NPC that is not present or whose base flags say otherwise,
// accepts an always-update one, and then asks the NPC for its LOD
// positions and tries every one of them against every active viewport's
// camera and every player's model position, widening the range by 15%
// while the NPC is already active. DoesNPCNeedToBePaused says no unless a
// pure cinematic is running, yes when no cinematic object exists, and
// otherwise reads the NPC's two cinematic flags and its asset's enemy
// flags. CheckLOD reads what the NPC is now, asks the two questions, and
// fills the result: with nothing changed it says update or idle, and
// otherwise it says which of activate, deactivate, pause, resume and
// update the caller has to run.
//
// Layouts from the DWARF (tools/dwarf_types.py): zNPCUpdateLOD 0xC --
// lodType at +0, lodCylinder at +4, npcBase at +8; LODTypeCylinder 0xC --
// Radius, HeightUp, HeightDown; Sext::NPCAsset 0x190 with LODType at
// +0x178, the LOD union at +0x17C and EnemyFlags at +0x140; zNPCBase 0xC0
// on xOGEntity, with baseFlags at +0x26 from xBase, npcAsset at +0x60 and
// eight one-bit flags in the byte at +0x70 -- puppetMode, alive, present,
// activated, spawned, paused, updateInCinematicAlways,
// updateInCinematicNever, in that order, which is what makes present
// 0x20, activated 0x10, alive 0x40, paused 0x04 and the two cinematic
// flags 0x02 and 0x01; zNPCUpdateLODResult 0x4 -- result at +0 and five
// one-bit flags at +1, callDeactivate 0x80 down to callUpdate 0x08;
// xGlobals::players (a zPlayerContainer of four zPlayer* and a count) at
// +0x428; zGlobals::runningCinematic at +0x52C; zCam::camGroup.mat.pos at
// 0xA4 + 0x30 = +0xD4, and zPlayer::ogModel.data->Mat.pos at +0x34 then
// +0x30. The names of every local and the statement structure come from
// tools/dwarf_locals.py and tools/dwarf_lines.py, which is also how the
// two float literals were placed: 1.15f and 1.0f, each with its own lis
// because a function with two literals never shares a base.
//
// MEASURED: 4 of 5 functions byte-identical by tools/unitcmp.py. The
// miss is Reset, and the last paragraph of this comment says what it is.
//
// Four more shapes the bytes fixed, each found by a diff.
// DoesNPCNeedToBePaused ends with `return updateInCinematicNever ||
// !(EnemyFlags & 2);` written as an early return and then a return --
// folded into one `||` expression the compiler builds a common 0/1
// result and branches to it, 16 of 36 words, where two statements give
// retail's `li r3,1` on the true path and the mask-to-bool (rlwinm,
// cntlzw, srwi) on the other. The last test of IsInsideUpdateRange is
// the NEGATION of a greater-than, not a less-or-equal: `<=` on floats is
// ordered and costs a cror retail does not have. Its LodRadius2 is TWO
// statements, the load on source line 66 and the square on 67 exactly as
// dwarf_lines.py has them -- written as one expression the radius lands
// in f0 and the three loads come out in the wrong registers, 6 of 53
// words. And the range multiplier for an NPC that is already active is a
// conditional EXPRESSION, `isCurrentlyActive ? 1.15f : 1.0f`, which the
// compiler hoists out of both loops on its own.
//
// NEAR MISS, Reset at 4 of 8 words. Every word is present and every one
// is in the wrong place: retail stores npcBase, loads the asset's
// LODType, stores lodType and only then compares -- source order,
// carrying a load-use stall -- where ours hoists the load above the
// first store and sinks the second store below the compare. It is the
// instruction SCHEDULER, and two measurements say so: `#pragma
// scheduling off` around this one function gives retail's eight words
// exactly, and so does `volatile` on BOTH the npcBase store and the
// LODType read (either one alone does not). Neither is plausible source
// for a two-line reset, and the project's flag sweep has already
// measured that turning scheduling off across the library costs fifteen
// functions elsewhere, so both are recorded as evidence about the shape
// of the answer rather than used. What the pair of them says is that
// retail's compiler did not treat that store and that load as disjoint.
// Twenty spellings tie at 4: the test written against the asset field
// instead of the member, the assignment folded into the test, an early
// return instead of the if, a switch with one case, `this->` on every
// member, the comparison reversed, a local for the asset (const and
// non-const), a local for `this`, a local for the LOD type, the two
// statements swapped, the cylinder addressed by byte offset and by
// const_cast, the asset given eight-byte alignment, its LOD member
// wrapped in a union, the store retyped as an unsigned int, and the load
// retyped as a pointer.

typedef unsigned long long uid;

class LinkAsset;
class TemplateEntity;
class xOGModelRefPtr;
class zCinematic;
class zNPCBase;

class xVec3 {
public:
    float x;
    float y;
    float z;

    float Distance2XZ(const xVec3& other) const;
};

// xMat4x3 on xMat3x3: the 0x30 bytes of the rotation, then pos.
class xMat4x3 {
public:
    unsigned char _pad0[0x30];
    xVec3 pos;
    unsigned int pad3;
};

// xOGModel on xModelInstance, whose Mat is the first member.
class xModelInstance {
public:
    xMat4x3 Mat;
};

class xOGModel : public xModelInstance {
};

class xOGModelRef {
public:
    xOGModel* data;
    xOGModelRefPtr* autoptr;
};

class xOGModelHandle : public xOGModelRef {
};

class zPlayer {
public:
    unsigned char _pad0[0x34];
    xOGModelHandle ogModel;
};

class zPlayerContainer {
public:
    zPlayer* playerArray[4];
    int numPlayers;
};

class xGlobals {
public:
    unsigned char _pad0[0x428];
    zPlayerContainer players;
};

class zGlobals {
public:
    unsigned char _pad0[0x52C];
    zCinematic* runningCinematic;
};

extern xGlobals* xglobals;
extern zGlobals globals;

// zCam through xCamGroup: the camera matrix at +0xA4, its position at
// +0x30 of that, so the position a viewport hands out is at +0xD4.
class xCamGroup {
public:
    unsigned char _pad0[0xA4];
    xMat4x3 mat;
};

class zCam {
public:
    xCamGroup camGroup;
};

int zViewportGetActiveCount();
zCam* zViewportGetCamera(int index);
bool zGameIsInPureCinematic();

namespace Sext {

class LODTypeCylinder {
public:
    float Radius;
    float HeightUp;
    float HeightDown;
};

class NPCAsset {
public:
    unsigned char _pad0[0x140];
    unsigned int EnemyFlags;
    unsigned char _pad1[0x178 - 0x144];
    unsigned int LODType;
    LODTypeCylinder LODCylinder;
    unsigned char _pad2[0x190 - 0x188];
};

}  // namespace Sext

typedef Sext::LODTypeCylinder LODTypeCylinder;

enum eNPCLODType {
    eNPCLODType_LODAlwaysUpdate = 0,
    eNPCLODType_LODCylinder = 1,
    END_eNPCLODType_ENUM = 2
};

// The result CheckLOD returns by value: four bytes, so it comes back in
// r3 with no hidden pointer, and nothing initialises it.
class zNPCUpdateLODResult {
public:
    unsigned char result;
    bool callDeactivate : 1;
    bool callActivate : 1;
    bool callPause : 1;
    bool callResume : 1;
    bool callUpdate : 1;
    unsigned char pad1 : 3;
    unsigned short pad2;
};

// The vtable slot GetNPCLODPositions occupies is +96, and mwcc puts the
// Nth virtual at 8 + 4N, so it is the twenty-third.
class zNPCBase {
public:
    virtual void _v0();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void _v7();
    virtual void _v8();
    virtual void _v9();
    virtual void _v10();
    virtual void _v11();
    virtual void _v12();
    virtual void _v13();
    virtual void _v14();
    virtual void _v15();
    virtual void _v16();
    virtual void _v17();
    virtual void _v18();
    virtual void _v19();
    virtual void _v20();
    virtual void _v21();
    virtual unsigned int GetNPCLODPositions(xVec3* positions);

    unsigned char _pad0[0x26 - 0x4];
    unsigned short baseFlags;
    unsigned char _pad1[0x60 - 0x28];
    Sext::NPCAsset* npcAsset;
    unsigned char _pad2[0x70 - 0x64];
    bool puppetMode : 1;
    bool alive : 1;
    bool present : 1;
    bool activated : 1;
    bool spawned : 1;
    bool paused : 1;
    bool updateInCinematicAlways : 1;
    bool updateInCinematicNever : 1;
};

class zNPCUpdateLOD {
public:
    void Reset(zNPCBase* npcBase, const Sext::NPCAsset* npcAsset);
    bool IsInsideUpdateRange(const xVec3& npcLODPos, const xVec3& checkLODPos,
                             float rangeMultiplier);
    bool DoesNPCNeedToBeActive(int isCurrentlyActive);
    bool DoesNPCNeedToBePaused(int isCurrentlyPaused);
    zNPCUpdateLODResult CheckLOD();

    eNPCLODType lodType;
    LODTypeCylinder* lodCylinder;
    zNPCBase* npcBase;
};

void zNPCUpdateLOD::Reset(zNPCBase* npcBase, const Sext::NPCAsset* npcAsset) {
    this->npcBase = npcBase;
    lodType = (eNPCLODType)npcAsset->LODType;

    if (lodType == eNPCLODType_LODCylinder) {
        lodCylinder = (LODTypeCylinder*)&npcAsset->LODCylinder;
    }
}

bool zNPCUpdateLOD::IsInsideUpdateRange(const xVec3& npcLODPos,
                                        const xVec3& checkLODPos,
                                        float rangeMultiplier) {
    if (lodType == eNPCLODType_LODAlwaysUpdate) {
        return true;
    } else {
        float distanceXZ2 = npcLODPos.Distance2XZ(checkLODPos);
        float distanceY = checkLODPos.y - npcLODPos.y;

        float LodRadius2 = lodCylinder->Radius;
        LodRadius2 *= LodRadius2;

        if (distanceXZ2 > LodRadius2 * rangeMultiplier * rangeMultiplier) {
            return false;
        }

        if (distanceY < -lodCylinder->HeightDown * rangeMultiplier) {
            return false;
        }

        return !(distanceY > lodCylinder->HeightUp * rangeMultiplier);
    }
}

bool zNPCUpdateLOD::DoesNPCNeedToBeActive(int isCurrentlyActive) {
    if (!npcBase->present) {
        return false;
    }

    if (npcBase->baseFlags & 0x200) {
        return false;
    }

    if (lodType == eNPCLODType_LODAlwaysUpdate) {
        return true;
    }

    xVec3 npcLODPositions[4];
    unsigned int numNPCLODPositions =
        npcBase->GetNPCLODPositions(npcLODPositions);

    int numberOfCams = zViewportGetActiveCount();

    for (int i = 0; i < numberOfCams; i++) {
        xVec3* checkLODPos = &zViewportGetCamera(i)->camGroup.mat.pos;

        for (unsigned int j = 0; j < numNPCLODPositions; j++) {
            if (IsInsideUpdateRange(npcLODPositions[j], *checkLODPos,
                                    isCurrentlyActive ? 1.15f : 1.0f)) {
                return true;
            }
        }
    }

    zPlayerContainer& players = xglobals->players;

    for (int i = 0; i < players.numPlayers; i++) {
        xVec3* checkLODPos = &players.playerArray[i]->ogModel.data->Mat.pos;

        for (unsigned int j = 0; j < numNPCLODPositions; j++) {
            if (IsInsideUpdateRange(npcLODPositions[j], *checkLODPos,
                                    isCurrentlyActive ? 1.15f : 1.0f)) {
                return true;
            }
        }
    }

    return false;
}

bool zNPCUpdateLOD::DoesNPCNeedToBePaused(int) {
    if (!zGameIsInPureCinematic()) {
        return false;
    }

    if (globals.runningCinematic == 0) {
        return true;
    }

    if (npcBase->updateInCinematicAlways) {
        return false;
    }

    if (npcBase->updateInCinematicNever) {
        return true;
    }

    return !(npcBase->npcAsset->EnemyFlags & 2);
}

zNPCUpdateLODResult zNPCUpdateLOD::CheckLOD() {
    int isNPCActive = npcBase->activated;
    int isNPCPaused = npcBase->paused;

    int npcNeedsToBeActivated = DoesNPCNeedToBeActive(isNPCActive);
    int npcNeedsToBePaused = DoesNPCNeedToBePaused(isNPCPaused);

    zNPCUpdateLODResult lodResult;

    if (isNPCActive == npcNeedsToBeActivated &&
        isNPCPaused == npcNeedsToBePaused) {
        if (npcNeedsToBeActivated && !npcNeedsToBePaused) {
            lodResult.result = 0;
        } else {
            lodResult.result = 1;
        }
    } else {
        npcNeedsToBeActivated = npcNeedsToBeActivated && npcBase->alive;

        if (!isNPCActive && isNPCPaused) {
            if (npcNeedsToBeActivated && !npcNeedsToBePaused) {
                lodResult.result = 2;
                lodResult.callActivate = true;
                lodResult.callDeactivate = false;
                lodResult.callPause = false;
                lodResult.callResume = true;
                lodResult.callUpdate = true;
            } else {
                lodResult.result = 1;
            }
        } else {
            lodResult.result = 2;
            lodResult.callDeactivate = false;
            lodResult.callActivate = false;
            lodResult.callPause = false;
            lodResult.callResume = false;
            lodResult.callUpdate = false;

            if (isNPCActive != npcNeedsToBeActivated) {
                if (npcNeedsToBeActivated) {
                    lodResult.callActivate = true;
                } else {
                    lodResult.callDeactivate = true;
                }
            }

            if (npcNeedsToBePaused != isNPCPaused) {
                if (npcNeedsToBePaused) {
                    lodResult.callPause = true;
                } else {
                    lodResult.callResume = true;
                }
            }

            if (npcNeedsToBeActivated && !npcNeedsToBePaused) {
                lodResult.callUpdate = true;
            }
        }
    }

    return lodResult;
}
