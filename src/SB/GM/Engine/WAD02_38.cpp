// The accessor part below was written by tools/gen_accessors.py and
// is kept unchanged; the rain collision callback's destructor at the foot was
// added by hand, so the generator's banner is gone -- gen_units.py
// overwrites any file that still carries it.
//
// Every function here touches nothing but its own members or a
// constant: one load, one store, the address of a member, a
// constant return, or members set to one constant. Each body was
// decoded from the image and re-encoded back to the same bytes,
// and each parameter list re-mangled back to the same symbol,
// before being written. GENERATED, not read: real matched
// functions whose offsets are recovered fact, but a count of them
// is not a count of decompiled code.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.


namespace World { class EntityHandleBase; }

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

// The NPC base every allocated type derives from: its constructor
// is a call in every one of them, so it is declared and never
// defined, and it carries a virtual because the derived classes'
// inline constructors store a vtable over its own.
class zNPCGroupBase;

class zNPCBase {
public:
    zNPCBase(World::EntityHandleBase* handle);

    virtual void _v0();
};

class zModule {
public:
    zModule();
    unsigned char _pad[0x14];
};



class zInteractionUP {
public:
    virtual void _v0() const;
    virtual void _v1() const;
    virtual void _v2() const;
    virtual void _v3() const;
    virtual void _v4() const;
    virtual void _v5() const;
    virtual void _v6() const;
    virtual void _v7() const;
    virtual void _v8() const;
    virtual void _v9() const;
    virtual void _v10() const;
    virtual void _v11() const;
    virtual void _v12() const;
    virtual void _v13() const;
    virtual void _v14() const;
    virtual void _v15() const;
    virtual void _v16() const;
    virtual void _v17() const;
    virtual void _v18() const;
    virtual void _v19() const;
    virtual void _v20() const;
    virtual void _v21() const;
    virtual void _v22() const;
    virtual void _v23() const;
    virtual void _v24() const;
    virtual void _v25() const;
    virtual void _v26() const;
    void GetCurrentPos();
    void GetOriginalMat();
    void GetOriginalPos();

};



class zNPCBTClientBase {
public:
    int GetOwner() const;

    unsigned char _pad0[0x1A0];
    int f1A0;
};


namespace Graphics {

class LightKitData {
public:
    LightKitData();

    unsigned char _pad0[0x70];
    int f70;
    int f74;
    int f78;
};

}  // namespace Graphics


class zNPCManager : public zModule {
public:
    virtual void __vtable_anchor();
    zNPCManager();

};



class zNPCNinjaManager : public zModule {
public:
    virtual void __vtable_anchor();
    zNPCNinjaManager();

};


class zNPCAnimViewer : public zNPCBase {
public:
    zNPCAnimViewer(World::EntityHandleBase* handle) : zNPCBase(handle) {}

    virtual void _v0();

    class Type {
    public:
        Type();

        unsigned char _vbase[0x38];
        virtual void __vtable_anchor();
    };

    unsigned char _pad0[0xBC];
};

class zNPCGeneric : public zNPCBase {
public:
    zNPCGeneric(World::EntityHandleBase* handle);

    class Type {
    public:
        Type();

        unsigned char _vbase[0x38];
        virtual void __vtable_anchor();
    };

    unsigned char _pad0[0x1CC];
};

class zNPCUPGeneric : public zNPCBase {
public:
    zNPCUPGeneric(World::EntityHandleBase* handle) : zNPCBase(handle) {}

    virtual void _v0();

    class Type {
    public:
        Type();

        unsigned char _vbase[0x38];
        virtual void __vtable_anchor();
    };

    unsigned char _pad0[0x15C];
};

class zNPCGenericSwarm : public zNPCBase {
public:
    zNPCGenericSwarm(World::EntityHandleBase* handle);

    class Type {
    public:
        Type();

        unsigned char _vbase[0x38];
        virtual void __vtable_anchor();
    };

    unsigned char _pad0[0x274];
};


class zNPCGroupType {
public:
    zNPCGroupType();

    template <class T>
    static zNPCGroupBase* sAllocateNPCGroup(World::EntityHandleBase* handle);

