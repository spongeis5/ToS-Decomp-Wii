// iCameraNG.cpp -- six functions, read from the image with
// tools/disasm.py. An iCamera is the game's camera seen by the NG
// renderer: it owns a viewport (or none, meaning "every 3D viewport of
// the screen view"), a field of view and a 4x3 frame. UpdateMatrix
// converts the frame into a Math::Matrix43 and hands it to the
// viewport's AmendCameraMatrix, or to every 3D viewport's when the
// camera has none; SetFOV keeps the angle, turns it into a vertical
// FOV through tan and hands that and the aspect ratio to
// AmendPerspProjection the same two ways; SetViewport records a
// viewport and re-runs UpdateMatrix for it; SetMatrix copies the frame
// in and does the same. Graphics::View::GetViewportCount and
// GetViewport are View.h inlines the unity build emitted here -- the
// DWARF puts both in View.h and the split puts them in this unit --
// and each walks the view's viewport list.
//
// Layouts from the DWARF (tools/dwarf_types.py): iCamera is 0x48 --
// camViewport +0, fov +4, xMat4x3 frame +8 -- and xMat4x3 is 0x40, the
// three xMat3x3 vectors at +0, +0x10 and +0x20 each with a padding word
// after it, then pos at +0x30 and pad3 at +0x3C. That is exactly the
// twelve offsets the Assign call reads: +8, +0xC, +0x10, then +0x18,
// +0x1C, +0x20, then +0x28, +0x2C, +0x30, then +0x38, +0x3C, +0x40 off
// `this`. Math::Matrix43 is 0x30 and DERIVES from Math::Matrix33, also
// 0x30 and adding nothing; Viewport's list node is at +0xC and its
// viewportIndex at +0x8C, which is why the count reads +128 off the
// NODE and GetViewport subtracts 12 to reach the viewport; View's
// viewport list is the Util::VoidList at +0x18, whose tail.next at
// +0x1C is the first node and whose own address is the end sentinel
// (the layout View.cpp already recovered).
//
// Five shapes the bytes fixed.
//
// `Math::Matrix43 ngMatrix;` compiles to `bl __ct__Q24Math8Matrix33Fv`
// -- the base's constructor, not its own. 0x800075C0 is the lone `blr`
// every empty function in the image folded onto, and dtk names it after
// Math::Matrix33's constructor; declaring Matrix43 with a constructor
// of its own would relocate against `__ct__Q24Math8Matrix43Fv` and name
// a symbol the image does not have there. Derived from Matrix33 with no
// constructor of its own, the implicit one is inlined to the base call
// and the relocation names what retail's displacement resolves to.
//
// The three float literals are 0.5, 0.75 and 4/3 (0x3F000000,
// 0x3F400000, 0x3FAAAAAB at 0x8067D450, 0x8067D47C and 0x8067D480), and
// retail gives each its own `lis`. A fragment compiled alone puts them
// at the head of .rodata, where mwcc shares one base for three or more;
// gen_poolprefix.py's padding array is what puts them past the 32 KB
// where the cost rule stops sharing.
//
// `tan` takes and returns a double, so the float argument crosses with
// no conversion and the result costs one `frsp` -- which is the
// `(float)` of `float view_window = tan(0.5f * fov);`. view_window
// lands in f0, the aspect ratio in f31 and the vertical FOV in f30,
// which is declaration order for the two that must survive the calls.
//
// GetViewport's hit test is `if (--n < 0) return viewport;`: `addic.
// r5,r5,-1` then `bltlr`, with the viewport pointer already formed in
// r3 by the scheduler. `n-- == 0` is a different sequence.
//
// The viewport pointer is never formed in GetViewportCount -- retail
// reads +128 off the list node, which is the -12 of the cast folded
// into the +0x8C of the member. Writing the cast per use is what gives
// that; the compiler does the arithmetic.
//
// MATCHED, 8 of 8 -- `python tools/unitcmp.py SB/GM/Engine/Core/x/
// iCameraNG`, 820 bytes. This file's record previously said 6 of 6 and
// the measurement said 3 of 7; the count here is the tool's, re-run,
// and the eight are the six the manifest names plus the two operator=
// functions the class shape below now emits. It is unitcmp's answer
// and not the oracle's: run ninja and read `main.dol: OK`.
//
// Three levers, from 3 of 7:
//
//   * SetFOV differed in two words, both operand order in a multiply
//     chain. Retail keeps the running product in one register --
//     `fmuls f30,f0,f31` then `fmuls f30,f30,f0` -- where the single
//     expression `vw * aspect * (4.0f/3.0f)` computes the first into
//     f1 and then multiplies the other way round. A COMPOUND
//     assignment (`fovy = vw * aspect; fovy *= 4.0f/3.0f;`) puts the
//     accumulated value in hand, which fixes both the register and the
//     operand order.
//
//   * xMat4x3 DERIVES from xMat3x3. This file used to say the flat
//     spelling was needed because the image has one operator= symbol;
//     it has both, and retail's __as__7xMat4x3FRC7xMat4x3 is 21
//     instructions whose fourth is `bl __as__7xMat3x3FRC7xMat3x3`
//     followed by four word copies of pos and pad3. Flat, mwcc
//     synthesised the whole 64-byte copy inline: 148 bytes against 84.
//     Derived, both operators come out byte-identical, and the base's
//     116 bytes are a function this unit did not have before.
//
//   * Both viewport-list walks had retail's shape exactly and differed
//     only in which register held the iterator and which the end
//     sentinel -- retail walks in r6 and compares against r5.
//     Declaring the sentinel BEFORE the iterator swaps the pair and
//     matched both functions at once.
#include "SB/GM/Engine/Core/x/iCameraNG.pool.h"

