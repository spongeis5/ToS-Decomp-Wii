// WAD00_9 -- one function, read from the image with tools/disasm.py:
// World::LightKitEntity's destructor, the compiler's own -- the
// null-this test, the light kit member's destructor (hkBaseObject's,
// with the don't-delete flag) at +0x18, and operator delete when the
// flag says so. The layout is the DWARF's.

void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

namespace World {

class Entity {
public:
    unsigned char _pad0[0x18];
};

class TextureResourceEntity;

class LightKitEntity : public Entity {
public:
    ~LightKitEntity();

    // The bytes call hkBaseObject's destructor on this member directly,
    // with the don't-delete flag: whatever the DWARF names it, a class
    // of its own here gets a destructor of its own emitted and called.
    hkBaseObject lightKit;
    unsigned char _pad1[0x88];
    TextureResourceEntity* lookupTexEnt;
};

}  // namespace World

World::LightKitEntity::~LightKitEntity() {}