    unsigned char _vbase[0xC];
    virtual void __vtable_anchor();
};


#pragma dont_inline on
void zInteractionUP::GetOriginalPos() { _v26(); }
void zInteractionUP::GetCurrentPos() { _v22(); }
void zInteractionUP::GetOriginalMat() { _v25(); }
int zNPCBTClientBase::GetOwner() const { return f1A0; }
Graphics::LightKitData::LightKitData() { f70 = 0; f74 = 0; f78 = 0; }
zNPCManager::zNPCManager() : zModule() {}
zNPCNinjaManager::zNPCNinjaManager() : zModule() {}
zNPCAnimViewer::Type::Type() {}
zNPCGeneric::Type::Type() {}
zNPCUPGeneric::Type::Type() {}
zNPCGenericSwarm::Type::Type() {}
zNPCGroupType::zNPCGroupType() {}
#pragma dont_inline off

void operator delete(void* mem);

// Its destructor is a real symbol in another unit: declared here, not
// defined.
class HavokPrimObj {
public:
    ~HavokPrimObj();
};

// The compiler's own destructor: the null-this test, the member at the
// offset below destroyed with the don't-delete flag, and operator
// delete when the CALLER's flag is positive, then `return this`. There
// is no second call, so nothing it derives from has a destructor.
// Six qualifiers deep, which the mangled name spells out in full:
// Q62FX9Particles13MotionSystems9Collision4Rain16intersect_env_CB.
namespace FX {
namespace Particles {
namespace MotionSystems {
namespace Collision {

class Rain {
public:
    class intersect_env_CB {
    public:
        ~intersect_env_CB();

        unsigned char _pad0[0x4];
        HavokPrimObj prim;
    };
};

}  // namespace Collision
}  // namespace MotionSystems
}  // namespace Particles
}  // namespace FX

FX::Particles::MotionSystems::Collision::Rain::intersect_env_CB::~intersect_env_CB() {}

// The 80-byte base-only destructor, the compiler's own: the
// null-this test, the BASE's destructor on `this` with the flag
// CLEAR -- r4 = 0 is a base subobject where r4 = -1 is a complete
// one -- then operator delete when the CALLER's flag is positive,
// and `return this`. No member is destroyed, so the class needs
// nothing but its base.
//
// Non-virtual throughout: the call is a direct `bl` either way, and
// a virtual destructor would make this unit the home of a vtable
// retail keeps elsewhere. Where the base's destructor folded onto
// __dt__12hkBaseObjectFv -- 64 bytes of null test, conditional
// operator delete and `return this` -- the base is spelled as the
// symbol that survived, which is what makes the relocation reach
// retail's own.
void operator delete(void* mem);

class xLightEffectFlicker {
public:
    ~xLightEffectFlicker();
};

class zNPCGenericSpawner : public xLightEffectFlicker {
public:
    ~zNPCGenericSpawner();
};

class zInteractionUPWithIcons : public xLightEffectFlicker {
public:
    ~zInteractionUPWithIcons();
};

class hkBaseObject {
public:
    ~hkBaseObject();
};

class zNPCCombatCollisionListener : public hkBaseObject {
public:
    ~zNPCCombatCollisionListener();
};

namespace Graphics {
class ScreenView : public hkBaseObject {
public:
    ~ScreenView();
};
}  // namespace Graphics

namespace Graphics {
class MaterialDepotSpace : public hkBaseObject {
public:
    ~MaterialDepotSpace();
};
}  // namespace Graphics

zNPCGenericSpawner::~zNPCGenericSpawner() {}

zInteractionUPWithIcons::~zInteractionUPWithIcons() {}

zNPCCombatCollisionListener::~zNPCCombatCollisionListener() {}

Graphics::ScreenView::~ScreenView() {}

// The flag is -1, not 0: what it destroys is a MEMBER at +0, a
// complete subobject, and not a base.
namespace EngineOG {
class SceneData {
public:
    ~SceneData();

    hkBaseObject m;
};
}  // namespace EngineOG

Graphics::MaterialDepotSpace::~MaterialDepotSpace() {}

EngineOG::SceneData::~SceneData() {}

