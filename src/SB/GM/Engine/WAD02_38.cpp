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


namespace zNPCAnimViewer {

class Type {
public:
    Type();

    unsigned char _vbase[0x38];
    virtual void __vtable_anchor();
};

}  // namespace zNPCAnimViewer

namespace zNPCGeneric {

class Type {
public:
    Type();

    unsigned char _vbase[0x38];
    virtual void __vtable_anchor();
};

}  // namespace zNPCGeneric

namespace zNPCUPGeneric {

class Type {
public:
    Type();

    unsigned char _vbase[0x38];
    virtual void __vtable_anchor();
};

}  // namespace zNPCUPGeneric

namespace zNPCGenericSwarm {

class Type {
public:
    Type();

    unsigned char _vbase[0x38];
    virtual void __vtable_anchor();
};

}  // namespace zNPCGenericSwarm


class zNPCGroupType {
public:
    zNPCGroupType();

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

Graphics::MaterialDepotSpace::~MaterialDepotSpace() {}
