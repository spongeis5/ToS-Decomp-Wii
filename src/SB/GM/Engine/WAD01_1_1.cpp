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
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void _v7();
    virtual void _v8();
    virtual void _v9();

    void SetAsset(const Sext::ActionBase* value);
    void SetBTClient(zBTClient* value);
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
    virtual void _v2();

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

class zBTActionBuilder {
public:
    void Destroy(zBTAction* action) const;
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

    virtual void _v2();

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

    virtual void _v2();

    unsigned char _pad0[0x24 - 0x10];
    bool f24;
    unsigned char _pad1[0x28 - 0x25];
};

class zBTActionHandleEvent : public zBTAction {
public:
    zBTActionHandleEvent() : f14(0), f18(0) {}

    virtual void _v2();

    unsigned char _pad0[0x4];
    int f14;
    int f18;
};

class zBTActionSendEvent : public zBTAction {
public:
    virtual void _v2();
};

class zBTActionWriteToBlackboard : public zBTAction {
public:
    virtual void _v2();
};

class zBTActionWriteVariable : public zBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTActionBossMeterHide : public zBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTActionBossMeterSet : public zBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTActionBossMeterShow : public zBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTBadgeCollectedAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTBounceAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x28];
};

class zNPCBTChumbotFistFlashAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zNPCBTClearDamageInfoAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTDamagePlayerInRangeAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x10];
};

class zNPCBTDamagePlayerOnContactAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTDefeatedAction : public zNPCBTAction {
public:
    virtual void _v2();

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
    virtual void _v2();

    zNPCSteeringEscortControl steering;
};

class zNPCBTExtraCollisionAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0xC];
    bit_array_alloc bits;
};

class zNPCBTFaceFromEventAction : public zNPCBTAction {
public:
    zNPCBTFaceFromEventAction() : f44(0) {}

    virtual void _v2();

    unsigned char _pad0[0x8];
    zNPCBTActionAnim anim;
    unsigned char _pad1[0x10];
    int f44;
    zNPCSteeringStopControl steering;
    unsigned char _pad2[0x4];
};

class zNPCBTFadeInAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTFadeOutAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTFleeAction : public zNPCBTMoveToAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTFlutterAction : public zNPCBTMoveToAction {
public:
    virtual void _v2();
};

class zNPCBTFollowPerceptionTargetAction : public zNPCBTAction {
public:
    virtual void _v2();

    zNPCSteeringMoveToControl steering;
    zNPCBTActionAnim anim;
    zNPCBTStuckRangeMultiplier stuckRange;
    unsigned char _pad0[0xC];
};

class zNPCBTFollowPlayerAction : public zNPCBTMoveToAction {
public:
    zNPCBTFollowPlayerAction() : f538(0), f53C(0) {}

    virtual void _v2();

    int f538;
    int f53C;
    unsigned char _pad0[0x8];
};

class zNPCBTFollowProjectileAction : public zNPCBTMoveToAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTGenerateCollectiblesAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x14];
};

class zNPCBTGenerateSpinVortexAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zNPCBTHideAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTHitAction : public zNPCBTAction {
public:
    virtual void _v2();

    zNPCSteeringStopControl steering;
    zNPCBTActionAnim anim;
};

class zNPCBTIdleAction : public zNPCBTAction {
public:
    virtual void _v2();

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
    virtual void _v2();

    zNPCSteeringJumpControl steering;
    zNPCBTActionAnim anim;
    unsigned char _pad0[0x14];
};

class zNPCBTKillAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTMonitorPerceptionAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zNPCBTOrbitAction : public zNPCBTMoveToAction {
public:
    virtual void _v2();

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
    virtual void _v2();

    zNPCSteeringFollowPathControl steering;
    zNPCBTActionAnim anim;
    zNPCBTStuckRangeMultiplier stuckRange;
    zSteeringPathMovePoints movePoints;
    zSteeringPathLine line;
    zSteeringPathSpline spline;
};

class zNPCBTPathThruMPsShiftedAction : public zNPCBTMoveToAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x1C];
};

class zNPCBTPlanktonShakeAction : public zNPCBTAction {
public:
    virtual void _v2();

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
    virtual void _v2();

