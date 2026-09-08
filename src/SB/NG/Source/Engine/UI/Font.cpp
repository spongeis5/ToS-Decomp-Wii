// The accessor part below was written by tools/gen_accessors.py and
// is kept unchanged; the constructors at the foot were added by
// hand, so the generator's banner is gone -- gen_units.py
// overwrites any file that still carries it.
//
// Every function here touches nothing but its own members or a
// constant: one load, one store, the address of a member, a
// constant return, or members set to one constant. Each body was
// decoded from the image and re-encoded back to the same bytes,
// and each parameter list re-mangled back to the same symbol,
// before being written. GENERATED, not read: real matched
// functions whose offsets are recovered fact, but a count of them
// is not a count of decompiled code.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.

namespace World {

class EntityHandleBase;

// The layout LightKitSceneEntity.cpp carries, from the DWARF: the
// type id at +0x10, which is what these two constructors set.
class Entity {
public:
    Entity(EntityHandleBase* handle);

    // A DECLARED destructor here would give every derived class an
    // implicit one of its own, emitted and not in retail -- three of
    // them, which is what mwcc produced first. Slot 0 is spelled as a
    // plain virtual instead: nothing here calls it, and the vtable is
    // referenced rather than emitted either way.
    virtual void _v0();
    virtual void _v1();

    unsigned char _pad0[0xC];
    unsigned int typeID;
    EntityHandleBase* handle;
};

class BlobEntity : public Entity {
public:
    BlobEntity(EntityHandleBase* handle);
};

}  // namespace World

namespace UI {

class Font : public World::Entity {
public:
    Font(World::EntityHandleBase* a0);
    virtual void __vtable_anchor();
};

class FontAssetBlobEntity : public World::BlobEntity {
public:
    FontAssetBlobEntity(World::EntityHandleBase* a0);
};

}  // namespace UI

namespace UI {

class TextureFont : public UI::Font {
public:
    virtual void __vtable_anchor();
    TextureFont(World::EntityHandleBase* a0);

};

}  // namespace UI

#pragma dont_inline on
UI::TextureFont::TextureFont(World::EntityHandleBase* a0) : UI::Font(a0) {}
#pragma dont_inline off

UI::Font::Font(World::EntityHandleBase* a0) : World::Entity(a0) {
    typeID = 14;
}

UI::FontAssetBlobEntity::FontAssetBlobEntity(World::EntityHandleBase* a0)
    : World::BlobEntity(a0) {
    typeID = 15;
}
