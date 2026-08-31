// C:/branches/SB09/main/NG/Source/Engine/AssetManager/Domains/Blobloids.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class Blobloid /* 0x20 */ {
//       /* +0x0  */ unsigned long long blobUID;
//       /* +0x8  */ int wmlTypeID;
//       /* +0xC  */ int blobSize;
//       /* +0x10 */ unsigned char subType;
//       /* +0x11 */ unsigned char blobFlags;
//       /* +0x12 */ unsigned short langID;
//       /* +0x14 */ union { MemHandle* memHandle; BlobCustomAlloc* customAlloc; };
//       /* +0x18 */ unsigned short domRefMaskList;
//       /* +0x1A */ unsigned short memOwnerRefMask;
//   };
//   class BlobAddArg /* 0x14 */ { wmlTypeID, blobSize, blobHandle, subType,
//                                 blobFlags, langID, customAlloc };
//   class MemHandle  /* 0x8  */ { lockFlags, memCBidx, mustAlign, entry };
//   class AllocEntry /* 0x18 */ { ... /* +0xC */ char* baseAddr; ... };
//   class World::EntityHandleBase /* 0x48 */ : Blobloid at +0.
//
// WHICH OF THESE ARE STATIC is decided by where the 64-bit uid lands, not
// by the mangled names -- CodeWarrior mangles static and non-static members
// alike. A `long long` argument takes an even/odd register pair, so:
//
//   FindHandle(uid)    takes it in r3:r4  -> nothing was passed before it
//                                            -> STATIC, and so is the
//                                               InsertBlobloid that hands
//                                               its own r3:r4 straight on
//   InsertHandle(uid)  takes it in r5:r6  -> r3 is a `this`, r4 skipped
//                                            for alignment -> NON-STATIC
//
// BlobCustomAlloc is 4 bytes: a vtable pointer and nothing else. The three
// slots used are vptr+8, +0xC and +0x10, which is the first, second and
// third virtual under CodeWarrior's two-word vtable header. Both Done and
// BlobData reach theirs with `bctr` rather than `bctrl` -- a tail call,
// which is only available because the return types line up.
//
// DoOverWrite's tail is `rlwinm r0, r3, 22, 26, 31` then `cntlzw`/`srwi 5`,
// which is a set-if-zero on ROTL32(langID, 22) & 0x3F -- that is the top six
// bits of the 16-bit field, i.e. langID >> 10. Its `return false` block sits
// LAST in retail, so the test has to be written the other way round.
//
// RemDomainRef took two measured steps and neither was guessable. Clearing
// domRefMaskList BEFORE memOwnerRefMask halved the difference; writing the
// complement back into the PARAMETER closed it. Retail's first two
// instructions are `nor r4, r4, r4` and `clrlwi r4, r4, 16` -- the
// complement lands in the register the argument arrived in, which a
// separate local does not do. Eight other spellings, including four with a
// local of each width, all stop at four words.

namespace World {
class EntityHandleBase;
}

namespace Domains {

class BlobCustomAlloc;

class AllocEntry {
public:
    unsigned char _head[0xC];
    char* baseAddr;
    int actualAmt;
};

class MemHandle {
public:
    unsigned char lockFlags;
    unsigned char memCBidx;
    unsigned short mustAlign;
    AllocEntry* entry;
};

class DynaMem {
public:
    static void Release(MemHandle* handle);
};

class BlobCustomAlloc {
public:
    virtual void Allocate(void* data, int size) = 0;
    virtual void Free() = 0;
    virtual void* GetData() const = 0;
};

class BlobAddArg {
public:
    int wmlTypeID;
    int blobSize;
    MemHandle* blobHandle;
    unsigned char subType;
    unsigned char blobFlags;
    unsigned short langID;
    BlobCustomAlloc* customAlloc;
};

class Blobloid {
public:
    void InitFromAddArg(BlobAddArg* addArg);
    void Done();
    void* BlobData() const;
    void CustomAllocate(BlobCustomAlloc* newCustAlloc);
    void AddDomainRef(unsigned int domRefMask);
    void RemDomainRef(unsigned int domRefMask);
    bool DoOverWrite(const BlobAddArg& addArg);

