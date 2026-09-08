// WAD03_24.cpp -- one function template written 42 times, read from
// the image with tools/disasm.py. zScene_SetupEach walks the circular
// list a scene keeps per entity subtype and does one thing to every
// object on it.
//
// EVERYTHING BUT THAT ONE THING IS THE SAME IN ALL 42, and all of it is
// in the bytes: the scene pointer at globals+0x43C, the twelve-byte
// stride the subtype index is multiplied by, the list at +40 of the
// element, and the four bytes back from a node to the object holding
// it. What the thing IS varies -- a named member, a free function taking
// the scene as well, a virtual through the object's own table, or two
// calls in a row -- so it is an overloaded helper the template calls,
// and each overload is one line.
//
// The nine virtual ones name their slot: the object's vtable pointer is
// at +0 and the index is (N - 8) / 4 of the `lwz r12,N(r12)`, which is
// why those classes carry that many placeholder declarations. None of
// them is defined here, so no vtable is emitted for any of them.
//
// THREE OF THE STEPS BRANCH TO A FUNCTION THAT IS NOT IN THE IMAGE:
// zSoundMask's and zTiki's Setup and zDispatcherData's setup helper are
// EMPTY, and the linker folded all three onto Math::Matrix33's
// constructor, the weak empty function every empty function collapses
// into. The branch is written to the name the source had; reloc_audit
// counts it as folded, which is the honest answer -- the linked image
// cannot say which of them was written.

class xVec3;
class xBase;

void operator delete(void* mem);

namespace Util {

// Declared and never defined: the destructor the list's own calls
// is an external, and the template argument is what the mangled
// name carries -- <P5xBase> is xBase* and <Ui> is unsigned int.
template <class T>
class BlockAllocatorArray {
public:
    ~BlockAllocatorArray();
};

}  // namespace Util

class OGUpdateList {
public:
    ~OGUpdateList();

    Util::BlockAllocatorArray<xBase*> bases;
    unsigned char _pad0[0x20 - 0x1];
    Util::BlockAllocatorArray<unsigned int> ids;
};

// The intrusive list a scene keeps per subtype: the anchor IS the first
// word of the element, and the node sits four bytes into its object.
struct xLink {
    xLink* next;
};

struct xSceneSubType {
    xLink link;
    unsigned char _pad0[0xC - 0x4];
};

class xScene {
public:
    unsigned char _pad0[0x28];
    xSceneSubType subTypes[1];
};

class zGlobals {
public:
    unsigned char _pad0[0x43C];
    xScene* sceneCur;
};

extern zGlobals globals;
extern int enableScreenAdj;

void zSceneEnableScreenAdj(unsigned int value);

void zSceneEnableScreenAdj(unsigned int value) { enableScreenAdj = value; }

class ElectricArc {
public:
    void Setup();
};

class ElectricCylinder {
public:
    void Setup();
};

class ElectricPoint {
public:
    void Reset();
};

namespace FX {

class zFXSpawnWithSoundAssetMultiple {
public:
    void Setup();
};

}  // namespace FX

namespace World {

class TemplateEntity {
public:
    void Setup();
};

}  // namespace World

class xGroup {
public:
    void Setup();
};

class xLightEnt {
public:
    void Setup();
};

class zBreakawayPlatform {
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
    virtual void _v22();
    virtual void _v23();
    virtual void _v24();
    virtual void _v25();
    virtual void _v26();
    virtual void Setup();
};

class zBungeeBall {
public:
    void Setup();
};

class zCamTweakCurve {
public:
    void Setup();
};

class zCatapult {
public:
    void Setup();
};

class zCinematic {
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
    virtual void _v22();
    virtual void _v23();
    virtual void _v24();
    virtual void _v25();
    virtual void _v26();
    virtual void _v27();
    virtual void _v28();
    virtual void _v29();
    virtual void _v30();
    virtual void _v31();
    virtual void Setup();
};

class zCollectibleSpawner {
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
    virtual void Setup();
};

class zDispatcherData {
public:
};

class zEntSimpleObj {
public:
};

class zExplosiveObject {
public:
    void Setup();
};

class zFloatingCollectible {
public:
    void Setup(bool flag, const xVec3* position);
};

class zFountain {
public:
    void Setup();
};

class zHammer {
public:
    void Setup();
};

class zHintSphere {
public:
    void Setup();
};

class zHitButton {
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
    virtual void _v22();
    virtual void _v23();
    virtual void _v24();
    virtual void _v25();
    virtual void Setup();
};

class zInWorldIconWidget {
public:
    void Setup();
};

