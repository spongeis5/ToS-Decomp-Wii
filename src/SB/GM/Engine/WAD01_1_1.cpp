// WAD01_1_1.cpp -- the behaviour-tree action factory, read from the
// image with tools/disasm.py. One function template, instantiated 113
// times: it takes sizeof(T) bytes from the behaviour-tree factory
// (memory type 14), places a T on them, and -- for the 106 the NPC side
// makes -- calls virtual slot 40 on the fresh object before handing it
// back. Destroy gives one back, eleven constructors are defined here,
// and the two accessors keep the asset and the client.
//
// EVERY FACT HERE IS ONE OF FOUR THINGS THE BYTES SAY: the `li r4,N`
// that is sizeof(T), the `bl __ct__...` that names the base whose
// constructor runs, the `addi r3,r31,N` in front of each member
// constructor call, and the stores between them. Nothing else about
// these classes is known -- what the members ARE is not in the image, so
// they are padding, and the ones with names are named for their offset.
//
// Three shapes the bytes fixed.
//
// THE PLACEMENT NEW IS A CONDITIONAL EXPRESSION WITH THE NULL CASE
// FIRST, the same shape zBTNodeCondition::CreateTask needed: retail
// tests the allocation twice -- once for the expression and once inside
// the new -- and lays the zero block before the constructor.
//
// `#pragma always_inline on` IS WHAT PUTS THE CONSTRUCTORS IN LINE.
// -inline auto takes a constructor whose body is the base call and one
// vtable store and declines it the moment a member constructor or a
// second store joins them; without the pragma mwcc emits __ct__<T> as
// its own function and calls it, and twenty of these come out 112 bytes
// against retail's 136 to 248. NOTES.md records the same lever on
// ClipEntity. It sits at the FOOT of the file, because mwcc
// instantiates a template at the end of the translation unit and reads
// the pragma's state THERE, while an ordinary function is compiled
// where it appears and never sees it -- which is what keeps the eleven
// constructors below from being folded into the Creates that call
// them.
//
// AND EACH T DECLARES AN OVERRIDE IT DOES NOT DEFINE, so mwcc REFERENCES
// __vt__<T> instead of emitting it. A class whose first virtual is
// defined nowhere in the unit gets no vtable of its own here, which is
// what keeps this object from defining 113 tables the manifest does not
// name.

#include "SB/GM/Engine/WAD01_1_1.pool.h"

#pragma always_inline on

namespace Sext { class ActionBase; }
class zBTClient;

namespace Memory {

enum eFactoryMemType { eFactoryMemType_ = 0x7FFFFFFF };

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
    void DeallocMem(void* block);
};

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

// The steering controls all keep the same 72 bytes in front of a vtable
// pointer at +0x48, so the data is a base and the virtuals are the
// derived class. The four floats and the flag at the end of it are what
// the stop, escort and jump controls set from their own constructors.
class xVec3 {
public:
    xVec3& operator=(const xVec3& other);

    static const xVec3 m_UnitAxisZ;

    float x;
    float y;
    float z;
};

class zNPCSteeringControlData {
public:
    unsigned char _pad0[0x34];
    float f34;
    float f38;
    float f3C;
    float f40;
    int f44;
};

class zNPCSteeringControl : public zNPCSteeringControlData {
public:
    class zWanderData {
    public:
        zWanderData();

        xVec3 dir;
        float fC;
        float f10;
        float f14;
        bool f18;
        bool f19;
        unsigned char _pad0[0x1C - 0x1A];
    };

    class zWallAvoidanceData {
    public:
        zWallAvoidanceData();

        float f0;
        float f4;
        float f8;
        bool fC;
        unsigned char _pad0[0x20 - 0xD];
    };

    zNPCSteeringControl();

    virtual void _v0();
};

class zNPCSteeringStopControl : public zNPCSteeringControl {
public:
    zNPCSteeringStopControl();

    virtual void _v0();
};

class zNPCSteeringMoveToControl : public zNPCSteeringControl {
public:
    zNPCSteeringMoveToControl();

    virtual void _v0();

    unsigned char _pad0[0x20C - 0x4C];
};

class zNPCSteeringFollowPathControl : public zNPCSteeringControl {
public:
    zNPCSteeringFollowPathControl();

    virtual void _v0();

    unsigned char _pad0[0x7C - 0x4C];
};

// Escort and jump both build their control in line, which is where the
// four floats and the flag on the shared base get written.
class zNPCSteeringEscortControl : public zNPCSteeringMoveToControl {
public:
    zNPCSteeringEscortControl();

    virtual void _v0();

    zNPCSteeringControl::zWanderData wander;
    zNPCSteeringControl::zWallAvoidanceData wallAvoidance;
};

class zNPCSteeringJumpControl : public zNPCSteeringControl {
public:
    zNPCSteeringJumpControl();

    virtual void _v0();

    float f4C;
    float f50;
    float f54;
    unsigned char _pad0[0x64 - 0x58];
    int f64;
    unsigned char _pad1[0x78 - 0x68];
    bool f78;
    unsigned char _pad2[0x7C - 0x79];
};

class zNPCBTActionAnim {
public:
    zNPCBTActionAnim();

    int f0;
    unsigned char _pad0[0x8 - 0x4];
    float f8;
    float fC;
    unsigned char _pad1[0x14 - 0x10];
    bool f14;
    unsigned char _pad2[0x18 - 0x15];
};

class zNPCBTStuckRangeMultiplier {
public:
    zNPCBTStuckRangeMultiplier();

    unsigned char _pad0[0x10];
};

class bit_array_alloc {
public:
    bit_array_alloc();

    unsigned char _pad0[0x20];
};

// Both springs set their rest value and then Reset; only the vector's
// Reset is in this unit, and it copies the target over the current.
class xSpringyVec3 {
public:
    xSpringyVec3();

    void Reset();

    float f0;
    float f4;
    float f8;
    float fC;
    xVec3 f10;
    xVec3 f1C;
};

class xSpringyF32 {
public:
    xSpringyF32();

    void Reset();

    float f0;
    float f4;
    float f8;
    unsigned char _pad0[0x10 - 0xC];
    float f10;
    unsigned char _pad1[0x30 - 0x14];
};

class zPathFinder {
public:
    zPathFinder();

    unsigned char _pad0[0x2D4];
};

// Sixteen bytes and a vtable pointer at +0, so nothing sits in front of
// it and the constructor is the store alone.
class zNPCSearchMapLinkCostCalculator {
public:
    virtual void _v0();

    unsigned char _pad0[0x10 - 0x4];
};

