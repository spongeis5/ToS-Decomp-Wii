// zPlayerAICommandGroup.cpp -- two functions, read from the image with
// tools/disasm.py. Sext::PlayerAICommandGroup::Create takes 88 bytes
// from the global heap (heap 0, tag 16), clears them, constructs a
// zPlayerAICommandGroup there when the block is not null (the placement
// new's own test on memset's return; the constructor clears the two
// words at +0x4C and +0x50 after the vtable, in line under the
// always-inline pragma), runs xBaseInit on it with the asset, stores the
// asset and returns. ZoneCallback, when the entity's base type is 0x55,
// asks it a question through its 106th virtual and does nothing with
// the answer but compare it: the branch folded and the compare stayed.
// The class is not named in the DWARF; its offsets are the stores.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" void* memset(void* dst, int c, unsigned long n);

inline void* operator new(unsigned long, void* p) { return p; }
#pragma always_inline on

namespace Sext {
class xBaseAsset;
class PlayerAICommandGroup;
}  // namespace Sext

class xBase {
public:
    unsigned char _pad0[0x20];
    unsigned int baseType;
};

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

// The entity a zone reports, with the one virtual the callback uses
// at its slot; the slots before it exist only to put it there.
class zZoneTarget {
public:
    virtual void _v0();   virtual void _v1();   virtual void _v2();
    virtual void _v3();   virtual void _v4();   virtual void _v5();
    virtual void _v6();   virtual void _v7();   virtual void _v8();
    virtual void _v9();   virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();
    virtual void _v15();  virtual void _v16();  virtual void _v17();
    virtual void _v18();  virtual void _v19();  virtual void _v20();
    virtual void _v21();  virtual void _v22();  virtual void _v23();
    virtual void _v24();  virtual void _v25();  virtual void _v26();
    virtual void _v27();  virtual void _v28();  virtual void _v29();
    virtual void _v30();  virtual void _v31();  virtual void _v32();
    virtual void _v33();  virtual void _v34();  virtual void _v35();
    virtual void _v36();  virtual void _v37();  virtual void _v38();
    virtual void _v39();  virtual void _v40();  virtual void _v41();
    virtual void _v42();  virtual void _v43();  virtual void _v44();
    virtual void _v45();  virtual void _v46();  virtual void _v47();
    virtual void _v48();  virtual void _v49();  virtual void _v50();
    virtual void _v51();  virtual void _v52();  virtual void _v53();
    virtual void _v54();  virtual void _v55();  virtual void _v56();
    virtual void _v57();  virtual void _v58();  virtual void _v59();
    virtual void _v60();  virtual void _v61();  virtual void _v62();
    virtual void _v63();  virtual void _v64();  virtual void _v65();
    virtual void _v66();  virtual void _v67();  virtual void _v68();
    virtual void _v69();  virtual void _v70();  virtual void _v71();
    virtual void _v72();  virtual void _v73();  virtual void _v74();
    virtual void _v75();  virtual void _v76();  virtual void _v77();
    virtual void _v78();  virtual void _v79();  virtual void _v80();
    virtual void _v81();  virtual void _v82();  virtual void _v83();
    virtual void _v84();  virtual void _v85();  virtual void _v86();
    virtual void _v87();  virtual void _v88();  virtual void _v89();
    virtual void _v90();  virtual void _v91();  virtual void _v92();
    virtual void _v93();  virtual void _v94();  virtual void _v95();
    virtual void _v96();  virtual void _v97();  virtual void _v98();
    virtual void _v99();  virtual void _v100(); virtual void _v101();
    virtual void _v102(); virtual void _v103(); virtual void _v104();
    virtual int IsZoneActive();
};

namespace World {

class EntityHandleBase;

class xOGEntity {
public:
    xOGEntity(EntityHandleBase* handle);
};

}  // namespace World

class zPlayerAICommandGroup : public World::xOGEntity {
public:
    zPlayerAICommandGroup(World::EntityHandleBase* handle)
        : World::xOGEntity(handle) {
        f4C = 0;
        f50 = 0;
    }

    virtual void __key();

    void ZoneCallback(unsigned int event, xBase* entity);

    unsigned char _pad0[0x38];
    Sext::PlayerAICommandGroup* asset;
    unsigned char _pad1[0xC];
    int f4C;
    int f50;
    unsigned char _pad2[0x4];
};

namespace Sext {

class PlayerAICommandGroup {
public:
    static zPlayerAICommandGroup* Create(World::EntityHandleBase* handle,
                                         PlayerAICommandGroup* asset);
};

}  // namespace Sext

zPlayerAICommandGroup* Sext::PlayerAICommandGroup::Create(
    World::EntityHandleBase* handle, PlayerAICommandGroup* asset) {
    zPlayerAICommandGroup* group = new (memset(
        Memory::AllocGlobalHeap(sizeof(zPlayerAICommandGroup),
                                (Memory::GlobalHeapEnum)0, (eMemMgrTag)16,
                                false),
        0, sizeof(zPlayerAICommandGroup))) zPlayerAICommandGroup(handle);

    xBaseInit((xBase*)group, (const Sext::xBaseAsset*)asset);
    group->asset = asset;

    return group;
}

void zPlayerAICommandGroup::ZoneCallback(unsigned int event, xBase* entity) {
    if (entity->baseType == 0x55) {
        if (!((zZoneTarget*)entity)->IsZoneActive()) {
            return;
        }
    }
}
