// WAD04.cpp -- one of the 154 functions the linker put in this unit.
// Graphics::Scene::AmendAddRenderable sits at 8018F7D0 between
// ImmediateRenderUIImage::AddToScene and parse_tag_tex, in the middle of
// xtextbox's tag parsers, because it is an inline that some translation
// unit in this run emitted out of line and this is where it landed.
// Nothing about it belongs to xtextbox.
//
// The body is its own AddRenderable called through a POINTER TO MEMBER:
// the twelve-byte constant is copied onto the stack, r12 is pointed at
// it, and __ptmf_scall does the call. The constant is in the image at
// 806C6FD4 and names the target -- delta 0, vtable offset -1, and a
// third word that is AddRenderable's address at 801CF720. The
// Renderable* passes through untouched in r4, which is why the constant
// is copied through r7/r6 rather than the r6/r5 of the ones whose only
// arguments are floats.
//
// The other 153 functions of the unit are not written.

namespace Graphics {

class Renderable;

class Scene {
public:
    void AddRenderable(Renderable* renderable);
    void AmendAddRenderable(Renderable* renderable);
};

}  // namespace Graphics

void Graphics::Scene::AmendAddRenderable(Renderable* renderable) {
    void (Scene::*add)(Renderable*) = &Scene::AddRenderable;

    (this->*add)(renderable);
}