extern "C" double tan(double x);

namespace Util {

class NodeHeader {
public:
    NodeHeader* prev;
    NodeHeader* next;
};

class VoidList {
public:
    NodeHeader tail;
};

}  // namespace Util

namespace Math {

// 0x30 bytes and no constructor of its own: the one the image has at
// 0x800075C0 belongs to the base.
class Matrix33 {
public:
    Matrix33();

    unsigned char _pad0[0x30];
};

class Matrix43 : public Matrix33 {
public:
    void Assign(float m00, float m01, float m02, float m10, float m11,
                float m12, float m20, float m21, float m22, float m30,
                float m31, float m32);
};

}  // namespace Math

class xVec3 {
public:
    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xVec3 left;
    int flags;
    xVec3 up;
    unsigned int pad1;
    xVec3 at;
    unsigned int pad2;
};

// Derived, not flat. Retail's __as__7xMat4x3FRC7xMat4x3 calls the
// BASE's __as__7xMat3x3FRC7xMat3x3 and then copies pos and pad3 as
// four words; spelled flat it has to synthesise all sixteen inline,
// which is 148 bytes against retail's 84.
class xMat4x3 : public xMat3x3 {
public:
    xVec3 pos;
    unsigned int pad3;
};

namespace Graphics {

class Viewport {
public:
    enum ViewportIndex {
        VIEWPORT_3D = 0,
        VIEWPORT_UI = 1,
        VIEWPORT_SHADOWMAP = 2,
        VIEWPORT_COLLISION = 3,
        VIEWPORT_PROJECTSHADOW = 4,
        VIEWPORT_CLOUD = 5
    };

    void AmendCameraMatrix(const Math::Matrix43& mat);
    void AmendPerspProjection(float fovy, float aspectRatio);

    unsigned char _pad0[0xC];
    Util::NodeHeader listNode;
    unsigned char _pad1[0x78];
    ViewportIndex viewportIndex;
};

class View {
public:
    int GetViewportCount(Viewport::ViewportIndex index);
    Viewport* GetViewport(Viewport::ViewportIndex index, int n);

    unsigned char _pad0[0x18];
    Util::VoidList viewports;
};

View* HackGetScreenView();

}  // namespace Graphics

class iCamera {
public:
    void UpdateMatrix();
    void SetViewport(Graphics::Viewport* viewport);
    void SetFOV(float fov);
    void SetMatrix(const xMat4x3& mat);

    Graphics::Viewport* camViewport;
    float fov;
    xMat4x3 frame;
};

void iCamera::UpdateMatrix() {
    Math::Matrix43 ngMatrix;

    ngMatrix.Assign(frame.left.x, frame.left.y, frame.left.z, frame.up.x,
                    frame.up.y, frame.up.z, frame.at.x, frame.at.y, frame.at.z,
                    frame.pos.x, frame.pos.y, frame.pos.z);

    if (camViewport) {
        camViewport->AmendCameraMatrix(ngMatrix);
    } else {
        Graphics::View* view = Graphics::HackGetScreenView();
        int numViewports =
            view->GetViewportCount(Graphics::Viewport::VIEWPORT_3D);

        for (int i = 0; i < numViewports; i++) {
            Graphics::Viewport* viewport =
                view->GetViewport(Graphics::Viewport::VIEWPORT_3D, i);

            viewport->AmendCameraMatrix(ngMatrix);
        }
    }
}

int Graphics::View::GetViewportCount(Viewport::ViewportIndex index) {
    int count = 0;
    Util::NodeHeader* end = &viewports.tail;
    Util::NodeHeader* it = viewports.tail.next;

    for (; it != end; it = it->next) {
        if (index == ((Viewport*)((char*)it - 12))->viewportIndex) {
            count++;
        }
    }

    return count;
}

Graphics::Viewport* Graphics::View::GetViewport(Viewport::ViewportIndex index,
                                                int n) {
    Util::NodeHeader* end = &viewports.tail;
    Util::NodeHeader* it = viewports.tail.next;

    for (; it != end; it = it->next) {
        Viewport* viewport = (Viewport*)((char*)it - 12);

        if (index == viewport->viewportIndex) {
            if (--n < 0) {
                return viewport;
            }
        }
    }

    return 0;
}

void iCamera::SetViewport(Graphics::Viewport* viewport) {
    camViewport = viewport;

    if (viewport) {
        UpdateMatrix();
    }
}

void iCamera::SetFOV(float fovIn) {
    fov = fovIn;

    float view_window = tan(0.5f * fovIn);
    float fAspectRatio = 0.75f;

    float fovy = view_window * fAspectRatio;
    fovy *= 4.0f / 3.0f;

    if (camViewport) {
        camViewport->AmendPerspProjection(fovy, fAspectRatio);
    } else {
        Graphics::View* view = Graphics::HackGetScreenView();
        int numViewports =
            view->GetViewportCount(Graphics::Viewport::VIEWPORT_3D);

        for (int i = 0; i < numViewports; i++) {
            Graphics::Viewport* viewport =
                view->GetViewport(Graphics::Viewport::VIEWPORT_3D, i);

            viewport->AmendPerspProjection(fovy, fAspectRatio);
        }
    }
}

void iCamera::SetMatrix(const xMat4x3& mat) {
    frame = mat;

    UpdateMatrix();
}