// The three path kinds share two floats and a vtable pointer at +8, and
// each is built in line: the base constructor is a call, the vtable
// store and the members after it are not.
class zSteeringPathData {
public:
    float f0;
    float f4;
};

class zSteeringPath : public zSteeringPathData {
public:
    zSteeringPath();

    virtual void _v0();
};

class zSteeringPathMovePoints : public zSteeringPath {
public:
    zSteeringPathMovePoints() : fC(0), f10(0) {}

    virtual void _v0();

    int fC;
    int f10;
};

class zSteeringPathLine : public zSteeringPath {
public:
    virtual void _v0();

    unsigned char _pad0[0x30 - 0xC];
};

class zSteeringPathSpline : public zSteeringPath {
public:
    zSteeringPathSpline() : f10(0), f14(0.001f) {}

    virtual void _v0();

    unsigned char _pad0[0x10 - 0xC];
    int f10;
    float f14;
    unsigned char _pad1[0x3C - 0x18];
};

inline zNPCSteeringEscortControl::zNPCSteeringEscortControl() {
    f34 = 16.0f;
    f38 = 16.0f;
    f3C = 16.0f;
    f40 = 16.0f;
    f44 = 1;
}

inline zNPCSteeringJumpControl::zNPCSteeringJumpControl() {
    f64 = 0;
    f34 = 16.0f;
    f38 = 16.0f;
    f3C = 16.0f;
    f40 = 16.0f;
    f44 = 1;
    f4C = 2.0f;
    f50 = 5.0f;
    f54 = 10.0f;
    f78 = true;
}

// The action keeps its asset and its client in front of its vtable
// pointer, which the constructor stores at +0xC. Ten virtuals: Destroy
// reads slots 0, 1, 6 and 9, and every Create the NPC side makes calls
// slot 8. Slot 0 answers what KIND of action it is and slot 1 an id,
// which is why those two are the ones with a return type.
class zBTActionData {
public:
    const Sext::ActionBase* asset;
    unsigned char _pad0[0x4];
    zBTClient* btClient;
};

class zBTAction : public zBTActionData {
public:
    zBTAction();

    virtual int _v0();
    virtual unsigned int _v1();
    virtual void _v2(Sext::ActionBase* asset);
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void _v7();
    virtual void _v8();
    virtual void _v9();

    void SetAsset(const Sext::ActionBase* value);
    void SetBTClient(zBTClient* value);

    static zBTAction gActionAlwaysComplete;
    static zBTAction gActionAlwaysFail;
};

class zBTFactory {
public:
    static Memory::Factory factory;

    template <class T> static T* Create();
};

template <class T>
T* zBTFactory::Create() {
    void* mem = factory.AllocMem(sizeof(T), (Memory::eFactoryMemType)14);

    return !mem ? 0 : new (mem) T();
}

class zNPCBTAction : public zBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    template <class T> static T* Create();

    int f10;
};

template <class T>
T* zNPCBTAction::Create() {
    void* mem =
        zBTFactory::factory.AllocMem(sizeof(T),
                                     (Memory::eFactoryMemType)14);

    T* action = !mem ? 0 : new (mem) T();

    action->_v8();

    return action;
}

// The builder keeps the client it hands every action it makes.
class zBTActionBuilder {
public:
    zBTAction* Build(int type, Sext::ActionBase* asset) const;
    void Destroy(zBTAction* action) const;

    zBTClient* btClient;
};

void zBTAction::SetBTClient(zBTClient* value) { btClient = value; }
void zBTAction::SetAsset(const Sext::ActionBase* value) { asset = value; }

// The two whose own constructor is out of line: their Create calls it
// and stores no vtable, and everything derived from them calls it too.
// Both derive from zBTAction rather than zNPCBTAction: the four bytes
// the intermediate class adds are padding either way, and going
// through it makes mwcc emit its constructor as a function these two
// then call, where retail runs zBTAction's own.
class zNPCBTMoveToAction : public zBTAction {
public:
    zNPCBTMoveToAction();

    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x18 - 0x10];
    zNPCSteeringMoveToControl steering;
    zNPCBTActionAnim anim;
    zNPCBTStuckRangeMultiplier stuckRange;
    unsigned char _pad1[0x250 - 0x24C];
    zPathFinder pathFinder;
    zNPCSearchMapLinkCostCalculator costCalculator;
    int f534;
};

class zNPCBTSwarmMoveToAction : public zBTAction {
public:
    zNPCBTSwarmMoveToAction();

    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x24 - 0x10];
    bool f24;
    unsigned char _pad1[0x28 - 0x25];
};

class zBTActionHandleEvent : public zBTAction {
public:
    zBTActionHandleEvent() : f14(0), f18(0) {}

    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
    int f14;
    int f18;
};

class zBTActionSendEvent : public zBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zBTActionWriteToBlackboard : public zBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zBTActionWriteVariable : public zBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTActionBossMeterHide : public zBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTActionBossMeterSet : public zBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTActionBossMeterShow : public zBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTBadgeCollectedAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTBounceAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x28];
};

class zNPCBTChumbotFistFlashAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x8];
};

class zNPCBTClearDamageInfoAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTDamagePlayerInRangeAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x10];
};

class zNPCBTDamagePlayerOnContactAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTDefeatedAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    zNPCSteeringStopControl steering;
    zNPCBTActionAnim anim;
};

// 42 of 47 words, 188 against retail's 200. Every instruction and
// every offset agrees; what differs is that retail keeps a POINTER
// to the control in r31 and reaches its fields through it, where
// this folds control+field into one displacement off the action and
// spends one register fewer. The inlined constructor is the same
// one either way -- see zNPCBTPathFollowMPAction, which misses the
// same way on three sub-objects.
class zNPCBTEscortAction : public zNPCBTMoveToAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    zNPCSteeringEscortControl steering;
};

class zNPCBTExtraCollisionAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0xC];
    bit_array_alloc bits;
};

class zNPCBTFaceFromEventAction : public zNPCBTAction {
public:
    zNPCBTFaceFromEventAction() : f44(0) {}

    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x8];
    zNPCBTActionAnim anim;
    unsigned char _pad1[0x10];
    int f44;
    zNPCSteeringStopControl steering;
    unsigned char _pad2[0x4];
};

class zNPCBTFadeInAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTFadeOutAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTFleeAction : public zNPCBTMoveToAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTFlutterAction : public zNPCBTMoveToAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTFollowPerceptionTargetAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    zNPCSteeringMoveToControl steering;
    zNPCBTActionAnim anim;
    zNPCBTStuckRangeMultiplier stuckRange;
    unsigned char _pad0[0xC];
};

