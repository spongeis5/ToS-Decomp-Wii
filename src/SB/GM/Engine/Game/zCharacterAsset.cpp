// C:/branches/SB09/main/GM/Engine/Game/zCharacterAsset.cpp
//
// From the Wii build's DWARF (tools/dwarf_types.py):
//
//   class zCharacterAsset  /* 0xE0 */ { CharacterAssets _base0; };
//   class CharacterAssets  /* 0xE0 */ {
//       /* +0x0  */ ModelInstanceAsset ModelInstance;   // 0x40 bytes
//       ...
//       /* +0x88 */ __ExtraModels__ ExtraModels;        // { count, data }
//   };
//   class __ExtraModels__ /* 0x8 */ { unsigned int count; Pointer32 data; };
//
// so 0x8C is ExtraModels.data, and `slwi r0, r4, 6` is index * 0x40 --
// which is sizeof(ModelInstanceAsset) exactly. That is what identifies the
// element type; the shift alone would only have given the size.
//
//   lwz  r3, 0x8c(r3)
//   slwi r0, r4, 6
//   add  r3, r3, r0
//   blr
//
// Everything below 0x88 is stood in for by its bytes rather than declared.
// Naming it would pull in ModelInstanceAsset, uid, SoundSourcesPhysics and
// half a dozen more for a function that reads one field, and the offsets
// are what this needs to be right about.

struct ModelInstanceAsset {
    unsigned char _bytes[0x40];
};

struct __ExtraModels__ {
    unsigned int count;
    unsigned int data;
};

class zCharacterAsset {
public:
    ModelInstanceAsset* GetExtraModelAsset(unsigned int index);

    unsigned char _head[0x88];
    __ExtraModels__ ExtraModels;
};

ModelInstanceAsset* zCharacterAsset::GetExtraModelAsset(unsigned int index) {
    return &((ModelInstanceAsset*)ExtraModels.data)[index];
}
