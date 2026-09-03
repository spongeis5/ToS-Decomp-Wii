// zNPCAsset.cpp -- three functions, read from the image with
// tools/disasm.py. An NPC asset carries an entity asset inside it, and
// Prepare fills that entity asset's header from the NPC asset's own: the
// uid, widened when it still fits in 32 bits by moving its top byte into
// the high word, then base type 57 and the link count; then it takes the
// character asset (looking it up when the caller passed none) and copies
// that character's model prototype id into the entity asset's model
// instance, or zeroes it. GetModelAssetInfo returns the asset that model
// prototype id names, or null. GetCharacterAsset returns the asset the
// NPC asset's character id names, falling back to the character id at
// the head of the NPC template when the NPC asset has none.
//
// Layouts from the DWARF (tools/dwarf_types.py): xBaseAsset 0x10 with
// the uid at +0 and the link count at +0xC; xEntAsset 0x100 on that
// base with its model instance at +0x50, whose first member is the
// model prototype id; NPCAsset 0x190 with the entity asset at +0x10,
// the character asset id at +0x118 and the NPC template at +0x138.
// The NPC template's own first member is a uid, which is the character
// asset id the fallback reads.
//
// Four shapes the bytes fixed.
//
// FindAsset is a STATIC member reached through an object expression,
// World::GetEntityManager()->FindAsset, so the manager call is made
// and its result discarded -- that is retail's unused branch before
// each lookup. The uid widening is one expression over the whole
// 64-bit value, not a pair of word operations.
//
// Prepare's two tests are conditional EXPRESSIONS, not if
// statements. Retail branches to the else block and then jumps past
// it, a redundant pair that appears when the true side is the value
// already in hand; an if statement inverts the branch instead and
// costs a word, and so does an if whose then-branch is written
// empty, because the compiler folds that away.
//
// In GetCharacterAsset the template's uid is declared UNINITIALISED
// above the character id and only assigned inside the branch that
// reads it. Declaration order is what picks the callee-saved pair:
// declared second it takes r29:r28 and the character id r31:r30,
// which is the opposite of retail. Its lookup also keeps the uid in
// a local, so the argument is loaded before the manager call as
// retail has it, and its null return is the LAST block.
//
// GetModelAssetInfo reaches the same FindAsset through an address
// the compiler materialises (lis, addi, mtctr, bctrl) where every
// other call in the unit is a branch. A pointer to it folds back to
// a direct branch however it is spelled -- a local pointer, a
// pointer declared inside the branch, a cast through void*, a
// reference to function, a file-scope const pointer (24 of 26
// words), a volatile pointer (23), and a longcall pragma, which
// changed nothing at all. Casting the address through an unsigned
// int is what keeps it a runtime value. What retail's source said
// here is not recoverable from the bytes; what the bytes say is
// that the address was not a link-time constant to the compiler.

typedef unsigned long long uid;

class zCharacterAsset;

namespace Sext {

class NPCTemplate;
class ModelAssetInfo;

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class ModelInstanceAsset {
public:
    uid modelPrototypeID;
    unsigned char _pad0[0x38];
};

class xEntAsset : public xBaseAsset {
public:
    unsigned char _pad0[0x50 - 0x10];
    ModelInstanceAsset modelInstance;
    unsigned char _pad1[0x100 - 0x90];
};

class NPCAsset : public xBaseAsset {
public:
    xEntAsset EntAsset;
    unsigned char _pad0[0x118 - 0x110];
    uid ChrAssetID;
    unsigned char _pad1[0x138 - 0x120];
    uid NPCTemplate;
    unsigned char _pad2[0x190 - 0x140];
};

}  // namespace Sext

namespace World {

class EntityManager {
public:
    static Sext::xBaseAsset* FindAsset(uid id);
};

EntityManager* GetEntityManager();

}  // namespace World

// The template's head is the character asset it names.
namespace Sext {

class NPCTemplate {
public:
    uid ChrAssetID;
};

}  // namespace Sext

zCharacterAsset* zNPCAsset_GetCharacterAsset(const Sext::NPCAsset* asset);

void zNPCAsset_Prepare(Sext::NPCAsset* asset, zCharacterAsset* character) {
    uid id = asset->id;

    id = id > 0xFFFFFFFF
             ? id
             : (((id << 8) & 0xFF00000000ULL) | (id & 0x00FFFFFF));

    asset->EntAsset.id = id;
    asset->EntAsset.linkCount = asset->linkCount;
    asset->EntAsset.baseType = 57;

    character = character ? character : zNPCAsset_GetCharacterAsset(asset);

    if (character) {
        asset->EntAsset.modelInstance.modelPrototypeID =
            *(const uid*)character;
    } else {
        asset->EntAsset.modelInstance.modelPrototypeID = 0;
    }
}

Sext::ModelAssetInfo* zNPCAsset_GetModelAssetInfo(Sext::NPCAsset* asset) {
    Sext::xBaseAsset* found = 0;

    if (asset->EntAsset.modelInstance.modelPrototypeID != 0) {
        World::GetEntityManager();

        found = ((Sext::xBaseAsset* (*)(uid))(unsigned int)
                     World::EntityManager::FindAsset)(
            asset->EntAsset.modelInstance.modelPrototypeID);
    }

    if (found != 0) {
        return (Sext::ModelAssetInfo*)found;
    }

    return 0;
}

zCharacterAsset* zNPCAsset_GetCharacterAsset(const Sext::NPCAsset* asset) {
    uid templateID;
    uid id = asset->ChrAssetID;

    if (id == 0) {
        templateID = asset->NPCTemplate;

        Sext::NPCTemplate* tmpl = (Sext::NPCTemplate*)
            World::GetEntityManager()->FindAsset(templateID);

        if (tmpl) {
            id = tmpl->ChrAssetID;
        }
    }

    if (id != 0) {
        return (zCharacterAsset*)World::GetEntityManager()->FindAsset(id);
    }

    return 0;
}