    unsigned char _pad0[0xC];
    zNPCBTActionAnim anim;
    unsigned char _pad1[0x4];
    zNPCSteeringStopControl steering;
};

class zNPCBTPlayAnimationTypeAction : public zNPCBTAction {
public:
    zNPCBTPlayAnimationTypeAction() : f94(0) {}

    virtual void _v2();

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
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zNPCBTPlayFXAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x10];
};

class zNPCBTPlayNPCFXAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zNPCBTPositionEntAtBoneAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTReleaseAttackAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTRemoveAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTRequestAttackAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTResetCurrentPlayerAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTRespondToKnockbackAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTRotateToFaceAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTSetCollectibleAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTSetCollidesAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTSetCurrentHitPointsAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTSetFlyingAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTSetHitProfileAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTSetInvulnerableAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTSetNeedCombatCleanupAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTSetNeedCombatTargetingCleanupAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTSetPlanktonShakableAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTSetRPSAttackStateAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zNPCBTSetUndamageableAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTShootAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTShowAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTSnapToFloorAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x40];
};

class zNPCBTStartHeadTrackingAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTStopAction : public zNPCBTAction {
public:
    virtual void _v2();

    zNPCSteeringStopControl steering;
    unsigned char _pad0[0x8];
};

class zNPCBTStopHeadTrackingAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTStrikeAction : public zNPCBTAction {
public:
    virtual void _v2();

    zNPCSteeringStopControl steering;
    zNPCBTActionAnim anim;
};

class zNPCBTStunAction : public zNPCBTAction {
public:
    virtual void _v2();

    zNPCSteeringStopControl steering;
    zNPCBTActionAnim anim;
};

class zNPCBTSwarmBadgeCollectedAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTSwarmBugCollectedAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zNPCBTSwarmFlockAction : public zNPCBTSwarmMoveToAction {
public:
    zNPCBTSwarmFlockAction() : f228(0.5235988f), f22C(0.01f) {}

    virtual void _v2();

    unsigned char _pad0[0x200];
    float f228;
    float f22C;
};

class zNPCBTSwarmFlutterAction : public zNPCBTSwarmMoveToAction {
public:
    zNPCBTSwarmFlutterAction() : f28(0) {}

    virtual void _v2();

    int f28;
};

class zNPCBTSwarmPathFollowCircleAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x10];
};

class zNPCBTSwarmPathFollowMPAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x18];
};

class zNPCBTSwarmResetKilledMembersAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTSwarmWanderAction : public zNPCBTSwarmMoveToAction {
public:
    virtual void _v2();
};

class zNPCBTTeleportAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x44];
};

class zNPCBTTextureSwapAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteBlackboardUidPosition : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteChildMovePointAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteClosestPlayerAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteCurHitPointsAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteCurrentPosition : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteGopherNextMovepointAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteInsideWallnetAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteLockedPlayerAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteMaxHitPointsAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteNetworkMovePointAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteNumberOfMovepointsAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWritePatrolMovePointAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWritePatrolMovePointShiftedAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTWritePerceptionTargetPositionAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTWritePlayerPositionAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteSquidBlockTimeAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x14];
};

class zNPCBTWriteSwarmHidePointAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBTWriteSwarmPosKilledByPlayerAsBadgePosAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTWriteTargetPlayerAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTWriteTrapPositionAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0xC];
};

class zNPCBTWriteWanderPositionAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBT_Bomb_Shoot_Action : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBT_GenericSpawnerInit_Action : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBT_InstantSpawnNPC_Action : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x10];
};

class zNPCBT_SpawnNPC_ThrowToLocation_Action : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x40];
};

class zNPCBT_Spawner_SetRotateToFaceVariable : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBT_Spawner_UnreserveNPC : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCBT_SplashDamage_Action : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zNPCBT_Turret_GetVariantData_Action : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCFlyingBTWriteCurrentPosition : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCFlyingBTWriteInsideWallnetAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCFlyingBTWritePerceptionTargetPositionAction : public zNPCBTAction {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCFlyingBTWritePlayerPositionAction : public zNPCBTAction {
public:
    virtual void _v2();
};

class zNPCFlyingBTWriteWanderPositionAction : public zNPCBTAction {
public:
    virtual void _v2();
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