// Memory::Creator<N, T, B>: one template body per instantiation.
// The factory allocation, a null test, and a placement new whose
// own null guard is the second `beq`. sizeof(T) is the `li r4`,
// N is the `li r5`, and the constructor is either a `bl` -- so it
// is declared and never defined -- or the vtable store an INLINE
// one makes, in which case the members it zeroes are read off the
// stores that follow.

extern "C" {
void* memset(void* dst, int c, unsigned long n);
}

inline void* operator new(unsigned long, void* p) { return p; }

namespace Memory {

enum eFactoryMemType { eFactoryMemType_ = 0x7FFFFFFF };

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
};

template <int N, class T, class B>
class Creator {
public:
    static B* Create(Factory* f);
    virtual B* CreateV(Factory* f);
};

template <int N, class T, class B>
B* Creator<N, T, B>::Create(Factory* f) {
    void* p = f->AllocMem(sizeof(T), (eFactoryMemType)N);

    if (p == 0) {
        return 0;
    }

    return new (p) T;
}

template <int N, class T, class B>
B* Creator<N, T, B>::CreateV(Factory* f) {
    void* p = f->AllocMem(sizeof(T), (eFactoryMemType)N);

    if (p == 0) {
        return 0;
    }

    return new (p) T;
}

}  // namespace Memory

// Constructors that are a CALL: declared, never defined, and the
// class padded to the size the allocation asks for.
class zNPCFX { public: zNPCFX(); unsigned char _pad0[60]; };
class zNPCPerception { public: zNPCPerception(); unsigned char _pad0[488]; };

class zNPCSteering { public: unsigned char _pad0[1]; };

class zNPCSwarmSteering : public zNPCSteering {
public:
    zNPCSwarmSteering();

    unsigned char _pad0[5703];
};

class zNPCSingleSteering : public zNPCSteering {
public:
    zNPCSingleSteering();

    unsigned char _pad0[167];
};

// Constructors that are INLINE: the vtable store is the whole of
// one, and the words after it are the members it zeroes.
class zNPCFXNode { public: unsigned char _pad0[0xC]; };

class zNPCFXImmediateInstanceLoop : public zNPCFXNode {
public:
    virtual void _v0();

    unsigned char _pad1[48 - 16];
};

class zNPCFXLoopFXScript : public zNPCFXNode {
public:
    zNPCFXLoopFXScript() : m10(0), m14(0), m18(0) {}

    virtual void _v0();

    int m10;
    int m14;
    int m18;
};

class zNPCFXOneShotFXScript : public zNPCFXNode {
public:
    zNPCFXOneShotFXScript() : m10(0), m14(0) {}

    virtual void _v0();

    int m10;
    int m14;
};

class zNPCLogic { public: zNPCLogic() : m0(0) {} int m0; };

class zNPCBTManager : public zNPCLogic {
public:
    virtual void _v0();

    unsigned char _pad0[4];
};

// The member at +0 is stored BEFORE the vtable, so it belongs to a
// BASE whose inline constructor runs first. The DWARF names that
// base zNPCComponent and puts it at +0 of a 0xC0-byte class.
class zNPCComponent { public: zNPCComponent() : m0(0) {} int m0; };

class zNPCQuickTimeCombat : public zNPCComponent {
public:
    virtual void _v0();

    unsigned char _pad0[192 - 8];
};

// always_inline because a constructor with an initialiser list is
// past what -inline auto takes on its own: without it mwcc emits
// __ct__18zNPCFXLoopFXScriptFv out of line and calls it, which
// unitcmp reports as a function NOT IN RETAIL.
#pragma always_inline on

