// The accessor part below was written by tools/gen_accessors.py and is
// kept unchanged; Graphics::Renderable::AmendColorMulAlpha at the foot
// was added by hand, so the generator's banner is gone -- gen_units.py
// overwrites any file that still carries it.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.
//
// AmendColorMulAlpha is in this unit and not in a renderer one because
// that is where the linker put it: the DWARF says it was DEFINED IN
// Renderable.h, at line 467, so it is an inline that some translation
// unit in this run emitted out of line, and this is the run. Nothing
// about it belongs to zPlatform.
//
// The body is its own SetColorMulAlpha called through a pointer to
// member -- the twelve-byte constant onto the stack, r12 pointed at it,
// __ptmf_scall. The constant is in the image: delta 0, vtable offset -1,
// and a third word that is SetColorMulAlpha's address. The float passes
// through untouched, because __ptmf_scall adjusts r3 and jumps.

namespace World { class EntityHandleBase; }


class xEnt {
public:
    xEnt(World::EntityHandleBase* a0);
    virtual void __vtable_anchor();
};



class zEnt : public xEnt {
public:
    virtual void __vtable_anchor();
    zEnt(World::EntityHandleBase* a0);

};


namespace Graphics {

class Renderable {
public:
    void SetColorMulAlpha(float alpha);
    void AmendColorMulAlpha(float alpha);
};

}  // namespace Graphics


#pragma dont_inline on
zEnt::zEnt(World::EntityHandleBase* a0) : xEnt(a0) {}
#pragma dont_inline off

void Graphics::Renderable::AmendColorMulAlpha(float alpha) {
    void (Renderable::*set)(float) = &Renderable::SetColorMulAlpha;

    (this->*set)(alpha);
}