class zNPCBTFollowPlayerAction : public zNPCBTMoveToAction {
public:
    zNPCBTFollowPlayerAction() : f538(0), f53C(0) {}

    virtual void _v2(Sext::ActionBase* asset);

    int f538;
    int f53C;
    unsigned char _pad0[0x8];
};

class zNPCBTFollowProjectileAction : public zNPCBTMoveToAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTGenerateCollectiblesAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x14];
};

class zNPCBTGenerateSpinVortexAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x8];
};

class zNPCBTHideAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTHitAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    zNPCSteeringStopControl steering;
    zNPCBTActionAnim anim;
};

class zNPCBTIdleAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    zNPCSteeringStopControl steering;
    zNPCBTActionAnim anim;
    unsigned char _pad0[0xC];
};

// 45 of 57 words at exactly retail's 228 bytes, and the reason is
// the one tools/unit_triage.py counts: the constructor loads FOUR
// distinct float literals (16, 2, 5 and 10), and in a fragment
// whose .rodata is small mwcc anchors one base register and reads
// all four off it, where retail -- past 32 KB of constants -- spells
// a lis per literal. Measured here, not assumed: this object emits
// `lis r31 / addi r31` once and four `lfs fN,K(r31)`.
class zNPCBTJumpAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    zNPCSteeringJumpControl steering;
    zNPCBTActionAnim anim;
    unsigned char _pad0[0x14];
};

class zNPCBTKillAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTMonitorPerceptionAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x8];
};

class zNPCBTOrbitAction : public zNPCBTMoveToAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    zNPCBTActionAnim anim;
    unsigned char _pad0[0x18];
};

// 39 of 60 words, 240 against retail's 248. The three paths are
// built in line in both, at the same offsets and in the same order;
// retail holds r29 and r28 on the second and third of them across
// their constructor calls and stores their vtable pointers through
// those, where this recomputes the displacement off the object and
// so needs two registers fewer -- which is also why retail saves
// r28..r31 through _savegpr and this stores two by hand.
class zNPCBTPathFollowMPAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    zNPCSteeringFollowPathControl steering;
    zNPCBTActionAnim anim;
    zNPCBTStuckRangeMultiplier stuckRange;
    zSteeringPathMovePoints movePoints;
    zSteeringPathLine line;
    zSteeringPathSpline spline;
};

class zNPCBTPathThruMPsShiftedAction : public zNPCBTMoveToAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x1C];
};

class zNPCBTPlanktonShakeAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    zNPCBTActionAnim anim0;
    zNPCBTActionAnim anim1;
    zNPCBTActionAnim anim2;
    zNPCBTActionAnim anim3;
    unsigned char _pad0[0xC];
    xSpringyVec3 springyVec;
    xSpringyF32 springyF32;
};

class zNPCBTPlayAnimationAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0xC];
    zNPCBTActionAnim anim;
    unsigned char _pad1[0x4];
    zNPCSteeringStopControl steering;
};

class zNPCBTPlayAnimationTypeAction : public zNPCBTAction {
public:
    zNPCBTPlayAnimationTypeAction() : f94(0) {}

    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x8];
    zNPCBTActionAnim anim;
    unsigned char _pad1[0x4];
    zNPCSteeringStopControl steering;
    unsigned char _pad2[0x10];
    int f94;
    unsigned char _pad3[0x8];
};

class zNPCBTPlayEELFXAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x8];
};

class zNPCBTPlayFXAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x10];
};

class zNPCBTPlayNPCFXAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x8];
};

class zNPCBTPositionEntAtBoneAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTReleaseAttackAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTRemoveAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTRequestAttackAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTResetCurrentPlayerAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTRespondToKnockbackAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTRotateToFaceAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTSetCollectibleAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTSetCollidesAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTSetCurrentHitPointsAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTSetFlyingAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTSetHitProfileAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTSetInvulnerableAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTSetNeedCombatCleanupAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTSetNeedCombatTargetingCleanupAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTSetPlanktonShakableAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTSetRPSAttackStateAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x8];
};

class zNPCBTSetUndamageableAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTShootAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTShowAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTSnapToFloorAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x40];
};

class zNPCBTStartHeadTrackingAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTStopAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    zNPCSteeringStopControl steering;
    unsigned char _pad0[0x8];
};

class zNPCBTStopHeadTrackingAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTStrikeAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    zNPCSteeringStopControl steering;
    zNPCBTActionAnim anim;
};

class zNPCBTStunAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    zNPCSteeringStopControl steering;
    zNPCBTActionAnim anim;
};

class zNPCBTSwarmBadgeCollectedAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTSwarmBugCollectedAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x8];
};

class zNPCBTSwarmFlockAction : public zNPCBTSwarmMoveToAction {
public:
    zNPCBTSwarmFlockAction() : f228(0.5235988f), f22C(0.01f) {}

    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x200];
    float f228;
    float f22C;
};

class zNPCBTSwarmFlutterAction : public zNPCBTSwarmMoveToAction {
public:
    zNPCBTSwarmFlutterAction() : f28(0) {}

    virtual void _v2(Sext::ActionBase* asset);

    int f28;
};

class zNPCBTSwarmPathFollowCircleAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x10];
};

class zNPCBTSwarmPathFollowMPAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x18];
};

class zNPCBTSwarmResetKilledMembersAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTSwarmWanderAction : public zNPCBTSwarmMoveToAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTTeleportAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x44];
};

class zNPCBTTextureSwapAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteBlackboardUidPosition : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteChildMovePointAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteClosestPlayerAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteCurHitPointsAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteCurrentPosition : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteGopherNextMovepointAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteInsideWallnetAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteLockedPlayerAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteMaxHitPointsAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteNetworkMovePointAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteNumberOfMovepointsAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWritePatrolMovePointAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWritePatrolMovePointShiftedAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTWritePerceptionTargetPositionAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTWritePlayerPositionAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteSquidBlockTimeAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x14];
};

class zNPCBTWriteSwarmHidePointAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBTWriteSwarmPosKilledByPlayerAsBadgePosAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTWriteTargetPlayerAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBTWriteTrapPositionAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0xC];
};

class zNPCBTWriteWanderPositionAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBT_Bomb_Shoot_Action : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBT_GenericSpawnerInit_Action : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCBT_InstantSpawnNPC_Action : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x10];
};

class zNPCBT_SpawnNPC_ThrowToLocation_Action : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x40];
};

class zNPCBT_Spawner_SetRotateToFaceVariable : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBT_Spawner_UnreserveNPC : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCBT_SplashDamage_Action : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x8];
};

