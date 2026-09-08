// The two accessors below were written by tools/gen_accessors.py and are
// kept unchanged; the eleven Amend functions at the foot were added by
// hand, so the generator's banner is gone -- gen_units.py overwrites any
// file that still carries it.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.
//
// EVERY Amend HERE CALLS ITS OWN Set THROUGH A POINTER TO MEMBER. The
// body is `__ptmf_scall` with a twelve-byte constant copied onto the
// stack in front of it, and the constant is in the image: delta 0,
// vtable offset -1, and a third word that is the address of
// Set<the same name>. Amend and Set differ in nothing but the
// indirection, and retail puts each Set immediately after its own Amend
// -- AmendRendering at 801D51A0 and SetRendering at 801D51F0,
// AmendLodEnabled at 801D5380 and SetLodEnabled at 801D53D0 -- so they
// were written as pairs.
//
// THE FLOAT ARGUMENTS DO NOT REACH THE BYTES AND THE OTHERS DO, in one
// place only: the scratch register pair. __ptmf_scall adjusts r3 and
// jumps, so every argument register is already in place and none is
// touched -- but the constant has to be copied through a register the
// arguments are not using, and the compiler takes the first free GPR.
// Nought, one, two, three and four floats all give r6/r5, because floats
// go in f1..f4 and take no GPR; one pointer, bool or enum gives r7/r6;
// AmendMoBlurSettings' const reference gives r7/r6 as well. That is the
// whole of the difference between the two shapes in this file, and it is
// why the twin census filed them as two clusters rather than one.
//
// The line table agrees with the two-statement spelling rather than
// merely permitting it: retail's rows for each of these are the
// function's own line, then the CALL's line, then the function's line
// again, and the DECLARATION's line has no row of its own -- the same
// seven rows over three lines that CMeshBlobEntity.cpp's Deactivate,
// which is matched, produces from exactly this source.
//
// SetRelativeCorner is the one target that does not live beside its
// caller (801A19E0, where the rest are in this unit's own 801D5xxx).
// SetRendering and SetLodEnabled are the two that are defined here.
// The other eight are declared and not defined: the address of a member
// function needs a declaration, and the relocation reaches the symbol
// retail's reaches.

namespace Math { class Matrix43; }

namespace Graphics {

class Viewport {
public:
    // Passed by value in a GPR, which is why AmendViewportVisibleMask
    // and the two bool ones share a shape.
    enum VisibilityMask { VisibilityMask_ = 0x7FFFFFFF };

    // Taken by const reference and never read here, so it stays
    // incomplete.
    class MoBlurSettings;

    void SetLodEnabled(bool value);
    void SetRendering(bool value);

    void SetCameraMatrix(const Math::Matrix43& camera);
    void SetFOVY(float fovy);
    void SetFogColor(float r, float g, float b);
    void SetFogStartEnd(float start, float end, unsigned int flags);
    void SetHeightFogStartEnd(float start, float end, unsigned int flags);
    void SetMoBlurSettings(const MoBlurSettings& settings);
    void SetPerspProjection(float fovy, float aspect);
    void SetRelativeCorner(float x, float y, float width, float height);
    void SetZPlanes(float nearZ, float farZ);

    void AmendCameraMatrix(const Math::Matrix43& camera);
    void AmendFOVY(float fovy);
    void AmendFogColor(float r, float g, float b);
    void AmendFogStartEnd(float start, float end, unsigned int flags);
    void AmendHeightFogStartEnd(float start, float end, unsigned int flags);
    void AmendLodEnabled(bool value);
    void AmendMoBlurSettings(const MoBlurSettings& settings);
    void AmendPerspProjection(float fovy, float aspect);
    void AmendRelativeCorner(float x, float y, float width, float height);
    void AmendRendering(bool value);
    void AmendZPlanes(float nearZ, float farZ);

    unsigned char _pad0[0x1C];
    unsigned char f1C;
    unsigned char f1D;
};

}  // namespace Graphics

void Graphics::Viewport::AmendRendering(bool value) {
    void (Viewport::*set)(bool) = &Viewport::SetRendering;

    (this->*set)(value);
}

void Graphics::Viewport::SetRendering(bool value) { f1D = value; }

void Graphics::Viewport::AmendCameraMatrix(const Math::Matrix43& camera) {
    void (Viewport::*set)(const Math::Matrix43&) = &Viewport::SetCameraMatrix;

    (this->*set)(camera);
}

void Graphics::Viewport::AmendPerspProjection(float fovy, float aspect) {
    void (Viewport::*set)(float, float) = &Viewport::SetPerspProjection;

    (this->*set)(fovy, aspect);
}

void Graphics::Viewport::AmendFOVY(float fovy) {
    void (Viewport::*set)(float) = &Viewport::SetFOVY;

    (this->*set)(fovy);
}

void Graphics::Viewport::AmendLodEnabled(bool value) {
    void (Viewport::*set)(bool) = &Viewport::SetLodEnabled;

    (this->*set)(value);
}

void Graphics::Viewport::SetLodEnabled(bool value) { f1C = value; }

void Graphics::Viewport::AmendZPlanes(float nearZ, float farZ) {
    void (Viewport::*set)(float, float) = &Viewport::SetZPlanes;

    (this->*set)(nearZ, farZ);
}

void Graphics::Viewport::AmendFogStartEnd(float start, float end,
                                          unsigned int flags) {
    void (Viewport::*set)(float, float, unsigned int) =
        &Viewport::SetFogStartEnd;

    (this->*set)(start, end, flags);
}

void Graphics::Viewport::AmendFogColor(float r, float g, float b) {
    void (Viewport::*set)(float, float, float) = &Viewport::SetFogColor;

    (this->*set)(r, g, b);
}

void Graphics::Viewport::AmendHeightFogStartEnd(float start, float end,
                                                unsigned int flags) {
    void (Viewport::*set)(float, float, unsigned int) =
        &Viewport::SetHeightFogStartEnd;

    (this->*set)(start, end, flags);
}

void Graphics::Viewport::AmendRelativeCorner(float x, float y, float width,
                                             float height) {
    void (Viewport::*set)(float, float, float, float) =
        &Viewport::SetRelativeCorner;

    (this->*set)(x, y, width, height);
}

void Graphics::Viewport::AmendMoBlurSettings(const MoBlurSettings& settings) {
    void (Viewport::*set)(const MoBlurSettings&) =
        &Viewport::SetMoBlurSettings;

    (this->*set)(settings);
}
