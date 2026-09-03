// CMeshBlobEntity.cpp -- six functions, read from the image with
// tools/disasm.py. A collision-mesh blob is an entity wrapping a Havok
// physics scene plus the game's own triangle-surface table. The asset's
// Create hands the blob to xHavok_LoadPhysicsData, which fills a root
// level container and a collision filter through out-parameters and
// returns the mesh header that follows the Havok data; it then takes one
// block from the global heap (tag 52) sized for the entity and one
// pointer per surface, hands it to a static stack allocator on the stack,
// places the entity on it, takes the surface table from it, looks up and
// activates a handle per surface uid, and finally points the entity's
// triangle data at the bytes after the header and copies the six AABB
// floats out of it. The constructor is the BlobEntity base, this class's
// vtable, a null container, a null owner list and type id 34.
// GetPhysicsSystem walks the container for the dataIndex'th hkpPhysicsData
// and returns one of its systems; GetPhysicsShape does the same by name
// for a shape. Deactivate is nothing but a call to Destroy through a
// pointer to member, and Destroy deactivates every surface handle and
// frees the block to the entity pool -- it does NOT run the destructor,
// where the other blob entities in this directory do.
//
// Layouts from the DWARF (tools/dwarf_types.py): Entity 0x18 with the
// type id at +0x10 and the handle at +0x14; BlobEntity 0x18, adding
// nothing; CollisionMeshBlobEntity 0x4C with dxTriMeshData at +0x18
// (AABBCenter[3], AABBExtents[3], TriangleInfo* triInfo -- 0x1C bytes),
// physicsDataContainer at +0x34, collFilter at +0x38, memBlock at +0x3C,
// numSurfaces at +0x40, surfaceTable at +0x44 and owners at +0x48, which
// is why Create's size is `n * 4 + 76` and Destroy's the same. That 0x4C
// is also why the placement new bumps the allocator's mark by 76.
// hkpPhysicsData 0x18 with hkArray<hkpPhysicsSystem*> m_systems at +0xC,
// whose m_data is at +0 -- the whole of retail's `lwz r3,12(r30)` before
// the indexed load. hkRootLevelContainer 0x8, EntityHandleBase 0x48,
// Util::Referrer one byte, StackAllocator {mark, buffer, end}. The
// destructor is declared and left undefined so the vtable's home stays in
// the unity unit that holds it, and BlobEntity's is declared for the same
// reason on top of another: an intermediate class with no declared
// destructor under a virtual one gets an 80-byte implicit destructor
// emitted and counted as EXTRA.
//
// The asset header xHavok_LoadPhysicsData RETURNS is not a named type in
// the DWARF; its shape is read off the loads. Eight bytes this unit never
// touches, then AABBCenter[3] at +8, AABBExtents[3] at +0x14,
// numSurfaces at +0x20 and the surface uid array at +0x24 -- 40 bytes,
// which is exactly the `addi r0,r31,40` that gives the entity its
// triangle data, so the triangles begin at `header + 1`. That the
// function returns it rather than parsing the argument in place is not a
// guess: r4 holds the asset before the call and nothing reloads it
// afterwards; every later read is off the returned r3.
//
// This unit reaches a POOLED string, "shape" at @stringBase0 + 32, so
// CMeshBlobEntity.pool.h (tools/gen_poolprefix.py) is included first to
// put the six strings WAD00.cpp's earlier files contributed in front of
// it. That table is data read from the image, not source.
//
// Five shapes the bytes fixed.
//
// The two zero stores of the constructor are attributed to the
// constructor's own line by the DWARF line table (tools/dwarf_lines.py:
// line 25 for the vtable AND for both, line 28 for the type id), which is
// where this producer puts a member initialiser list, so the container
// and the owner list are initialised there and only the id is a body
// statement.
//
// `Alloc` is inlined and the placement new's own null test is the only
// branch: `lwz mark; cmpwi; addi mark,76; stw mark; beq` around nothing
// but the constructor call, with the four member stores after it running
// on either path.
//
// `if (entity->numSurfaces > 0)` tests the MEMBER just stored, not the
// header field it was stored from. Retail compares without reloading --
// the store forwards -- and then reloads `header->numSurfaces` for the
// shift on the next line, which it would not need to do if the compare
// had read the header. Two reads of one value, and which object each one
// names is what puts the reload where retail has it.
//
// A surface handle is stored into the table and READ BACK for the test
// and the call: retail loads +0x44 twice and indexes it twice, where a
// named local for the lookup's result (which is what
// ImmediatePrototypeEntity wanted) would keep it in r3.
//
// Destroy frees `memBlock` and never touches `this`, and it runs no
// destructor: the count is recomputed from the member the loop condition
// has just reloaded, which is why `slwi r5,r0,2` reuses the loop's own
// register.
//
// hkpPhysicsDataClass's accessor cannot be named from the source. The
// image branches to `Get__28Pointer32<PQ24Sext8EventAny>CFv`, an eight-
// byte weak body that every one-load accessor folded onto -- Havok's
// `hkClass::getName` among them -- so what the original wrote is gone the
// way RawBlobEntity's `delete this` is. The call is made through the type
// the image names, the same declaration xWMLTypes.cpp already uses, so
// the relocation reaches the symbol retail's branch reaches.
//
// NEAR MISS, two of the seven, neither carried further -- the session
// that wrote this unit was cut off. Create is 53 of 99 words and
// GetPhysicsShape 32 of 34. Nothing here records a spelling that was
// tried and rejected, because none was.
//

