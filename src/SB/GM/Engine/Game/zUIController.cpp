// zUIController.cpp -- four functions, read from the image with
// tools/disasm.py. A UI controller is a zUI with nothing of its own but a
// link array: DoInit runs the base DoInit and points the entity's link array
// at the asset's event links. The asset's Create takes one 256-byte block
// from the global heap (tag 16), clears it, places the entity on it -- the
// base zUI constructor and this class's vtable -- and runs zUI_Init.
// zUI's own constructor lives in this unit: it runs xOGEntity's, stores
// __vt__3zUI and constructs the two 0x40-byte State members, `current` and
// `startMovement`, in place. zUIController_NGLoadScreenInit is a free
// function that is nothing but a tail call to zUI_Init.
//
// Layouts from the DWARF (tools/dwarf_types.py): xBase 0x38 with linkArray at
// +0x28 -- its own last member, eventFunc, ends at +0x34 and the size is 0x38
// because the id at +0x18 is eight bytes; xOGEntity 0x40 with its model handle
// at +0x34, inside that tail padding; zUI 0x100 with UIViewportMask at +0x3C
// (inside xOGEntity's), the asset at +0x40, State current at +0x50 and State
// startMovement at +0x90; State 0x40 -- three Math::Vectors, a colour word, a
// brightness byte and three of padding, then eight more that come from
// Math::Vector's own alignment; Sext::zUI 0xB0 with its last member at +0xA0
// and Sext::UI_Controller 0xB0 too, its EventLinksNew at +0xA8 in the base's
// tail padding. The vtable slot count is not needed here: nothing in this unit
// calls through one, and both vtable stores are HA/LO relocations against the
// symbol.
//
// Four shapes the bytes fixed. The two State members are constructed by CALLS
// written in the constructor body, not by a State constructor of our own:
// retail branches to 0x800075C0, the shared empty `blr` the image names
// __ct__Q24Math8Matrix33Fv, and a State with an inline empty constructor emits
// a differently NAMED function that unitcmp's branch-target check would
// reject. zUIModel.cpp calls that symbol the same way. The allocation is a
// placement new on memset's return, whose OWN null test is the `cmpwi r3,0 ;
// mr r31,r3 ; beq` retail has -- the constructor and the vtable store sit
// inside it and zUI_Init runs after it either way. `Sext::zUI` reaches size
// 0xB0 with its members ending at 0xA8 because its transform is 16-aligned, so
// the derived EventLinksNew lands in the base's tail padding; a plainly padded
// 0xB0 base puts it at 0xB0 and DoInit's `addi r0,r3,168` becomes 176. And the
// first virtual of each class is DECLARED and never defined, so no vtable is
// emitted here and the homes stay in the unity unit's data -- which is also
// why this unit matches and does not link.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" {
void* memset(void* dst, int c, unsigned long n);

// The shared empty function every folded empty body branches to; it is what
// retail calls to construct each State.
void __ct__Q24Math8Matrix33Fv(void* state);
}

inline void* operator new(unsigned long, void* p) { return p; }

class LinkAsset {
public:
    unsigned int count;
    unsigned int data;
};
class TemplateEntity;
class xBase;
class zUI;
class zUIController;

namespace Sext {
class EventAny;
}

namespace World {
class EntityHandleBase;
}

namespace Graphics {

class Viewport {
public:
    enum VisibilityMask { VisibilityMask_ = 0x7FFFFFFF };
};

}  // namespace Graphics

namespace Math {

class Vector4 {
public:
    float x;
    float y;
    float z;
    float w;
};

class Vector : public Vector4 {};

}  // namespace Math

class xColor {
public:
    union {
        unsigned int rgbaU32;
        unsigned char rgbaU8s[4];
    };
};

// The DWARF's State, 0x40 bytes. The eight bytes after the padding are the
// tail the vector members' alignment leaves; nothing in this unit reads them.
class State {
public:
    Math::Vector position;
    Math::Vector scale;
    Math::Vector rotation;
    xColor color;
    unsigned char brightness;
    unsigned char pad[3];
    unsigned char UNUSED_tail[8];
};

namespace Sext {

class xBaseAsset {
public:
    unsigned long long id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

// The asset's transform is 16-aligned, so the class is 0xB0 with its own
// members ending at 0xA8 -- which is where the derived asset's links go.
class zUI : public xBaseAsset {
public:
    unsigned char Transform[0x30] __attribute__((aligned(16)));
    unsigned char _pad0[0xA8 - 0x40];
};

class UI_Controller : public zUI {
public:
    static zUIController* Create(World::EntityHandleBase* handle,
                                 UI_Controller* asset);

    LinkAsset EventLinksNew;
};

}  // namespace Sext

void zUI_Init(zUI* ui, Sext::zUI* asset);

class xBase {
public:
    virtual void _v0();

    unsigned char _pad0[0x14];
    unsigned long long id;
    unsigned int baseType;
    unsigned char UNUSED_linkCount;
    unsigned char assertFlags;
    unsigned short baseFlags;
    LinkAsset* linkArray;
    TemplateEntity* templateParent;
    void (*eventFunc)(xBase* from, xBase* to, unsigned int event,
                      Sext::EventAny* any);
};

namespace World {

class EntityHandleBase;
class xOGModel;
class xOGModelRefPtr;

class xOGModelRef {
public:
    xOGModel* data;
    xOGModelRefPtr* autoptr;
};

class xOGModelHandle : public xOGModelRef {};

class xOGEntity : public xBase {
public:
    xOGEntity(EntityHandleBase* handle);

    xOGModelHandle ogModel;
};

}  // namespace World

class zUI : public World::xOGEntity {
public:
    zUI(World::EntityHandleBase* handle);

    virtual void _v0();

    void DoInit();

    Graphics::Viewport::VisibilityMask UIViewportMask;
    Sext::zUI* asset;
    unsigned char _pad0[0x50 - 0x44];
    State current;
    State startMovement;
    unsigned char _pad1[0x100 - 0xD0];
};

class zUIController : public zUI {
public:
    zUIController(World::EntityHandleBase* handle) : zUI(handle) {}

    virtual void _v0();

    void DoInit();

    Sext::UI_Controller* controllerAsset() {
        return (Sext::UI_Controller*)asset;
    }
};

void zUIController::DoInit() {
    zUI::DoInit();

    linkArray = &controllerAsset()->EventLinksNew;
}

zUIController* Sext::UI_Controller::Create(World::EntityHandleBase* handle,
                                           UI_Controller* asset) {
    zUIController* ui = new (memset(
        Memory::AllocGlobalHeap(sizeof(zUIController),
                                (Memory::GlobalHeapEnum)0, (eMemMgrTag)16,
                                false),
        0, sizeof(zUIController))) zUIController(handle);

    zUI_Init(ui, asset);

    return ui;
}

zUI::zUI(World::EntityHandleBase* handle) : World::xOGEntity(handle) {
    __ct__Q24Math8Matrix33Fv(&current);        // State::State(), empty
    __ct__Q24Math8Matrix33Fv(&startMovement);  // State::State(), empty
}

void zUIController_NGLoadScreenInit(zUIController* ui,
                                    Sext::UI_Controller* asset) {
    zUI_Init(ui, asset);
}
