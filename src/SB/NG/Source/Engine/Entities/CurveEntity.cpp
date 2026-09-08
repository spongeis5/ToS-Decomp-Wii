// CurveEntity.cpp -- one function of the unit: World::CurveEntity's
// constructor. The rest is not written.

// The same 68-byte shape as the module constructors -- run the base,
// store the vtable, put one constant in one member -- but an entity
// rather than a module, and the vtable's OFFSET is what says so.
// World::Entity is polymorphic from +0, so the vptr lands at +0 and
// the constant is the type id at +0x10; System::Module declares two
// members ahead of its first virtual, so a module's vptr lands at
// +0x14 and the constant goes into events.stage[i].

namespace World {

class EntityHandleBase;

// The layout LightKitSceneEntity.cpp carries, from the DWARF: the type
// id at +0x10.
class Entity {
public:
    Entity(EntityHandleBase* handle);

    virtual void _v0();

    unsigned char _pad0[0xC];
    unsigned int typeID;
    EntityHandleBase* handle;
};

class CurveEntity : public Entity {
public:
    CurveEntity(EntityHandleBase* handle);
};

}  // namespace World

World::CurveEntity::CurveEntity(EntityHandleBase* handle)
    : World::Entity(handle) {
    typeID = 18;
}