class zNPCBT_Turret_GetVariantData_Action : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCFlyingBTWriteCurrentPosition : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCFlyingBTWriteInsideWallnetAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCFlyingBTWritePerceptionTargetPositionAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);

    unsigned char _pad0[0x4];
};

class zNPCFlyingBTWritePlayerPositionAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

class zNPCFlyingBTWriteWanderPositionAction : public zNPCBTAction {
public:
    virtual void _v2(Sext::ActionBase* asset);
};

#pragma dont_inline on

zNPCBTActionAnim::zNPCBTActionAnim() {
    f0 = 0;
    f8 = 0.2f;
    fC = 0.0f;
    f14 = true;
}

// 20 of 26 words, 108 against retail's 104, and the cause is the
// float anchor: THREE distinct literals (5, 0.5 and 10) are
// enough for mwcc to form one `lis`/`addi` base and read all
// three off it, where retail spells a lis per literal. Two is
// not -- zNPCBTActionAnim and zSteeringPath each load two and
// match. tools/unit_triage.py counts the blocker at four.
zNPCSteeringControl::zWanderData::zWanderData() {
    dir = xVec3::m_UnitAxisZ;
    fC = 5.0f;
    f10 = 0.5f;
    f14 = 10.0f;
    f18 = true;
    f19 = true;
}

// 9 of 11 words, 44 against retail's 48: the same three-literal
// anchor as zWanderData above (24, 1 and 0).
zNPCSteeringControl::zWallAvoidanceData::zWallAvoidanceData() {
    f0 = 24.0f;
    f4 = 1.0f;
    f8 = 0.0f;
    fC = true;
}

zNPCSteeringStopControl::zNPCSteeringStopControl() { f34 = 10.0f; }

zSteeringPath::zSteeringPath() {
    f0 = 0.5f;
    f4 = 0.25f;
}

void xSpringyVec3::Reset() {
    f1C = f10;
    fC = 0.0f;
    f8 = 0.0f;
    f4 = 0.0f;
}

xSpringyVec3::xSpringyVec3() {
    f0 = 10.0f;
    f10.z = 0.0f;
    f10.y = 0.0f;
    f10.x = 0.0f;

    Reset();
}

xSpringyF32::xSpringyF32() {
    f0 = 10.0f;
    f10 = 0.0f;
    f8 = 0.0f;
    f4 = 0.0f;

    Reset();
}

// 21 of 61 words, 244 against retail's 248, and the whole
// difference is one word: retail's SECOND id test is `bne` to the
// deallocation with a `b` to the exit in front of it, where this
// branches straight past. Five spellings give the same bytes --
// an and-chain of inequalities round the call, an or-chain of
// equalities that breaks, the same with an explicit else, an
// EMPTY then with the call in the else, and a nested switch on
// the id with the two constants as cases -- and the case block
// order (this one second, the four that share a deallocation
// last) is already what retail lays. Everything before the id
// test is byte-identical, dispatch included.
void zBTActionBuilder::Destroy(zBTAction* action) const {
    action->_v6();

    switch (action->_v0()) {
    case 3:
        action->_v9();
        zBTFactory::factory.DeallocMem(action);
        break;
    case 0: {
        unsigned int id = action->_v1();

        if (id == 0xF7756BA5 || id == 0xFD239E46) {
        } else {
            zBTFactory::factory.DeallocMem(action);
        }

        break;
    }
    case 1:
    case 2:
    case 4:
    case 5:
        zBTFactory::factory.DeallocMem(action);
        break;
    }
}

