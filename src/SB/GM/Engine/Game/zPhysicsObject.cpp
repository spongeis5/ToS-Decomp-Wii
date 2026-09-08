// The accessor part below was written by tools/gen_accessors.py and is
// kept unchanged; Graphics::Renderable::AmendViewportVisibleMask at the
// foot was added by hand, so the generator's banner is gone --
// gen_units.py overwrites any file that still carries it.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.
//
// AmendViewportVisibleMask calls its own SetViewportVisibleMask through
// a POINTER TO MEMBER: the twelve-byte constant onto the stack, r12
// pointed at it, __ptmf_scall. The constant is at 806C1B98 and names the
// target -- delta 0, vtable offset -1, and a third word that is
// SetViewportVisibleMask's address at 8003D8F0, which is in another unit
// and is why the target is declared and not defined here.
//
// The mask is passed by value in r4, so the constant is copied through
// r7/r6 rather than the r6/r5 of the ones whose only arguments are
// floats. That it fits a GPR is all the bytes say about it; an enum is
// what it is written as, and with -enum int it is four bytes either way.
// Its name is the mangled one, Graphics::Viewport::VisibilityMask.

namespace World {

class ModelInstanceArticle {
public:
    int* GetModel();

    unsigned char _pad0[0x24];
    int f24;
};

}  // namespace World

namespace World {

class xOGModel {
public:
    int* GetModelArticle();

    unsigned char _pad0[0xC4];
    int fC4;
};

}  // namespace World

namespace Graphics {

class Viewport {
public:
    enum VisibilityMask { VisibilityMask_ = 0x7FFFFFFF };
};

class Renderable {
public:
    void SetViewportVisibleMask(Viewport::VisibilityMask mask);
    void AmendViewportVisibleMask(Viewport::VisibilityMask mask);
};

}  // namespace Graphics

int* World::ModelInstanceArticle::GetModel() { return &f24; }
int* World::xOGModel::GetModelArticle() { return &fC4; }

void Graphics::Renderable::AmendViewportVisibleMask(
    Viewport::VisibilityMask mask) {
    void (Renderable::*set)(Viewport::VisibilityMask) =
        &Renderable::SetViewportVisibleMask;

    (this->*set)(mask);
}
