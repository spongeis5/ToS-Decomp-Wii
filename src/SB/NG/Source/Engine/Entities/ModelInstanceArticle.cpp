// The accessor part below was written by tools/gen_accessors.py and is
// kept unchanged; Graphics::Renderable3D::AmendProjectShadowFlag at the
// foot was added by hand, so the generator's banner is gone --
// gen_units.py overwrites any file that still carries it.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.
//
// This is the one place in the image where an Amend and the Set it calls
// are both written, and the pair is what the shape looks like end to
// end: SetProjectShadowFlag is a single store, and
// AmendProjectShadowFlag reaches it through a POINTER TO MEMBER -- the
// twelve-byte constant onto the stack, r12 pointed at it, __ptmf_scall.
// The constant is at 806CA0E8 and names the target: delta 0, vtable
// offset -1, and a third word that is SetProjectShadowFlag's own address
// at 801BA6F0.
//
// The unsigned int passes through untouched in r4, so the constant is
// copied through r7/r6 rather than the r6/r5 of the ones whose only
// arguments are floats.

namespace Graphics {

class Renderable3D {
public:
    void SetProjectShadowFlag(unsigned int value);
    void AmendProjectShadowFlag(unsigned int value);

    unsigned char _pad0[0xF0];
    int fF0;
};

}  // namespace Graphics

namespace World {

class UVMovementInstanceData {
public:
    void SetRows(float* value);

    unsigned char _pad0[0x5C];
    int f5C;
};

}  // namespace World

void Graphics::Renderable3D::SetProjectShadowFlag(unsigned int value) { fF0 = value; }
void World::UVMovementInstanceData::SetRows(float* value) { f5C = (int)value; }

void Graphics::Renderable3D::AmendProjectShadowFlag(unsigned int value) {
    void (Renderable3D::*set)(unsigned int) =
        &Renderable3D::SetProjectShadowFlag;

    (this->*set)(value);
}
