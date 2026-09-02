// WAD00_11 -- one function, read from the image with tools/disasm.py:
// World::MaterialEntity's destructor. The class has no destructor of
// its own to speak of: the bytes are the compiler's -- the null-this
// test, the material member's destructor (hkBaseObject's, with the
// don't-delete flag), and operator delete when the flag says so. The
// layout is the DWARF's; the material sits at +0x2C and its base is
// the Havok object with the virtual destructor.

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

class EffectEntity;
class RenderModeEntity;

class MaterialEntity : public Entity {
public:
    ~MaterialEntity();

    int memSize;
    EffectEntity* effectEnt;
    RenderModeEntity* renderModeEnt;
    unsigned char paramCargo[0x8];
    // The bytes call hkBaseObject's destructor on this member directly,
    // with the don't-delete flag: whatever the DWARF names it, a class
    // of its own here gets a destructor of its own emitted and called.
    hkBaseObject material;
    unsigned char _pad1[0x28];
};

}  // namespace World

World::MaterialEntity::~MaterialEntity() {}
