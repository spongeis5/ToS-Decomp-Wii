// Model.cpp -- one of the nine functions the linker put in this unit,
// and it is not a Model. Graphics::Renderable3D::AmendLOD sits between
// Graphics::Model::AmendLODScale and Graphics::Model::DisableLOD at
// 801C7B70, and the DWARF says it was DEFINED IN Renderable3D.h at line
// 923: an inline some translation unit in this run emitted out of line,
// which landed here beside the two Model functions that call it.
//
// The body is its own SetLOD called through a POINTER TO MEMBER: the
// twelve-byte constant is copied onto the stack, r12 is pointed at it,
// and __ptmf_scall does the call. The constant is in the image at
// 806CD1C0 and names the target -- delta 0, vtable offset -1, and a
// third word that is SetLOD's address at 801C9DF0. The two floats pass
// through untouched, because __ptmf_scall adjusts r3 and jumps.
//
// The other eight functions of the unit are not written: Create,
// Destroy, CalcWorldChildTransforms, GetPartCount, ShowPart, HidePart,
// AmendLODScale and DisableLOD, 1,596 bytes between them.

namespace Graphics {

class Renderable3D {
public:
    void SetLOD(float lod, float scale);
    void AmendLOD(float lod, float scale);
};

}  // namespace Graphics

void Graphics::Renderable3D::AmendLOD(float lod, float scale) {
    void (Renderable3D::*set)(float, float) = &Renderable3D::SetLOD;

    (this->*set)(lod, scale);
}
