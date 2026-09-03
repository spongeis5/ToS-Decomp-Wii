// RVLFaceLibEntity.cpp -- four functions, read from the image with
// tools/disasm.py.
//
//   World::RVLFaceLibAsset::Create takes 132 bytes (0x84, the DWARF's
//   size for the entity) from the global heap (heap 0, tag 48, no
//   clear), hands them to a static stack allocator on the stack, takes
//   the entity's 132 back from it through the inline Alloc, constructs
//   an RVLFaceLibEntity there when the block is not null, publishes it
//   into World::gRVLFaceLibEntity and returns it.  Its second
//   parameter, the asset, is NEVER READ: `li r4,0` overwrites r4 in the
//   first three instructions and nothing stores to mAsset.
//
//   The two constructors have the SAME body -- the default one passes a
//   null handle to the Entity base.
//
//   Math::Matrix43::MakeScale scales the three unit axes by the
//   argument and copies each result into a row.
//
// Layouts from the DWARF (tools/dwarf_types.py --type RVLFaceLibEntity):
// Entity 0x18, RVLFaceLibEntity 0x84 with the asset at +0x18, four
// 12-byte pool arrays at +0x1C/+0x28/+0x34/+0x40, mScaleMatrix at
// +0x4C, mTime at +0x7C and the three bools at +0x80..+0x82; Matrix43
// derives from Matrix33, which is three Vector4, each a nested
// DataType of four floats.  The two float constants are read out of the
// image: @125029 at 0x80692ADC is 0.0f and @131113 at 0x80692B70 is
// 0x3C4CCCCD, 0.0125f.
//
// MATCHED, 4 of the 4 functions the object defines, 0 extra.
//
// Five things this unit needed, each of them a lever already in NOTES:
//
//   1. vec4OneX, vec4OneY and vec4OneZ are in .bss in the image
//      (symbols.txt: 0x8077B418/28/38), not .rodata.  They are built at
//      runtime and are NOT const, so they are declared plain here -- the
//      Math-constants lever.
//   2. Each Vector4's one member is itself a class (DataType), which is
//      what makes `v[0] = x` a block move of four WORDS rather than a
//      memberwise lfs/stfs operator= emitted out of line.
//   3. Math::Mul takes its RESULT as its first parameter -- the whole
//      Math family does, `Mul(dst, a, b)` -- so MakeScale calls it into
//      a local and assigns the local afterwards.  Retail reads the copy
//      back out of the frame slot (40(r1), 24(r1), 8(r1)), which is what
//      a named local gives; assigning from the call's returned reference
//      would have copied through r3.
//   4. Declaration order picks the three frame slots: mwcc lays a
//      scope's locals out in reverse declaration order, so x, y, z land
//      at +40, +24, +8 -- the order retail uses them in.
//   5. The three bools store ASCENDING (128, 129, 130), so they are
//      three separate statements and not the `a = b = c = false` chain,
//      which would store descending.
//   6. And mTime is assigned AFTER them, though its member sits below
//      them: with `mTime = 0.0f;` written first the four stores come out
//      stfs, stb, stb, stb and every other word of both constructors is
//      already identical -- 4 of 29 and 4 of 28 words, all of them that
//      one instruction sliding three places.  Statement order survives
//      the scheduler here; member order does not decide it.
//
// Math::Matrix33's default constructor is DECLARED and not defined:
// retail calls it (0x800075C0, the weak lone `blr` every empty function
// folded onto), and an empty inline definition would be inlined away.
// Matrix43's own default constructor is the compiler's, so it is inlined
// down to that one call, which is exactly what the entity constructors
// have at this+0x4C.  MakeScale is `scope:weak` in the image, so it is
// defined in-class and emitted weak; being emitted at its first use is
// also what puts it between the two constructors, as retail has it.
// Entity's undefined virtual keeps the vtable's home in the WAD blob:
// this unit defines no virtual, so it emits no __vt and the
// constructors' store relocates against the image's own symbol.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);

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

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

namespace Math {

class Vector4 {
public:
    class DataType {
    public:
        float x;
        float y;
        float z;
        float w;
    };

    DataType data;
};

// These three are in .bss in the image, not .rodata: they are set up at
// runtime and are not const.  See the note at the top.
extern Vector4 vec4OneX;
extern Vector4 vec4OneY;
extern Vector4 vec4OneZ;

Vector4& Mul(Vector4& dst, const Vector4& src, float scale);

class Matrix33 {
public:
    // Declared, never defined: retail calls the image's weak lone blr.
    Matrix33();

    Vector4 v[3];
};

class Matrix43 : public Matrix33 {
public:
    void MakeScale(float scale) {
        Vector4 x;
        Vector4 y;
        Vector4 z;

        Mul(x, vec4OneX, scale);
        v[0] = x;
        Mul(y, vec4OneY, scale);
        v[1] = y;
        Mul(z, vec4OneZ, scale);
        v[2] = z;
    }
};

}  // namespace Math

namespace World {

class EntityHandleBase;
class RVLFaceLibAsset;

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

class Entity {
public:
    Entity(EntityHandleBase* handle);

    virtual void __key();

    EmbeddedListNode ogSceneNode;
    int ogUpdateIdx;
    unsigned int typeID;
    EntityHandleBase* handle;
};

// The DWARF gives these as three distinct 12-byte pool-array
// instantiations; nothing in this unit touches one, so only the size is
// load-bearing.
class PoolArray {
public:
    int size;
    void* pool;
    int poolSize;
};

class RVLFaceLibEntity : public Entity {
public:
    RVLFaceLibEntity();
    RVLFaceLibEntity(EntityHandleBase* handle);

    RVLFaceLibAsset* mAsset;
    PoolArray mRFLWorkBuffer;
    PoolArray mMiddleDBBuffer;
    PoolArray mCharacters;
    PoolArray mMatrices;
    Math::Matrix43 mScaleMatrix;
    float mTime;
    bool ready;
    bool renderOpaque;
    bool renderTransparent;
};

class RVLFaceLibAsset {
public:
    static RVLFaceLibEntity* Create(EntityHandleBase* handle,
                                    RVLFaceLibAsset* asset);
};

extern RVLFaceLibEntity* gRVLFaceLibEntity;

}  // namespace World

World::RVLFaceLibEntity* World::RVLFaceLibAsset::Create(
    World::EntityHandleBase* handle, World::RVLFaceLibAsset* asset) {
    Memory::StaticStackAllocator alloc;

    alloc.Create(Memory::AllocGlobalHeap(sizeof(World::RVLFaceLibEntity),
                                         (Memory::GlobalHeapEnum)0,
                                         (eMemMgrTag)48, false),
                 sizeof(World::RVLFaceLibEntity));

    World::RVLFaceLibEntity* entity =
        new (alloc.Alloc(sizeof(World::RVLFaceLibEntity)))
            World::RVLFaceLibEntity(handle);

    World::gRVLFaceLibEntity = entity;

    return entity;
}

World::RVLFaceLibEntity::RVLFaceLibEntity() : Entity(0) {
    ready = false;
    renderOpaque = false;
    renderTransparent = false;
    mTime = 0.0f;

    mScaleMatrix.MakeScale(0.0125f);
}

World::RVLFaceLibEntity::RVLFaceLibEntity(World::EntityHandleBase* handle)
    : Entity(handle) {
    ready = false;
    renderOpaque = false;
    renderTransparent = false;
    mTime = 0.0f;

    mScaleMatrix.MakeScale(0.0125f);
}