#include "SB/NG/Source/Engine/Entities/Blobs/CMeshBlobEntity.pool.h"

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
enum EntityPoolEnum { EntityPool = 0 };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);

void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

class StaticStackAllocator {
public:
    void Create(void* buffer, int size);

    void* Alloc(int size) {
        void* p = mark;
        mark += size;
        return p;
    }

    unsigned char* mark;
    unsigned char* buffer;
    unsigned char* bufferEnd;
};

// The entity pool frees to the global heap; the image's instance of
// DeleteArray for it is a null test and this tail call.
inline void Free(void* block, EntityPoolEnum) {
    FreeGlobalHeap(block, (GlobalHeapEnum)0);
}

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

template <class H, class T>
void DeleteArray(const H& heap, T* array, unsigned long count) {
    if (array) {
        Memory::Free(array, heap);
    }
}

namespace Util {

// Empty, and passed as a named default-initialised local.
class Referrer {};

}  // namespace Util

typedef unsigned long long uid;

namespace Sext {
class EventAny;
}

// The weak one-load body every accessor of its shape folded onto; the
// image names the survivor after this instantiation, and it is how
// hkpPhysicsDataClass's type name is reached. Declared, never defined.
template <class T>
class Pointer32 {
public:
    T Get() const;
};

class hkpPhysicsSystem;
class hkpShape;

template <class T>
class hkArray {
public:
    T& operator[](int i) { return m_data[i]; }

    T* m_data;
    int m_size;
    int m_capacityAndFlags;
};

class hkpPhysicsData {
public:
    unsigned char _pad0[0xC];
    hkArray<hkpPhysicsSystem*> m_systems;
};

class hkRootLevelContainer {
public:
    void* findObjectByName(const char* name, const void* prev) const;
    void* findObjectByType(const char* typeName, const void* prev) const;

    void* m_namedVariants;
    int m_numNamedVariants;
};

// .bss, 0x30 bytes, global: Havok's hkClass for hkpPhysicsData. Only the
// folded accessor above is ever called on it.
extern Pointer32<Sext::EventAny*> hkpPhysicsDataClass;

class TriangleInfo;

class dxTriMeshData {
public:
    float AABBCenter[3];
    float AABBExtents[3];
    TriangleInfo* triInfo;
};

namespace World {

class Owner;

class EntityHandleBase {
public:
    void Activate(const Util::Referrer& referrer);
    void Deactivate(const Util::Referrer& referrer);
};

class EntityManager {
public:
    static EntityHandleBase* FindHandle(uid id);
};

class Entity {
public:
    Entity(EntityHandleBase* handle);

    virtual ~Entity();
    virtual void Deactivate();

    unsigned char _pad0[0xC];
    unsigned int typeID;
    EntityHandleBase* handle;
};

class BlobEntity : public Entity {
public:
    BlobEntity(EntityHandleBase* handle);

    // Declared and not defined: an intermediate class with none of its
    // own under a virtual destructor makes the compiler emit an implicit
    // one here, which retail does not have.
    virtual ~BlobEntity();
};

class CollisionMeshBlobEntity : public BlobEntity {
public:
    CollisionMeshBlobEntity(EntityHandleBase* handle);

    virtual ~CollisionMeshBlobEntity();
    virtual void Deactivate();

    hkpPhysicsSystem* GetPhysicsSystem(int sysIndex, int dataIndex) const;
    hkpShape* GetPhysicsShape(int index) const;

    void Destroy();

    dxTriMeshData triMesh;
    const hkRootLevelContainer* physicsDataContainer;
    int collFilter;
    unsigned char* memBlock;
    int numSurfaces;
    EntityHandleBase** surfaceTable;
    Owner* owners;
};

// The header xHavok_LoadPhysicsData returns, read off the loads: 40
// bytes, and the triangle data begins right after it.
class CollisionMeshBlobData {
public:
    unsigned char _pad0[0x8];
    float AABBCenter[3];
    float AABBExtents[3];
    int numSurfaces;
    uid* surfaceUIDs;
};

class CollisionMeshBlobAsset {
public:
    static CollisionMeshBlobEntity* Create(EntityHandleBase* handle,
                                           CollisionMeshBlobAsset* asset);
};

}  // namespace World

