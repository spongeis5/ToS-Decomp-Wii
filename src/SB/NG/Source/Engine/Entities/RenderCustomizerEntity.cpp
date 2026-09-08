// The accessor part below was written by tools/gen_accessors.py and is
// kept unchanged; the three Deactivate functions at the foot were added
// by hand, so the generator's banner is gone -- gen_units.py overwrites
// any file that still carries it.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.
//
// THIS UNIT HOLDS BOTH SPELLINGS OF THE SAME SENTENCE. DTRFrontBufferEntity
// says `DeferDestroy();` and is four bytes -- one branch. The three at the
// foot say the same thing through a POINTER TO MEMBER and are 68 bytes
// each: the twelve-byte constant is copied onto the stack, r12 is pointed
// at it, and __ptmf_scall does the call. The constant is in the image and
// says which member: delta 0, vtable offset -1, and a third word that is
// the address of that class's own Destroy.
//
// Retail's line table for each is seven rows over three lines -- the
// function's line, the CALL's line, the function's line again -- and the
// declaration's line has no row of its own, so the two-statement spelling
// is not merely permitted by the bytes, it is what the debug info records.
// CMeshBlobEntity.cpp's Deactivate, which is matched, produces those same
// seven rows from exactly this source.
//
// None of the three is declared virtual. Retail's Deactivate is an
// override, but declaring it so here would make each of these classes
// polymorphic with its first non-inline virtual defined in this unit, and
// mwcc would emit a vtable this unit does not have. The mangled name is
// the same either way, and nothing in the body reads a member.

namespace World { class EntityHandleBase; }

namespace World {

class Entity {
public:
    Entity(World::EntityHandleBase* a0);
    virtual void __vtable_anchor();
};

}  // namespace World

namespace World {

class RenderCustomizerBaseEntity : public World::Entity {
public:
    virtual void __vtable_anchor();
    RenderCustomizerBaseEntity(World::EntityHandleBase* a0);

};

}  // namespace World

namespace World {

class UVMovementSettingsEntity {
public:
    unsigned int GetInstanceDataSize();

    void Deactivate();
    void Destroy();
};

}  // namespace World

namespace World {

class DTRFrontBufferEntity {
public:
    void Deactivate();
    void DeferDestroy();

};

}  // namespace World

namespace World {

class TextureAnimationSettingsEntity {
public:
    void Deactivate();
    void Destroy();
};

class DTRMovieEntity {
public:
    void Deactivate();
    void Destroy();
};

}  // namespace World

#pragma dont_inline on
World::RenderCustomizerBaseEntity::RenderCustomizerBaseEntity(World::EntityHandleBase* a0) : World::Entity(a0) {}
unsigned int World::UVMovementSettingsEntity::GetInstanceDataSize() { return 0x00000060u; }
void World::DTRFrontBufferEntity::Deactivate() { DeferDestroy(); }
#pragma dont_inline off

void World::UVMovementSettingsEntity::Deactivate() {
    void (UVMovementSettingsEntity::*destroy)() =
        &UVMovementSettingsEntity::Destroy;

    (this->*destroy)();
}

void World::TextureAnimationSettingsEntity::Deactivate() {
    void (TextureAnimationSettingsEntity::*destroy)() =
        &TextureAnimationSettingsEntity::Destroy;

    (this->*destroy)();
}

void World::DTRMovieEntity::Deactivate() {
    void (DTRMovieEntity::*destroy)() = &DTRMovieEntity::Destroy;

    (this->*destroy)();
}