// The switch is 115 cases in the order the image lays their blocks,
// which is source order: 113 that make one action each, and two that
// answer with a shared global and skip the setup entirely -- and those
// two ids are exactly the pair Destroy above refuses to free.
//
// The setup block comes FIRST and the fallback last, because that is
// where retail puts them: `beq` past the setup to a block that loads
// the always-fail action.
zBTAction* zBTActionBuilder::Build(int type,
                                   Sext::ActionBase* asset) const {
    zBTAction* action = 0;

    switch (type) {
    case 0xF7756BA5:
        return &zBTAction::gActionAlwaysComplete;
    case 0xFD239E46:
        return &zBTAction::gActionAlwaysFail;
    case 0xD0762FE1:
        action = zBTFactory::Create<zBTActionSendEvent>();
        break;
    case 0xC65A85F5:
        action = zBTFactory::Create<zBTActionHandleEvent>();
        break;
    case 0x83993522:
        action = zBTFactory::Create<zNPCBTActionBossMeterShow>();
        break;
    case 0x82201CCF:
        action = zBTFactory::Create<zNPCBTActionBossMeterHide>();
        break;
    case 0x04E9B695:
        action = zBTFactory::Create<zNPCBTActionBossMeterSet>();
        break;
    case 0xFC57E1DA:
        action = zBTFactory::Create<zBTActionWriteVariable>();
        break;
    case 0x307680C0:
        action = zBTFactory::Create<zBTActionWriteToBlackboard>();
        break;
    case 0x957664BB:
        action = zNPCBTAction::Create<zNPCBTIdleAction>();
        break;
    case 0x60878ADC:
        action = zNPCBTAction::Create<zNPCBTRotateToFaceAction>();
        break;
    case 0xA91E4975:
        action = zNPCBTAction::Create<zNPCBTPlayFXAction>();
        break;
    case 0xCE65A00C:
        action = zNPCBTAction::Create<zNPCBTPlayNPCFXAction>();
        break;
    case 0x2EF33CDB:
        action = zNPCBTAction::Create<zNPCBTPlayEELFXAction>();
        break;
    case 0xE04735A5:
        action = zNPCBTAction::Create<zNPCBTPlayAnimationAction>();
        break;
    case 0x9E397889:
        action = zNPCBTAction::Create<zNPCBTBadgeCollectedAction>();
        break;
    case 0x22E53567:
        action = zNPCBTAction::Create<zNPCBTPlayAnimationTypeAction>();
        break;
    case 0x96CE7A88:
        action = zNPCBTAction::Create<zNPCBTShowAction>();
        break;
    case 0x95556235:
        action = zNPCBTAction::Create<zNPCBTHideAction>();
        break;
    case 0x429D6C58:
        action = zNPCBTAction::Create<zNPCBTSetCollidesAction>();
        break;
    case 0x7863C327:
        action = zNPCBTAction::Create<zNPCBTGenerateCollectiblesAction>();
        break;
    case 0x4A506C9D:
        action = zNPCBTAction::Create<zNPCBTSetCollectibleAction>();
        break;
    case 0xDED3AF67:
        action = zNPCBTAction::Create<zNPCBTFadeOutAction>();
        break;
    case 0x153E2F18:
        action = zNPCBTAction::Create<zNPCBTFadeInAction>();
        break;
    case 0x4A91ECAE:
        action = zNPCBTAction::Create<zNPCBTStartHeadTrackingAction>();
        break;
    case 0xC4F700D8:
        action = zNPCBTAction::Create<zNPCBTStopHeadTrackingAction>();
        break;
    case 0xAFE1279C:
        action = zNPCBTAction::Create<zNPCBTExtraCollisionAction>();
        break;
    case 0xA84C3D3A:
        action = zNPCBTAction::Create<zNPCBTPositionEntAtBoneAction>();
        break;
    case 0xA33E487B:
        action = zNPCBTAction::Create<zNPCBTTextureSwapAction>();
        break;
    case 0xF05CA191:
        action = zNPCBTAction::Create<zNPCBTWriteCurHitPointsAction>();
        break;
    case 0x129E8F7D:
        action = zNPCBTAction::Create<zNPCBTWriteMaxHitPointsAction>();
        break;
    case 0x4E2D3614:
        action = zNPCBTAction::Create<zNPCBTWriteInsideWallnetAction>();
        break;
    case 0x23FC0BC6:
        action = zNPCBTAction::Create<zNPCFlyingBTWriteInsideWallnetAction>();
        break;
    case 0x0109BB57:
        action = zNPCBTAction::Create<zNPCBTWriteNumberOfMovepointsAction>();
        break;
    case 0x9F954932:
        action = zNPCBTAction::Create<zNPCBTWriteLockedPlayerAction>();
        break;
    case 0x75A4F8D7:
        action = zNPCBTAction::Create<zNPCBTWriteClosestPlayerAction>();
        break;
    case 0x46B1E1A5:
        action = zNPCBTAction::Create<zNPCBTWriteTargetPlayerAction>();
        break;
    case 0xBD13F938:
        action = zNPCBTAction::Create<zNPCBTWritePatrolMovePointAction>();
        break;
    case 0xD3DBCD1D:
        action = zNPCBTAction::Create<zNPCBTWritePatrolMovePointShiftedAction>();
        break;
    case 0x4AA40A6E:
        action = zNPCBTAction::Create<zNPCBTWriteChildMovePointAction>();
        break;
    case 0x1B9BE8AD:
        action = zNPCBTAction::Create<zNPCBTWriteWanderPositionAction>();
        break;
    case 0x847342C3:
        action = zNPCBTAction::Create<zNPCFlyingBTWriteWanderPositionAction>();
        break;
    case 0xB4B6E161:
        action = zNPCBTAction::Create<zNPCBTWritePlayerPositionAction>();
        break;
    case 0x1D8E3B77:
        action = zNPCBTAction::Create<zNPCFlyingBTWritePlayerPositionAction>();
        break;
    case 0x457818A1:
        action = zNPCBTAction::Create<zNPCBTWriteTrapPositionAction>();
        break;
    case 0x2F5C488C:
        action = zNPCBTAction::Create<zNPCBTWriteNetworkMovePointAction>();
        break;
    case 0x5272A24B:
        action = zNPCBTAction::Create<zNPCBTWriteBlackboardUidPosition>();
        break;
    case 0xBBFADCF3:
        action = zNPCBTAction::Create<zNPCBTWriteCurrentPosition>();
        break;
    case 0x622DF635:
        action = zNPCBTAction::Create<zNPCFlyingBTWriteCurrentPosition>();
        break;
    case 0x6D7B2193:
        action = zNPCBTAction::Create<zNPCBTResetCurrentPlayerAction>();
        break;
    case 0xE7F37266:
        action = zNPCBTAction::Create<zNPCBTSetNeedCombatCleanupAction>();
        break;
    case 0x003D16CD:
        action = zNPCBTAction::Create<zNPCBTSetNeedCombatTargetingCleanupAction>();
        break;
    case 0xDF79BEFD:
        action = zNPCBTAction::Create<zNPCBTDefeatedAction>();
        break;
    case 0x96D1A1FD:
        action = zNPCBTAction::Create<zNPCBTStunAction>();
        break;
    case 0x14AE9360:
        action = zNPCBTAction::Create<zNPCBTHitAction>();
        break;
    case 0x1C5B3812:
        action = zNPCBTAction::Create<zNPCBTDamagePlayerOnContactAction>();
        break;
    case 0x2BA8AFD4:
        action = zNPCBTAction::Create<zNPCBTShootAction>();
        break;
    case 0x95BC4F25:
        action = zNPCBTAction::Create<zNPCBTKillAction>();
        break;
    case 0x26DFF12B:
        action = zNPCBTAction::Create<zNPCBTRemoveAction>();
        break;
    case 0x2A5BD78D:
        action = zNPCBTAction::Create<zNPCBTStrikeAction>();
        break;
    case 0xD1DD8099:
        action = zNPCBTAction::Create<zNPCBTDamagePlayerInRangeAction>();
        break;
    case 0x698F47B8:
        action = zNPCBTAction::Create<zNPCBTSetInvulnerableAction>();
        break;
    case 0x5DAFAAC1:
        action = zNPCBTAction::Create<zNPCBTSetUndamageableAction>();
        break;
    case 0x2840596C:
        action = zNPCBTAction::Create<zNPCBTRequestAttackAction>();
        break;
    case 0xEDF31EB4:
        action = zNPCBTAction::Create<zNPCBTReleaseAttackAction>();
        break;
    case 0x0387DC96:
        action = zNPCBTAction::Create<zNPCBTSetCurrentHitPointsAction>();
        break;
    case 0x3E831EED:
        action = zNPCBTAction::Create<zNPCBTSetHitProfileAction>();
        break;
    case 0xEE05B233:
        action = zNPCBTAction::Create<zNPCBTMoveToAction>();
        break;
    case 0x959D267D:
        action = zNPCBTAction::Create<zNPCBTJumpAction>();
        break;
    case 0x33986A11:
        action = zNPCBTAction::Create<zNPCBTFollowPlayerAction>();
        break;
    case 0x55910AB7:
        action = zNPCBTAction::Create<zNPCBTEscortAction>();
        break;
    case 0x9511909D:
        action = zNPCBTAction::Create<zNPCBTFleeAction>();
        break;
    case 0xD9EFDBC9:
        action = zNPCBTAction::Create<zNPCBTFlutterAction>();
        break;
    case 0xC686B2E1:
        action = zNPCBTAction::Create<zNPCBTFollowProjectileAction>();
        break;
    case 0x3CF0C7FE:
        action = zNPCBTAction::Create<zNPCBTTeleportAction>();
        break;
    case 0x3DE3F5BC:
        action = zNPCBTAction::Create<zNPCBTPathFollowMPAction>();
        break;
    case 0x96D19EED:
        action = zNPCBTAction::Create<zNPCBTStopAction>();
        break;
    case 0xA5B7D8C8:
        action = zNPCBTAction::Create<zNPCBTFaceFromEventAction>();
        break;
    case 0xE6C57417:
        action = zNPCBTAction::Create<zNPCBTOrbitAction>();
        break;
    case 0x725C0C42:
        action = zNPCBTAction::Create<zNPCBTSetFlyingAction>();
        break;
    case 0xFA2D2570:
        action = zNPCBTAction::Create<zNPCBTSnapToFloorAction>();
        break;
    case 0xEF418FCA:
        action = zNPCBTAction::Create<zNPCBTSwarmWanderAction>();
        break;
    case 0x928A25FC:
        action = zNPCBTAction::Create<zNPCBTSwarmFlockAction>();
        break;
    case 0xBA6835FF:
        action = zNPCBTAction::Create<zNPCBTSwarmFlutterAction>();
        break;
    case 0x3E0941AD:
        action = zNPCBTAction::Create<zNPCBTWriteSwarmHidePointAction>();
        break;
    case 0xDC3A1307:
        action = zNPCBTAction::Create<zNPCBTWriteSwarmPosKilledByPlayerAsBadgePosAction>();
        break;
    case 0x7FD6A393:
        action = zNPCBTAction::Create<zNPCBTSwarmBugCollectedAction>();
        break;
    case 0x12E94D45:
        action = zNPCBTAction::Create<zNPCBTSwarmMoveToAction>();
        break;
    case 0xCC3A4DFE:
        action = zNPCBTAction::Create<zNPCBTSwarmPathFollowMPAction>();
        break;
    case 0xA1CEE0D3:
        action = zNPCBTAction::Create<zNPCBTSwarmPathFollowCircleAction>();
        break;
    case 0x5CA86C4C:
        action = zNPCBTAction::Create<zNPCBTSwarmBadgeCollectedAction>();
        break;
    case 0x55875237:
        action = zNPCBTAction::Create<zNPCBTSwarmResetKilledMembersAction>();
        break;
    case 0x7838E47C:
        action = zNPCBTAction::Create<zNPCBT_GenericSpawnerInit_Action>();
        break;
    case 0xF0A20B00:
        action = zNPCBTAction::Create<zNPCBT_SpawnNPC_ThrowToLocation_Action>();
        break;
    case 0x6F6FDA6B:
        action = zNPCBTAction::Create<zNPCBT_Spawner_SetRotateToFaceVariable>();
        break;
    case 0x634DA9A4:
        action = zNPCBTAction::Create<zNPCBT_Spawner_UnreserveNPC>();
        break;
    case 0x9DB83D10:
        action = zNPCBTAction::Create<zNPCBT_InstantSpawnNPC_Action>();
        break;
    case 0x90416D1C:
        action = zNPCBTAction::Create<zNPCBTPathThruMPsShiftedAction>();
        break;
    case 0x4766616F:
        action = zNPCBTAction::Create<zNPCBTSetRPSAttackStateAction>();
        break;
    case 0x2A17999B:
        action = zNPCBTAction::Create<zNPCBTClearDamageInfoAction>();
        break;
    case 0xCF8433D5:
        action = zNPCBTAction::Create<zNPCBTWriteGopherNextMovepointAction>();
        break;
    case 0xFF2A858A:
        action = zNPCBTAction::Create<zNPCBTPlanktonShakeAction>();
        break;
    case 0x3F7C56C9:
        action = zNPCBTAction::Create<zNPCBTSetPlanktonShakableAction>();
        break;
    case 0x1F36793B:
        action = zNPCBTAction::Create<zNPCBTBounceAction>();
        break;
    case 0x10D63B7A:
        action = zNPCBTAction::Create<zNPCBTRespondToKnockbackAction>();
        break;
    case 0x4B135530:
        action = zNPCBTAction::Create<zNPCBTGenerateSpinVortexAction>();
        break;
    case 0xDA789044:
        action = zNPCBTAction::Create<zNPCBTMonitorPerceptionAction>();
        break;
    case 0x54497409:
        action = zNPCBTAction::Create<zNPCBT_SplashDamage_Action>();
        break;
    case 0x9950E449:
        action = zNPCBTAction::Create<zNPCBT_Bomb_Shoot_Action>();
        break;
    case 0x23FCD139:
        action = zNPCBTAction::Create<zNPCBT_Turret_GetVariantData_Action>();
        break;
    case 0x88671173:
        action = zNPCBTAction::Create<zNPCBTWriteSquidBlockTimeAction>();
        break;
    case 0xCD037DAD:
        action = zNPCBTAction::Create<zNPCBTChumbotFistFlashAction>();
        break;
    case 0x3FED91AA:
        action = zNPCBTAction::Create<zNPCBTWritePerceptionTargetPositionAction>();
        break;
    case 0x2BBED830:
        action = zNPCBTAction::Create<zNPCFlyingBTWritePerceptionTargetPositionAction>();
        break;
    case 0xAFFF3262:
        action = zNPCBTAction::Create<zNPCBTFollowPerceptionTargetAction>();
        break;
    }

    if (action != 0) {
        action->SetBTClient(btClient);
        action->SetAsset(asset);
        action->_v2(asset);

        return action;
    }

    return &zBTAction::gActionAlwaysFail;
}

