// C:/branches/SB09/main/GM/Engine/Core/x/xOGRenderHelperInfo.cpp
//
//   mr   r4, r3          asset
//   lwz  r3, 0(r3)       modelPrototypeID, high word
//   lwz  r4, 4(r4)       modelPrototypeID, low word
//   bl   World::EntityManager::FindHandle(unsigned long long)
//   lwz  r3, 0x38(r3)
//
// The pair of loads into r3:r4 is a 64-bit value passed BY VALUE -- which
// is what identifies the argument as `modelPrototypeID`, the
// `unsigned long long` the DWARF puts at offset 0 of ModelInstanceAsset,
// and it agrees with FindHandle's own mangled `Ux`.
//
// r3 arrives holding the asset, not a `this`, so GetModelPrototypeEntity is
// static.

class ModelPrototypeEntity;

namespace World {

class ModelInstanceAsset {
public:
    unsigned long long modelPrototypeID;
};

struct EntityHandle {
    unsigned char _head[0x38];
    ModelPrototypeEntity* entity;
};

class EntityManager {
public:
    static EntityHandle* FindHandle(unsigned long long id);
};

}  // namespace World

class xOGRenderHelper {
public:
    static ModelPrototypeEntity* GetModelPrototypeEntity(
        const World::ModelInstanceAsset* asset);
};

ModelPrototypeEntity* xOGRenderHelper::GetModelPrototypeEntity(
    const World::ModelInstanceAsset* asset) {
    return World::EntityManager::FindHandle(asset->modelPrototypeID)->entity;
}