    unsigned long long blobUID;
    int wmlTypeID;
    int blobSize;
    unsigned char subType;
    unsigned char blobFlags;
    unsigned short langID;
    union {
        MemHandle* memHandle;
        BlobCustomAlloc* customAlloc;
    };
    unsigned short domRefMaskList;
    unsigned short memOwnerRefMask;
};

class DomainPriv {
public:
    void AddUIDItem(World::EntityHandleBase* handle);
};

class BlobGlob {
public:
    static bool InsertBlobloid(unsigned long long blobUID, BlobAddArg* addArg,
                               unsigned int domRefMask, DomainPriv* domain);
    static void RemoveBlobloid(World::EntityHandleBase* handle,
                               unsigned int domRefMask);
};

}  // namespace Domains

namespace World {

class EntityHandleBase : public Domains::Blobloid {
public:
    void InitForBlobloid(Domains::BlobAddArg* addArg);
    void RemoveEntityFromWorld();
};

class EntityManager {
public:
    static EntityHandleBase* FindHandle(unsigned long long blobUID);

    EntityHandleBase* InsertHandle(unsigned long long blobUID);
    void RemoveHandle(EntityHandleBase* handle);
};

EntityManager* GetEntityManager();

}  // namespace World

namespace Domains {

bool BlobGlob::InsertBlobloid(unsigned long long blobUID, BlobAddArg* addArg,
                              unsigned int domRefMask, DomainPriv* domain) {
    World::EntityHandleBase* handle = World::EntityManager::FindHandle(blobUID);
    bool inserted = false;

    if (handle == 0) {
        handle = World::GetEntityManager()->InsertHandle(blobUID);
        handle->InitForBlobloid(addArg);

        if (domain != 0) {
            domain->AddUIDItem(handle);
        }

        inserted = true;
    } else if (handle->DoOverWrite(*addArg)) {
        RemoveBlobloid(handle, domRefMask);

        handle = World::GetEntityManager()->InsertHandle(blobUID);
        handle->InitForBlobloid(addArg);

        inserted = true;
    }

    if (inserted) {
        handle->AddDomainRef(domRefMask);
    }

    return inserted;
}

void BlobGlob::RemoveBlobloid(World::EntityHandleBase* handle,
                              unsigned int domRefMask) {
    handle->RemDomainRef(domRefMask);

    if (handle->domRefMaskList == 0) {
        handle->RemoveEntityFromWorld();
        World::GetEntityManager()->RemoveHandle(handle);
    }
}

void Blobloid::InitFromAddArg(BlobAddArg* addArg) {
    wmlTypeID = addArg->wmlTypeID;
    blobSize = addArg->blobSize;
    memHandle = addArg->blobHandle;
    subType = addArg->subType;
    blobFlags = addArg->blobFlags;
    langID = addArg->langID;
    domRefMaskList = 0;

    if (addArg->customAlloc != 0) {
        CustomAllocate(addArg->customAlloc);
    }
}

void Blobloid::Done() {
    if (blobFlags & 2) {
        customAlloc->Free();
        return;
    }

    memHandle->lockFlags &= ~1;
    DynaMem::Release(memHandle);
}

void* Blobloid::BlobData() const {
    if (blobFlags & 2) {
        return customAlloc->GetData();
    }

    return memHandle->entry->baseAddr;
}

void Blobloid::CustomAllocate(BlobCustomAlloc* newCustAlloc) {
    newCustAlloc->Allocate(BlobData(), blobSize);

    memHandle->lockFlags &= ~1;
    DynaMem::Release(memHandle);

    customAlloc = newCustAlloc;
    blobFlags |= 2;
}

void Blobloid::AddDomainRef(unsigned int domRefMask) {
    domRefMaskList |= domRefMask;

    if (domRefMaskList == domRefMask) {
        memOwnerRefMask = domRefMaskList;
    }
}

void Blobloid::RemDomainRef(unsigned int domRefMask) {
    domRefMask = ~domRefMask;

    domRefMaskList &= domRefMask;
    memOwnerRefMask &= domRefMask;

    if (memOwnerRefMask != 0) {
        return;
    }

    if (domRefMaskList != 0) {
        unsigned short bit = 1;

        while (bit != 0x8000) {
            if (domRefMaskList & bit) {
                memOwnerRefMask = bit;
                return;
            }

            bit <<= 1;
        }
    }
}

bool Blobloid::DoOverWrite(const BlobAddArg& addArg) {
    if (addArg.langID != langID) {
        return (langID >> 10) == 0;
    }

    return false;
}

}  // namespace Domains