#pragma dont_inline off

template zBTActionHandleEvent* zBTFactory::Create<zBTActionHandleEvent>();
template zBTActionSendEvent* zBTFactory::Create<zBTActionSendEvent>();
template zBTActionWriteToBlackboard* zBTFactory::Create<zBTActionWriteToBlackboard>();
template zBTActionWriteVariable* zBTFactory::Create<zBTActionWriteVariable>();
template zNPCBTActionBossMeterHide* zBTFactory::Create<zNPCBTActionBossMeterHide>();
template zNPCBTActionBossMeterSet* zBTFactory::Create<zNPCBTActionBossMeterSet>();
template zNPCBTActionBossMeterShow* zBTFactory::Create<zNPCBTActionBossMeterShow>();
template zNPCBTBadgeCollectedAction* zNPCBTAction::Create<zNPCBTBadgeCollectedAction>();
template zNPCBTBounceAction* zNPCBTAction::Create<zNPCBTBounceAction>();
template zNPCBTChumbotFistFlashAction* zNPCBTAction::Create<zNPCBTChumbotFistFlashAction>();
template zNPCBTClearDamageInfoAction* zNPCBTAction::Create<zNPCBTClearDamageInfoAction>();
template zNPCBTDamagePlayerInRangeAction* zNPCBTAction::Create<zNPCBTDamagePlayerInRangeAction>();
template zNPCBTDamagePlayerOnContactAction* zNPCBTAction::Create<zNPCBTDamagePlayerOnContactAction>();
template zNPCBTDefeatedAction* zNPCBTAction::Create<zNPCBTDefeatedAction>();
template zNPCBTEscortAction* zNPCBTAction::Create<zNPCBTEscortAction>();
template zNPCBTExtraCollisionAction* zNPCBTAction::Create<zNPCBTExtraCollisionAction>();
template zNPCBTFaceFromEventAction* zNPCBTAction::Create<zNPCBTFaceFromEventAction>();
template zNPCBTFadeInAction* zNPCBTAction::Create<zNPCBTFadeInAction>();
template zNPCBTFadeOutAction* zNPCBTAction::Create<zNPCBTFadeOutAction>();
template zNPCBTFleeAction* zNPCBTAction::Create<zNPCBTFleeAction>();
template zNPCBTFlutterAction* zNPCBTAction::Create<zNPCBTFlutterAction>();
template zNPCBTFollowPerceptionTargetAction* zNPCBTAction::Create<zNPCBTFollowPerceptionTargetAction>();
template zNPCBTFollowPlayerAction* zNPCBTAction::Create<zNPCBTFollowPlayerAction>();
template zNPCBTFollowProjectileAction* zNPCBTAction::Create<zNPCBTFollowProjectileAction>();
template zNPCBTGenerateCollectiblesAction* zNPCBTAction::Create<zNPCBTGenerateCollectiblesAction>();
template zNPCBTGenerateSpinVortexAction* zNPCBTAction::Create<zNPCBTGenerateSpinVortexAction>();
template zNPCBTHideAction* zNPCBTAction::Create<zNPCBTHideAction>();
template zNPCBTHitAction* zNPCBTAction::Create<zNPCBTHitAction>();
template zNPCBTIdleAction* zNPCBTAction::Create<zNPCBTIdleAction>();
template zNPCBTJumpAction* zNPCBTAction::Create<zNPCBTJumpAction>();
template zNPCBTKillAction* zNPCBTAction::Create<zNPCBTKillAction>();
template zNPCBTMonitorPerceptionAction* zNPCBTAction::Create<zNPCBTMonitorPerceptionAction>();
template zNPCBTMoveToAction* zNPCBTAction::Create<zNPCBTMoveToAction>();
template zNPCBTOrbitAction* zNPCBTAction::Create<zNPCBTOrbitAction>();
template zNPCBTPathFollowMPAction* zNPCBTAction::Create<zNPCBTPathFollowMPAction>();
template zNPCBTPathThruMPsShiftedAction* zNPCBTAction::Create<zNPCBTPathThruMPsShiftedAction>();
template zNPCBTPlanktonShakeAction* zNPCBTAction::Create<zNPCBTPlanktonShakeAction>();
template zNPCBTPlayAnimationAction* zNPCBTAction::Create<zNPCBTPlayAnimationAction>();
template zNPCBTPlayAnimationTypeAction* zNPCBTAction::Create<zNPCBTPlayAnimationTypeAction>();
template zNPCBTPlayEELFXAction* zNPCBTAction::Create<zNPCBTPlayEELFXAction>();
template zNPCBTPlayFXAction* zNPCBTAction::Create<zNPCBTPlayFXAction>();
template zNPCBTPlayNPCFXAction* zNPCBTAction::Create<zNPCBTPlayNPCFXAction>();
template zNPCBTPositionEntAtBoneAction* zNPCBTAction::Create<zNPCBTPositionEntAtBoneAction>();
template zNPCBTReleaseAttackAction* zNPCBTAction::Create<zNPCBTReleaseAttackAction>();
template zNPCBTRemoveAction* zNPCBTAction::Create<zNPCBTRemoveAction>();
template zNPCBTRequestAttackAction* zNPCBTAction::Create<zNPCBTRequestAttackAction>();
template zNPCBTResetCurrentPlayerAction* zNPCBTAction::Create<zNPCBTResetCurrentPlayerAction>();
template zNPCBTRespondToKnockbackAction* zNPCBTAction::Create<zNPCBTRespondToKnockbackAction>();
template zNPCBTRotateToFaceAction* zNPCBTAction::Create<zNPCBTRotateToFaceAction>();
template zNPCBTSetCollectibleAction* zNPCBTAction::Create<zNPCBTSetCollectibleAction>();
template zNPCBTSetCollidesAction* zNPCBTAction::Create<zNPCBTSetCollidesAction>();
template zNPCBTSetCurrentHitPointsAction* zNPCBTAction::Create<zNPCBTSetCurrentHitPointsAction>();
template zNPCBTSetFlyingAction* zNPCBTAction::Create<zNPCBTSetFlyingAction>();
template zNPCBTSetHitProfileAction* zNPCBTAction::Create<zNPCBTSetHitProfileAction>();
template zNPCBTSetInvulnerableAction* zNPCBTAction::Create<zNPCBTSetInvulnerableAction>();
template zNPCBTSetNeedCombatCleanupAction* zNPCBTAction::Create<zNPCBTSetNeedCombatCleanupAction>();
template zNPCBTSetNeedCombatTargetingCleanupAction* zNPCBTAction::Create<zNPCBTSetNeedCombatTargetingCleanupAction>();
template zNPCBTSetPlanktonShakableAction* zNPCBTAction::Create<zNPCBTSetPlanktonShakableAction>();
template zNPCBTSetRPSAttackStateAction* zNPCBTAction::Create<zNPCBTSetRPSAttackStateAction>();
template zNPCBTSetUndamageableAction* zNPCBTAction::Create<zNPCBTSetUndamageableAction>();
template zNPCBTShootAction* zNPCBTAction::Create<zNPCBTShootAction>();
template zNPCBTShowAction* zNPCBTAction::Create<zNPCBTShowAction>();
template zNPCBTSnapToFloorAction* zNPCBTAction::Create<zNPCBTSnapToFloorAction>();
template zNPCBTStartHeadTrackingAction* zNPCBTAction::Create<zNPCBTStartHeadTrackingAction>();
template zNPCBTStopAction* zNPCBTAction::Create<zNPCBTStopAction>();
template zNPCBTStopHeadTrackingAction* zNPCBTAction::Create<zNPCBTStopHeadTrackingAction>();
template zNPCBTStrikeAction* zNPCBTAction::Create<zNPCBTStrikeAction>();
template zNPCBTStunAction* zNPCBTAction::Create<zNPCBTStunAction>();
template zNPCBTSwarmBadgeCollectedAction* zNPCBTAction::Create<zNPCBTSwarmBadgeCollectedAction>();
template zNPCBTSwarmBugCollectedAction* zNPCBTAction::Create<zNPCBTSwarmBugCollectedAction>();
template zNPCBTSwarmFlockAction* zNPCBTAction::Create<zNPCBTSwarmFlockAction>();
template zNPCBTSwarmFlutterAction* zNPCBTAction::Create<zNPCBTSwarmFlutterAction>();
template zNPCBTSwarmMoveToAction* zNPCBTAction::Create<zNPCBTSwarmMoveToAction>();
template zNPCBTSwarmPathFollowCircleAction* zNPCBTAction::Create<zNPCBTSwarmPathFollowCircleAction>();
template zNPCBTSwarmPathFollowMPAction* zNPCBTAction::Create<zNPCBTSwarmPathFollowMPAction>();
template zNPCBTSwarmResetKilledMembersAction* zNPCBTAction::Create<zNPCBTSwarmResetKilledMembersAction>();
template zNPCBTSwarmWanderAction* zNPCBTAction::Create<zNPCBTSwarmWanderAction>();
template zNPCBTTeleportAction* zNPCBTAction::Create<zNPCBTTeleportAction>();
template zNPCBTTextureSwapAction* zNPCBTAction::Create<zNPCBTTextureSwapAction>();
template zNPCBTWriteBlackboardUidPosition* zNPCBTAction::Create<zNPCBTWriteBlackboardUidPosition>();
template zNPCBTWriteChildMovePointAction* zNPCBTAction::Create<zNPCBTWriteChildMovePointAction>();
template zNPCBTWriteClosestPlayerAction* zNPCBTAction::Create<zNPCBTWriteClosestPlayerAction>();
template zNPCBTWriteCurHitPointsAction* zNPCBTAction::Create<zNPCBTWriteCurHitPointsAction>();
template zNPCBTWriteCurrentPosition* zNPCBTAction::Create<zNPCBTWriteCurrentPosition>();
template zNPCBTWriteGopherNextMovepointAction* zNPCBTAction::Create<zNPCBTWriteGopherNextMovepointAction>();
template zNPCBTWriteInsideWallnetAction* zNPCBTAction::Create<zNPCBTWriteInsideWallnetAction>();
template zNPCBTWriteLockedPlayerAction* zNPCBTAction::Create<zNPCBTWriteLockedPlayerAction>();
template zNPCBTWriteMaxHitPointsAction* zNPCBTAction::Create<zNPCBTWriteMaxHitPointsAction>();
template zNPCBTWriteNetworkMovePointAction* zNPCBTAction::Create<zNPCBTWriteNetworkMovePointAction>();
template zNPCBTWriteNumberOfMovepointsAction* zNPCBTAction::Create<zNPCBTWriteNumberOfMovepointsAction>();
template zNPCBTWritePatrolMovePointAction* zNPCBTAction::Create<zNPCBTWritePatrolMovePointAction>();
template zNPCBTWritePatrolMovePointShiftedAction* zNPCBTAction::Create<zNPCBTWritePatrolMovePointShiftedAction>();
template zNPCBTWritePerceptionTargetPositionAction* zNPCBTAction::Create<zNPCBTWritePerceptionTargetPositionAction>();
template zNPCBTWritePlayerPositionAction* zNPCBTAction::Create<zNPCBTWritePlayerPositionAction>();
template zNPCBTWriteSquidBlockTimeAction* zNPCBTAction::Create<zNPCBTWriteSquidBlockTimeAction>();
template zNPCBTWriteSwarmHidePointAction* zNPCBTAction::Create<zNPCBTWriteSwarmHidePointAction>();
template zNPCBTWriteSwarmPosKilledByPlayerAsBadgePosAction* zNPCBTAction::Create<zNPCBTWriteSwarmPosKilledByPlayerAsBadgePosAction>();
template zNPCBTWriteTargetPlayerAction* zNPCBTAction::Create<zNPCBTWriteTargetPlayerAction>();
template zNPCBTWriteTrapPositionAction* zNPCBTAction::Create<zNPCBTWriteTrapPositionAction>();
template zNPCBTWriteWanderPositionAction* zNPCBTAction::Create<zNPCBTWriteWanderPositionAction>();
template zNPCBT_Bomb_Shoot_Action* zNPCBTAction::Create<zNPCBT_Bomb_Shoot_Action>();
template zNPCBT_GenericSpawnerInit_Action* zNPCBTAction::Create<zNPCBT_GenericSpawnerInit_Action>();
template zNPCBT_InstantSpawnNPC_Action* zNPCBTAction::Create<zNPCBT_InstantSpawnNPC_Action>();
template zNPCBT_SpawnNPC_ThrowToLocation_Action* zNPCBTAction::Create<zNPCBT_SpawnNPC_ThrowToLocation_Action>();
template zNPCBT_Spawner_SetRotateToFaceVariable* zNPCBTAction::Create<zNPCBT_Spawner_SetRotateToFaceVariable>();
template zNPCBT_Spawner_UnreserveNPC* zNPCBTAction::Create<zNPCBT_Spawner_UnreserveNPC>();
template zNPCBT_SplashDamage_Action* zNPCBTAction::Create<zNPCBT_SplashDamage_Action>();
template zNPCBT_Turret_GetVariantData_Action* zNPCBTAction::Create<zNPCBT_Turret_GetVariantData_Action>();
template zNPCFlyingBTWriteCurrentPosition* zNPCBTAction::Create<zNPCFlyingBTWriteCurrentPosition>();
template zNPCFlyingBTWriteInsideWallnetAction* zNPCBTAction::Create<zNPCFlyingBTWriteInsideWallnetAction>();
template zNPCFlyingBTWritePerceptionTargetPositionAction* zNPCBTAction::Create<zNPCFlyingBTWritePerceptionTargetPositionAction>();
template zNPCFlyingBTWritePlayerPositionAction* zNPCBTAction::Create<zNPCFlyingBTWritePlayerPositionAction>();
template zNPCFlyingBTWriteWanderPositionAction* zNPCBTAction::Create<zNPCFlyingBTWriteWanderPositionAction>();

zNPCBTSwarmMoveToAction::zNPCBTSwarmMoveToAction() { f24 = false; }

zNPCBTMoveToAction::zNPCBTMoveToAction() { f534 = 0; }