// ONE MEMBER EACH, not the whole class: retail carries Create for
// three of these and CreateV for the rest, never both, so
// instantiating the class emits a function the image does not have.
template zNPCFX* Memory::Creator<3, zNPCFX, zNPCFX>::CreateV(Memory::Factory*);
template zNPCPerception* Memory::Creator<5, zNPCPerception, zNPCPerception>::CreateV(Memory::Factory*);
template zNPCSteering* Memory::Creator<7, zNPCSwarmSteering, zNPCSteering>::CreateV(Memory::Factory*);
template zNPCSteering* Memory::Creator<7, zNPCSingleSteering, zNPCSteering>::CreateV(Memory::Factory*);
template zNPCFXNode* Memory::Creator<4, zNPCFXImmediateInstanceLoop, zNPCFXNode>::Create(Memory::Factory*);
template zNPCFXNode* Memory::Creator<4, zNPCFXLoopFXScript, zNPCFXNode>::Create(Memory::Factory*);
template zNPCFXNode* Memory::Creator<4, zNPCFXOneShotFXScript, zNPCFXNode>::Create(Memory::Factory*);
template zNPCLogic* Memory::Creator<1, zNPCBTManager, zNPCLogic>::CreateV(Memory::Factory*);
template zNPCQuickTimeCombat* Memory::Creator<16, zNPCQuickTimeCombat, zNPCQuickTimeCombat>::CreateV(Memory::Factory*);

#pragma always_inline off

// zNPCType::sAllocateNPC<T> and zNPCGroupType::sAllocateNPCGroup<T>:
// one allocation, one memset, one placement new. sizeof(T) is the
// `li r3` and the memset's `li r5`, which agree; the constructor is
// either a `bl` of its own or the base's `bl` followed by the vtable
// store an INLINE one makes.

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap,
                      eMemMgrTag tag, bool clear);
}  // namespace Memory

class zNPCGroupBase {
public:
    zNPCGroupBase(World::EntityHandleBase* handle);

    virtual void _v0();
};

class zNPCGroupCircle : public zNPCGroupBase {
public:
    zNPCGroupCircle(World::EntityHandleBase* handle)
        : zNPCGroupBase(handle) {}

    virtual void _v0();

    unsigned char _pad0[72 - 4];
};

class zNPCGroupSpaceInvaders : public zNPCGroupBase {
public:
    zNPCGroupSpaceInvaders(World::EntityHandleBase* handle)
        : zNPCGroupBase(handle) {}

    virtual void _v0();

    unsigned char _pad0[72 - 4];
};

// Nothing of its own: zNPCGeneric is already the whole 464 bytes,
// and the virtual is here only so the class has a vtable to store.
class zNPCSBGeneric : public zNPCGeneric {
public:
    zNPCSBGeneric(World::EntityHandleBase* handle)
        : zNPCGeneric(handle) {}

    virtual void _v0();
};

class zNPCType {
public:
    template <class T>
    static zNPCBase* sAllocateNPC(World::EntityHandleBase* handle);
};

template <class T>
zNPCBase* zNPCType::sAllocateNPC(World::EntityHandleBase* handle) {
    return new (memset(
        Memory::AllocGlobalHeap(sizeof(T), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(T))) T(handle);
}

template <class T>
zNPCGroupBase* zNPCGroupType::sAllocateNPCGroup(
    World::EntityHandleBase* handle) {
    return new (memset(
        Memory::AllocGlobalHeap(sizeof(T), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(T))) T(handle);
}

#pragma always_inline on

template zNPCBase* zNPCType::sAllocateNPC<zNPCAnimViewer>(
    World::EntityHandleBase*);
template zNPCBase* zNPCType::sAllocateNPC<zNPCGeneric>(
    World::EntityHandleBase*);
// zNPCGenericSpawner is left out: this file already derives it
// from xLightEffectFlicker for its destructor, and sAllocateNPC
// wants it deriving from zNPCBase. One of those two is wrong and
// the bytes here do not say which, so neither is guessed at.
template zNPCBase* zNPCType::sAllocateNPC<zNPCUPGeneric>(
    World::EntityHandleBase*);
template zNPCBase* zNPCType::sAllocateNPC<zNPCSBGeneric>(
    World::EntityHandleBase*);
template zNPCBase* zNPCType::sAllocateNPC<zNPCGenericSwarm>(
    World::EntityHandleBase*);
template zNPCGroupBase* zNPCGroupType::sAllocateNPCGroup<
    zNPCGroupCircle>(World::EntityHandleBase*);
template zNPCGroupBase* zNPCGroupType::sAllocateNPCGroup<
    zNPCGroupSpaceInvaders>(World::EntityHandleBase*);

#pragma always_inline off