class zInflatablePlatform {
public:
    void Setup();
};

class zNPCGenericPool {
public:
    void Setup();
};

class zPlantTrap {
public:
    void Setup();
};

class zPlatform {
public:
};

class zPlayerLocationEntConnector {
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
    virtual void Setup();
};

class zPuckReflector {
public:
    void Setup();
};

class zRubberBand {
public:
    void Setup();
};

class zScaleform {
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
    virtual void _v22();
    virtual void _v23();
    virtual void _v24();
    virtual void _v25();
    virtual void _v26();
    virtual void _v27();
    virtual void _v28();
    virtual void _v29();
    virtual void _v30();
    virtual void _v31();
    virtual void _v32();
    virtual void _v33();
    virtual void _v34();
    virtual void _v35();
    virtual void _v36();
    virtual void _v37();
    virtual void _v38();
    virtual void _v39();
    virtual void Setup();
};

class zSlope {
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
    virtual void _v22();
    virtual void _v23();
    virtual void _v24();
    virtual void _v25();
    virtual void _v26();
    virtual void _v27();
    virtual void _v28();
    virtual void _v29();
    virtual void _v30();
    virtual void _v31();
    virtual void _v32();
    virtual void Setup();
};

class zSoundCue {
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
    virtual void _v22();
    virtual void Setup();
};

class zSoundFXMultiple {
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
    virtual void Setup();
};

class zSoundMask {
public:
    void Setup();
};

class zSpinner {
public:
    void Setup();
};

class zTiki {
public:
    void Reset();
    void Setup();
};

class zTrampoline {
public:
    void Setup();
};

class zTriggerEntity {
public:
    void DoSetup();
};

class zUIGroup {
public:
};

class zWallNet {
public:
    void CalcBoundingBox();
};

class zWallNetGroup {
public:
    void Setup();
};

class zWaterWheel {
public:
    void Setup();
};

void zDispatcherData_Setup(zDispatcherData* object, xScene* scene);
void zEntSimpleObj_Setup(zEntSimpleObj* object);
void zPlatform_Setup(zPlatform* object, xScene* scene);
void zUIGroupSetup(zUIGroup* object);

inline void zScene_Setup(ElectricArc* p) {
    p->Setup();
}

inline void zScene_Setup(ElectricCylinder* p) {
    p->Setup();
}

inline void zScene_Setup(ElectricPoint* p) {
    p->Reset();
}

inline void zScene_Setup(FX::zFXSpawnWithSoundAssetMultiple* p) {
    p->Setup();
}

inline void zScene_Setup(World::TemplateEntity* p) {
    p->Setup();
}

inline void zScene_Setup(xGroup* p) {
    p->Setup();
}

inline void zScene_Setup(xLightEnt* p) {
    p->Setup();
}

inline void zScene_Setup(zBreakawayPlatform* p) {
    p->Setup();
}

inline void zScene_Setup(zBungeeBall* p) {
    p->Setup();
}

inline void zScene_Setup(zCamTweakCurve* p) {
    p->Setup();
}

inline void zScene_Setup(zCatapult* p) {
    p->Setup();
}

inline void zScene_Setup(zCinematic* p) {
    p->Setup();
}

inline void zScene_Setup(zCollectibleSpawner* p) {
    p->Setup();
}

inline void zScene_Setup(zDispatcherData* p) {
    zDispatcherData_Setup(p, globals.sceneCur);
}

inline void zScene_Setup(zEntSimpleObj* p) {
    zEntSimpleObj_Setup(p);
}

inline void zScene_Setup(zExplosiveObject* p) {
    p->Setup();
}

inline void zScene_Setup(zFloatingCollectible* p) {
    p->Setup(true, 0);
}

inline void zScene_Setup(zFountain* p) {
    p->Setup();
}

inline void zScene_Setup(zHammer* p) {
    p->Setup();
}

inline void zScene_Setup(zHintSphere* p) {
    p->Setup();
}

inline void zScene_Setup(zHitButton* p) {
    p->Setup();
}

inline void zScene_Setup(zInWorldIconWidget* p) {
    p->Setup();
}

inline void zScene_Setup(zInflatablePlatform* p) {
    p->Setup();
}

inline void zScene_Setup(zNPCGenericPool* p) {
    p->Setup();
}

inline void zScene_Setup(zPlantTrap* p) {
    p->Setup();
}

inline void zScene_Setup(zPlatform* p) {
    zPlatform_Setup(p, globals.sceneCur);
}

inline void zScene_Setup(zPlayerLocationEntConnector* p) {
    p->Setup();
}