World::CollisionMeshBlobData* xHavok_LoadPhysicsData(
    const void* blob, const hkRootLevelContainer** container, int* collFilter);

World::CollisionMeshBlobEntity::CollisionMeshBlobEntity(
    EntityHandleBase* handle)
    : BlobEntity(handle), physicsDataContainer(0), owners(0) {
    typeID = 34;
}

hkpPhysicsSystem* World::CollisionMeshBlobEntity::GetPhysicsSystem(
    int sysIndex, int dataIndex) const {
    if (physicsDataContainer == 0) {
        return 0;
    }

    hkpPhysicsData* physicsData =
        (hkpPhysicsData*)physicsDataContainer->findObjectByType(
            (const char*)hkpPhysicsDataClass.Get(), 0);

    while (physicsData != 0 && dataIndex > 0) {
        physicsData = (hkpPhysicsData*)physicsDataContainer->findObjectByType(
            (const char*)hkpPhysicsDataClass.Get(), physicsData);
        dataIndex--;
    }

    if (physicsData != 0) {
        return physicsData->m_systems[sysIndex];
    }

    return 0;
}

hkpShape* World::CollisionMeshBlobEntity::GetPhysicsShape(int index) const {
    if (physicsDataContainer == 0) {
        return 0;
    }

    hkpShape* physicsShape =
        (hkpShape*)physicsDataContainer->findObjectByName("shape", 0);

    while (physicsShape != 0 && index > 0) {
        physicsShape =
            (hkpShape*)physicsDataContainer->findObjectByName("shape",
                                                              physicsShape);
        index--;
    }

    return physicsShape;
}

World::CollisionMeshBlobEntity* World::CollisionMeshBlobAsset::Create(
    EntityHandleBase* handle, CollisionMeshBlobAsset* asset) {
    const hkRootLevelContainer* havokRootContainer = 0;
    int collFilter;
    CollisionMeshBlobData* meshData;
    CollisionMeshBlobEntity* entity;
    int memSize;
    unsigned char* memBlock;
    uid* temp;
    int i;

    meshData = xHavok_LoadPhysicsData(asset, &havokRootContainer, &collFilter);

    memSize = meshData->numSurfaces * sizeof(EntityHandleBase*) +
              sizeof(CollisionMeshBlobEntity);
    memBlock = (unsigned char*)Memory::AllocGlobalHeap(
        memSize, (Memory::GlobalHeapEnum)0, (eMemMgrTag)52, false);

    Memory::StaticStackAllocator allocator;

    allocator.Create(memBlock, memSize);

    entity = new (allocator.Alloc(sizeof(CollisionMeshBlobEntity)))
        CollisionMeshBlobEntity(handle);

    if (havokRootContainer != 0) {
        entity->physicsDataContainer = havokRootContainer;
    }

    entity->memBlock = memBlock;
    entity->collFilter = collFilter;
    entity->numSurfaces = meshData->numSurfaces;

    if (entity->numSurfaces > 0) {
        entity->surfaceTable = (EntityHandleBase**)allocator.Alloc(
            meshData->numSurfaces * sizeof(EntityHandleBase*));
    } else {
        entity->surfaceTable = 0;
    }

    temp = meshData->surfaceUIDs;

    Util::Referrer referrer;

    for (i = 0; i < meshData->numSurfaces; i++) {
        entity->surfaceTable[i] = EntityManager::FindHandle(temp[i]);

        if (entity->surfaceTable[i] != 0) {
            entity->surfaceTable[i]->Activate(referrer);
        }
    }

    entity->triMesh.triInfo = (TriangleInfo*)(meshData + 1);

    entity->triMesh.AABBCenter[0] = meshData->AABBCenter[0];
    entity->triMesh.AABBCenter[1] = meshData->AABBCenter[1];
    entity->triMesh.AABBCenter[2] = meshData->AABBCenter[2];
    entity->triMesh.AABBExtents[0] = meshData->AABBExtents[0];
    entity->triMesh.AABBExtents[1] = meshData->AABBExtents[1];
    entity->triMesh.AABBExtents[2] = meshData->AABBExtents[2];

    return entity;
}

void World::CollisionMeshBlobEntity::Deactivate() {
    void (CollisionMeshBlobEntity::*destroy)() =
        &CollisionMeshBlobEntity::Destroy;

    (this->*destroy)();
}

void World::CollisionMeshBlobEntity::Destroy() {
    int i;

    Util::Referrer referrer;

    for (i = 0; i < numSurfaces; i++) {
        if (surfaceTable[i] != 0) {
            surfaceTable[i]->Deactivate(referrer);
        }
    }

    DeleteArray(Memory::EntityPool, memBlock,
                numSurfaces * sizeof(EntityHandleBase*) +
                    sizeof(CollisionMeshBlobEntity));
}