inline void zScene_Setup(zPuckReflector* p) {
    p->Setup();
}

inline void zScene_Setup(zRubberBand* p) {
    p->Setup();
}

inline void zScene_Setup(zScaleform* p) {
    p->Setup();
}

inline void zScene_Setup(zSlope* p) {
    p->Setup();
}

inline void zScene_Setup(zSoundCue* p) {
    p->Setup();
}

inline void zScene_Setup(zSoundFXMultiple* p) {
    p->Setup();
}

inline void zScene_Setup(zSoundMask* p) {
    p->Setup();
}

inline void zScene_Setup(zSpinner* p) {
    p->Setup();
}

inline void zScene_Setup(zTiki* p) {
    p->Reset();
    p->Setup();
}

inline void zScene_Setup(zTrampoline* p) {
    p->Setup();
}

inline void zScene_Setup(zTriggerEntity* p) {
    p->DoSetup();
}

inline void zScene_Setup(zUIGroup* p) {
    zUIGroupSetup(p);
}

inline void zScene_Setup(zWallNet* p) {
    p->CalcBoundingBox();
}

inline void zScene_Setup(zWallNetGroup* p) {
    p->Setup();
}

inline void zScene_Setup(zWaterWheel* p) {
    p->Setup();
}

template <class T>
static void zScene_SetupEach(unsigned int subType) {
    xLink* anchor = &globals.sceneCur->subTypes[subType].link;

    for (xLink* node = anchor->next; node != anchor;
         node = node->next) {
        zScene_Setup((T*)((char*)node - 4));
    }
}

template void zScene_SetupEach<ElectricArc>(unsigned int);
template void zScene_SetupEach<ElectricCylinder>(unsigned int);
template void zScene_SetupEach<ElectricPoint>(unsigned int);
template void zScene_SetupEach<FX::zFXSpawnWithSoundAssetMultiple>(unsigned int);
template void zScene_SetupEach<World::TemplateEntity>(unsigned int);
template void zScene_SetupEach<xGroup>(unsigned int);
template void zScene_SetupEach<xLightEnt>(unsigned int);
template void zScene_SetupEach<zBreakawayPlatform>(unsigned int);
template void zScene_SetupEach<zBungeeBall>(unsigned int);
template void zScene_SetupEach<zCamTweakCurve>(unsigned int);
template void zScene_SetupEach<zCatapult>(unsigned int);
template void zScene_SetupEach<zCinematic>(unsigned int);
template void zScene_SetupEach<zCollectibleSpawner>(unsigned int);
template void zScene_SetupEach<zDispatcherData>(unsigned int);
template void zScene_SetupEach<zEntSimpleObj>(unsigned int);
template void zScene_SetupEach<zExplosiveObject>(unsigned int);
template void zScene_SetupEach<zFloatingCollectible>(unsigned int);
template void zScene_SetupEach<zFountain>(unsigned int);
template void zScene_SetupEach<zHammer>(unsigned int);
template void zScene_SetupEach<zHintSphere>(unsigned int);
template void zScene_SetupEach<zHitButton>(unsigned int);
template void zScene_SetupEach<zInWorldIconWidget>(unsigned int);
template void zScene_SetupEach<zInflatablePlatform>(unsigned int);
template void zScene_SetupEach<zNPCGenericPool>(unsigned int);
template void zScene_SetupEach<zPlantTrap>(unsigned int);
template void zScene_SetupEach<zPlatform>(unsigned int);
template void zScene_SetupEach<zPlayerLocationEntConnector>(unsigned int);
template void zScene_SetupEach<zPuckReflector>(unsigned int);
template void zScene_SetupEach<zRubberBand>(unsigned int);
template void zScene_SetupEach<zScaleform>(unsigned int);
template void zScene_SetupEach<zSlope>(unsigned int);
template void zScene_SetupEach<zSoundCue>(unsigned int);
template void zScene_SetupEach<zSoundFXMultiple>(unsigned int);
template void zScene_SetupEach<zSoundMask>(unsigned int);
template void zScene_SetupEach<zSpinner>(unsigned int);
template void zScene_SetupEach<zTiki>(unsigned int);
template void zScene_SetupEach<zTrampoline>(unsigned int);
template void zScene_SetupEach<zTriggerEntity>(unsigned int);
template void zScene_SetupEach<zUIGroup>(unsigned int);
template void zScene_SetupEach<zWallNet>(unsigned int);
template void zScene_SetupEach<zWallNetGroup>(unsigned int);
template void zScene_SetupEach<zWaterWheel>(unsigned int);

OGUpdateList::~OGUpdateList() {}
