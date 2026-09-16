#include "SB/NG/Engine/WAD02_13.pool.h"

// WAD02_13: CHavokShapeBuilder (the scaled-shape cache) and Scaleform's
// custom text renderer, image creator, translator, command handlers and
// file opener.
//
// This library code carries no DWARF -- tools/brief.py knows none of these
// functions -- so every layout below is the recovered one from
// tools/dwarf_types.py and every body was read off tools/disasm.py alone.
//
// The function ORDER is not retail's, and in three places it is
// load-bearing: the weak copies retail CALLS (GColor's constructor,
// GString::GetData, hkpSingleShapeContainer's destructor) are defined BELOW
// their callers so -inline auto does not take them; the two destructors
// retail INLINES (GFxState's into GFxFSCommandHandler's,
// GRefCountBaseStatImpl's into GRefCountBase's) are defined ABOVE theirs so
// that it does; and the two constructors retail CALLS (GFxState's and
// GRefCountBaseStatImpl's) are defined below every constructor that calls
// them. The weak copies nothing written here calls are kept in the object
// by their addresses at the foot.
//
// No virtual function is DEFINED anywhere in this file: every one of them
// is a declaration, which is enough for mwcc to name the slots. That does
// NOT keep the vtables out of the object, and an earlier version of this
// comment said it did. objsyms.py reports this object defining 19 symbols
// outside .text, four of them weak vtables --
// __vt__23hkpSingleShapeContainer, __vt__Q29Scaleform20CustomCommandHandler,
// __vt__Q29Scaleform24ExternalInterfaceHandler and
// __vt__24GRefCountBaseStatImpl<2>. Those are exactly the four polymorphic
// classes for which this file defines a constructor or a destructor, and
// mwcc emits a class's vtable wherever it emits one of those. The split
// takes .text only and unitcmp compares .text, so it costs no row --
// WAD02_15_1 carries six of them -- but it is a real difference from
// retail's object and is recorded, not hidden.
//
// The accessor part below was written by tools/gen_accessors.py and
// is kept unchanged; the TextParams destructor at the foot was
// added by hand, so the generator's banner is gone -- gen_units.py
// overwrites any file that still carries it.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.

// Each class below stands in for one a member points at.
// Nothing NAMES that class -- these five words carry no
// relocation -- so it is named after where it was found,
// and holds virtuals only up to the slot that is called.
class hkpMoppBvTreeShape_m34 {
public:
    virtual void _v0() const;
    virtual void _v1() const;
    virtual void _v2() const;
    virtual void _v3() const;
    virtual void _v4() const;
};


class hkpMoppBvTreeShape {
public:
    void getContainer() const;

    unsigned char _pad0[0x34];
    int f34;
};


namespace Scaleform {

class CustomTextRenderer {
public:
    void SetTextChanged();

    unsigned char _pad0[0x4688];
    unsigned char f4688;
};

}  // namespace Scaleform

void hkpMoppBvTreeShape::getContainer() const { ((hkpMoppBvTreeShape_m34*)f34)->_v4(); }
void Scaleform::CustomTextRenderer::SetTextChanged() { f4688 = 1; }

void operator delete(void* mem);

// GFx's unsynchronised atomic: the raw word is read once for the value it
// returns and again for the sum it stores (WAD02_15_1).
class GAtomicInt {
public:
    volatile long Value;

    long ExchangeAdd_NoSync(long delta)
    {
        volatile unsigned long* p = (volatile unsigned long*)&Value;
        unsigned long old = *p;
        *p += delta;
        return old;
    }
};

// GFx's reference-count hook is NOT a vtable: the object holds a pointer at
// +4 to a pair of function pointers, and AddRef/Release are loaded from it
// with no slot offset (a virtual call here would be two loads and a +8).
class GRefCountImpl {
public:
    void (*AddRef)(void* pobj);
    void (*Release)(void* pobj, unsigned int flags);
};

class GRefCounted {
public:
    int RefCount;
    GRefCountImpl* pRefCountImpl;
};

// Scaleform's own string: one word, whose low two bits carry the heap type,
// pointing at a reference-counted buffer. Its destructor is defined below
// TextParams', which calls it.
class GString {
public:
    class DataDesc {
    public:
        unsigned long Size;
        GAtomicInt RefCount;
        char Data[1];
    };

    ~GString();
    DataDesc* GetData() const;

    union {
        DataDesc* pData;
        unsigned long HeapTypeBits;
    };
};

// The compiler's own destructor: the null-this test, the member at the
// offset below destroyed with the don't-delete flag, and operator
// delete when the CALLER's flag is positive, then `return this`. There
// is no second call, so nothing it derives from has a destructor.
class GFxDrawTextManager {
public:
    class TextParams {
    public:
        ~TextParams();

        unsigned char _pad0[0x14];
        GString text;
    };
};

GFxDrawTextManager::TextParams::~TextParams() {}

// The 80-byte base-only destructor, the compiler's own: the
// null-this test, the BASE's destructor on `this` with the flag
// CLEAR, then the delete when the CALLER's flag is positive, and
// `return this`.
//
// A GFx class DELETES THROUGH ITS OWN OPERATOR. Retail's branch here
// is Free__11GMemoryHeapFPv and not the global __dl__FPv, which is
// what reloc_audit caught when these were first written the ordinary
// way: every word was equal, report.json credited them, and the call
// went somewhere else. A one-line `operator delete` is taken by
// -inline auto at the call site, so the heap's Free lands in the
// destructor itself, which is what the bytes have.

class GMemoryHeap {
public:
    static void Free(void* p);
};

class GNewOverrideBase {
public:
    static void operator delete(void* p) { GMemoryHeap::Free(p); }
};

// Three words and then the vtable pointer at +0xC, which is where every
// constructor in this chain stores one. The single virtual is DECLARED and
// never defined, so no vtable is emitted here.
class GRefCountBaseImpl : public GNewOverrideBase {
public:
    GRefCountBaseImpl();
    ~GRefCountBaseImpl();

    void SetRefCountMode(unsigned int mode);

    int RefCount;
    GRefCountImpl* pRefCountImpl;
    void* pWeakProxy;

    virtual void _v0();
};

// The chain is four deep and only the ends of it are called: a destructor
// here that tests the null `this` TWICE off one cmpwi is one whose
// INTERMEDIATE destructor mwcc took in line -- the second test is that
// inlined body's own. GRefCountBaseStatImpl's goes in line in
// GRefCountBase's, and GFxState's in GFxFSCommandHandler's; each is
// defined above the one that takes it, and the ones retail CALLS are
// defined below their callers.
template <int N>
class GRefCountBaseStatImpl : public GRefCountBaseImpl {
public:
    GRefCountBaseStatImpl();
    ~GRefCountBaseStatImpl();
};

template <class T, int N>
class GRefCountBase : public GRefCountBaseStatImpl<N> {
public:
    ~GRefCountBase();
};

class GFxState : public GRefCountBase<GFxState, 2> {
public:
    enum StateType {
        State_None = 0,
        State_RenderConfig = 1,
        State_RenderStats = 2,
        State_Translator = 3,
        State_Log = 4,
        State_ImageLoader = 5,
        State_ActionControl = 6,
        State_UserEventHandler = 7,
        State_FSCommandHandler = 8,
        State_ExternalInterface = 9,
        State_FileOpener = 10,
        State_URLBuilder = 11,
        State_ImageCreator = 12
    };

    GFxState(StateType state);
    ~GFxState();

    StateType SType;
};

class GFile : public GRefCountBase<GFxState, 2> {
public:
    ~GFile();
};

// mwcc will not inline a base constructor: a constructor defined here for
// a class whose base is one of these intermediates CALLS the intermediate's
// constructor (naming the wrong symbol) and emits it out of line, which
// retail's image does not have anywhere. So these two carry destructors
// only, and the three constructors whose construction chain would need the
// intermediate skipped -- GFxState's, GFile's and GFxFileOpener's -- are
// left unwritten rather than written wrong. Measured: with them in, the
// unit read 43 of 56 (one extra match, four EXTRA rows and four
// constructors one relocation wrong); without them, 45 of 49.
class GFxFSCommandHandler : public GFxState {
public:
    ~GFxFSCommandHandler();
};

class GFxFileOpener : public GFxFSCommandHandler {
public:
    ~GFxFileOpener();
};

GFxFileOpener::~GFxFileOpener() {}

GFxState::~GFxState() {}

GFile::~GFile() {}

// Takes GFxState's destructor, defined above, in line.
GFxFSCommandHandler::~GFxFSCommandHandler() {}

template <> GRefCountBaseStatImpl<2>::~GRefCountBaseStatImpl() {}

// Takes GRefCountBaseStatImpl's destructor, defined above, in line.
template <> GRefCountBase<GFxState, 2>::~GRefCountBase() {}

// ---------------------------------------------------------------------------
// GString, below the TextParams destructor that calls it.
//
// The buffer's count is read twice and decremented twice -- the count it
// tests is the one ExchangeAdd_NoSync returns, the one it stores is the
// re-read -- and the buffer goes back to the heap, while the string itself
// goes through the GLOBAL operator delete: GString derives from nothing.

GString::~GString()
{
    DataDesc* pdesc = GetData();

    if ((pdesc->RefCount.ExchangeAdd_NoSync(-1) - 1) == 0) {
        GMemoryHeap::Free(pdesc);
    }
}

// Below its caller: retail calls it out of line.
GString::DataDesc* GString::GetData() const
{
    return (DataDesc*)(HeapTypeBits & ~3);
}

// ---------------------------------------------------------------------------
// Havok values

class Dummy;
class hkpWorldObject;

typedef unsigned long hkUlong;
typedef unsigned long long hkUint64;

enum HK_MEMORY_CLASS {
    HK_MEMORY_CLASS_BASE_CLASS = 22,
    HK_MEMORY_CLASS_ARRAY = 24,
    HK_MEMORY_CLASS_MAP = 29,
    HK_MEMORY_CLASS_SHAPE = 40
};

class hkBool {
public:
    hkBool() {}
    hkBool(bool b) { m_bool = (char)b; }

    operator bool() const { return m_bool != 0; }

    char m_bool;
};

class hkVector4 {
public:
    void operator=(const hkVector4& v);
    void mul4(const hkVector4& a);

    // NEAR MISS: 8 of 17 words differ -- retail keeps y in f1 and z in f2,
    // this keeps y in f2 and z in f1, and every fabs, frsp and store
    // follows. Tried: the four results through locals declared x, y, z, w
    // (identical diff). It is the FIRST function of retail's fragment,
    // where compiler state no spelling reaches has cost a register choice
    // before (BRIEF.md).
    void setAbs4(const hkVector4& v)
    {
        x = (float)__fabs(v.x);
        y = (float)__fabs(v.y);
        z = (float)__fabs(v.z);
        w = (float)__fabs(v.w);
    }

    float x __attribute__((aligned(16)));
    float y;
    float z;
    float w;
};

class hkTransform {
public:
    hkTransform(const hkTransform& t);

    hkVector4 m_rotation[3];
    hkVector4 m_translation;
};

namespace Math {

class Vector4 {
public:
    float x;
    float y;
    float z;
    float w;
};

void Mul(Vector4& out, const Vector4& a, const Vector4& b);

}  // namespace Math

class hkMemory;

// WAD00's spelling, which put m_stack at +0x10 there: the virtuals come
// before the data members, and onStackUnderflow is slot 6. deallocateStack
// is NOT an inline here -- mwcc left it out of line and called it, which
// retail does not have in this object, so its three statements are written
// out at the one call site.
class hkThreadMemory {
public:
    class Stack {
    public:
        char* m_current;
        Stack* m_prev;
        char* m_base;
        char* m_end;
    };

    virtual void* alignedAllocate(int alignment, int nbytes, HK_MEMORY_CLASS cl);
    virtual void alignedDeallocate(void* p);
    virtual void setStackArea(void* buf, int nbytes);
    virtual void releaseCachedMemory();
    virtual ~hkThreadMemory();
    virtual void* onStackOverflow(int nbytes);
    virtual void onStackUnderflow(void* p);

    static hkThreadMemory& getInstance();

    void* allocateChunk(int nbytes, HK_MEMORY_CLASS cl);
    void deallocateChunk(void* p, int nbytes, HK_MEMORY_CLASS cl);
    void deallocateChunkConstSize(void* p, int nbytes, HK_MEMORY_CLASS cl);

    hkMemory* m_memory;
    int m_referenceCount;
    unsigned char _padC[0x10 - 0xC];
    Stack m_stack;
};

extern hkThreadMemory* hkThreadMemory__s_threadMemoryInstance;

inline hkThreadMemory& hkThreadMemory::getInstance()
{
    return *hkThreadMemory__s_threadMemoryInstance;
}

class hkReferencedObject {
public:
    void addReference() const;
    void removeReference() const;

    unsigned char _pad0[4];
    unsigned short m_memSizeAndFlags;
    short m_referenceCount;
};

extern const float hkConvexShapeDefaultRadius;

// The shape kinds scaleShape switches on, as the jump table at 0x806CEE9C
// orders them: 13 entries, four of them the default.
enum hkpShapeType {
    HK_SHAPE_SPHERE = 1,
    HK_SHAPE_CYLINDER = 2,
    HK_SHAPE_BOX = 4,
    HK_SHAPE_CAPSULE = 5,
    HK_SHAPE_CONVEX_VERTICES = 6,
    HK_SHAPE_LIST = 9,
    HK_SHAPE_MOPP = 10,
    HK_SHAPE_CONVEX_TRANSLATE = 11,
    HK_SHAPE_CONVEX_TRANSFORM = 12
};

// A shape is allocated and given back at memory class 40, and its size is
// the one it was allocated with. operator new writes that size through the
// pointer BEFORE the new-expression's null test, which is what the image
// has; neither has a null test of its own.
class hkpShape : public hkReferencedObject {
public:
    static void* operator new(unsigned long nbytes)
    {
        void* p = hkThreadMemory::getInstance().allocateChunk((int)nbytes, HK_MEMORY_CLASS_SHAPE);
        ((hkReferencedObject*)p)->m_memSizeAndFlags = (unsigned short)nbytes;
        return p;
    }

    static void operator delete(void* p)
    {
        hkThreadMemory::getInstance().deallocateChunk(
            p, ((hkpShape*)p)->m_memSizeAndFlags, HK_MEMORY_CLASS_SHAPE);
    }

    void* m_userData;
    int m_type;
};

template <class T>
class hkArray;

class hkpShapeShrinker {
public:
    class ShapePair {
    public:
        hkpShape* originalShape;
        hkpShape* newShape;
    };

    static hkpShape* shrinkByConvexRadius(hkpShape* shape, hkArray<ShapePair>* pairs);
};

class hkArrayUtil {
public:
    static void _reserveMore(void* array, int sizeElem);
};

// hkArray's own. The destructor is DECLARED here and defined only for the
// shrinker's pair array: the other instantiations' copies live elsewhere in
// the image, and defining the template here would emit them.
//
// isEmpty returns an hkBool, a one-byte class, so the emptiness test is a
// VALUE (cntlzw, srwi, extsb.) and not a branch on the compare -- which is
// what collectGarbage has.
template <class T>
class hkArray {
public:
    enum {
        CAPACITY_MASK = int(0x3FFFFFFF),
        DONT_DEALLOCATE_FLAG = int(0x80000000)
    };

    ~hkArray();

    void operator delete(void* p)
    {
        if (p) {
            hkThreadMemory::getInstance().deallocateChunkConstSize(p, sizeof(hkArray<T>),
                                                                   HK_MEMORY_CLASS_ARRAY);
        }
    }

    int getCapacity() const { return m_capacityAndFlags & CAPACITY_MASK; }
    int dontDeallocate() const { return m_capacityAndFlags & DONT_DEALLOCATE_FLAG; }
    hkBool isEmpty() const { return m_size == 0; }

    T& expandOne();
    void removeAt(int index);

    T* m_data;
    int m_size;
    int m_capacityAndFlags;
};

template <class T>
T& hkArray<T>::expandOne()
{
    if (m_size == getCapacity()) {
        hkArrayUtil::_reserveMore(this, sizeof(T));
    }

    return m_data[m_size++];
}

// NEAR MISS: 7 of 13 words, the right length and the right tail branch --
// retail holds index*32 in r0 and the new size*32 in r3 where this holds
// them in r4 and r0, and forms the source address after the load rather
// than before it. Four spellings measured: both ends through references
// (this one, 52 B, 7 of 13); the new size through a local with the element
// indexed from it (60 B, 13 of 13); the source spelled twice with no local
// (52 B, 13 of 13, reloading m_size and m_data); and naming only the
// destination so the source element is loaded before its address is formed
// -- BRIEF.md's naming lever, which made it WORSE, 68 B and 15 of 13.
// Only the ScaledShape array instantiates it.
template <class T>
void hkArray<T>::removeAt(int index)
{
    m_size--;

    T& dst = m_data[index];
    T& src = m_data[m_size];

    dst.m_shape = src.m_shape;
    dst.m_scale = src.m_scale;
}

// The count the deallocation is sized from is the RAW field, not
// getCapacity(): in this branch the flag bit is known clear, and the image
// has no mask. The flag test goes through an accessor so the field is read
// TWICE, as retail reads it.
template <>
hkArray<hkpShapeShrinker::ShapePair>::~hkArray()
{
    if (dontDeallocate() == 0) {
        hkThreadMemory::getInstance().deallocateChunk(
            m_data, m_capacityAndFlags * sizeof(hkpShapeShrinker::ShapePair),
            HK_MEMORY_CLASS_ARRAY);
    }
}

// The list shape's scratch array: its elements come off the thread stack,
// and its base destructor is the one the linker kept, hkArray<hkpWorldObject*>'s
// -- so the base is spelled as that instantiation and only DECLARED, which
// is what makes the branch name retail's symbol without emitting a copy.
template <class T>
class hkLocalArray : public hkArray<hkpWorldObject*> {
public:
    ~hkLocalArray();

    void operator delete(void* p)
    {
        if (p) {
            hkThreadMemory::getInstance().deallocateChunkConstSize(p, sizeof(hkLocalArray<T>),
                                                                   HK_MEMORY_CLASS_ARRAY);
        }
    }

    void* m_stackMem;
};

// deallocateStack written out: as an inline member mwcc emitted it and
// called it, and retail has no such function in this object. The pointer is
// a local, so the underflow call passes the one already in the register
// rather than re-reading the member.
template <>
hkLocalArray<hkpShape*>::~hkLocalArray()
{
    void* p = m_stackMem;
    hkThreadMemory& mem = hkThreadMemory::getInstance();

    mem.m_stack.m_current = (char*)p;

    if (mem.m_stack.m_current == mem.m_stack.m_base) {
        mem.onStackUnderflow(p);
    }
}

// An iterator is the element INDEX, carried in a pointer. The empty key is
// the all-ones word, which mwcc tests with addis + cmplwi.
template <class K, class V>
class hkPointerMapOperations {};

template <class K, class V, class OPS>
class hkPointerMapBase {
public:
    class Pair {
    public:
        K key;
        V val;
    };

    void clear();
    void remove(Dummy* it);

    Pair* m_elem;
    int m_numElems;
    int m_hashMod;
};

template <class K, class V>
class hkPointerMap {
public:
    typedef hkPointerMapBase<hkUlong, hkUlong, hkPointerMapOperations<hkUlong, hkUlong> > Storage;

    Dummy* getIterator() const;
    Dummy* getNext(Dummy* it) const;
    hkBool isValid(Dummy* it) const;

    void clear() { m_map.clear(); }
    void remove(Dummy* it) { m_map.remove(it); }

    Storage m_map;
};

template <class K, class V>
Dummy* hkPointerMap<K, V>::getIterator() const
{
    int i = 0;

    for (; i <= m_map.m_hashMod; i++) {
        if (m_map.m_elem[i].key != hkUlong(-1)) {
            break;
        }
    }

    return (Dummy*)i;
}

template <class K, class V>
Dummy* hkPointerMap<K, V>::getNext(Dummy* it) const
{
    int i = (int)it + 1;

    for (; i <= m_map.m_hashMod; i++) {
        if (m_map.m_elem[i].key != hkUlong(-1)) {
            break;
        }
    }

    return (Dummy*)i;
}

// isValid lost its name to linker folding: the address the branch names is
// the collision-mesh map's copy, so it is called through a cast to that
// instantiation. Neither of these two classes is otherwise used here.
namespace World {
class CollisionMeshBlobEntity;
}

class xHavokPhysicsObject {
public:
    class PhysicsJointRelativeTransforms;
};

typedef hkPointerMap<const World::CollisionMeshBlobEntity*,
                     xHavokPhysicsObject::PhysicsJointRelativeTransforms*>
    FoldedIsValidMap;

// ---------------------------------------------------------------------------
// Havok shapes

class hkpShapeContainer {
public:
    enum ReferencePolicy { REFERENCE_POLICY_IGNORE = 0, REFERENCE_POLICY_INCREMENT = 1 };

    virtual void _v0();
};

// NEAR MISS: 22 of 21 words, ours 104 B against retail's 84. Retail's
// destructor ignores the delete flag entirely -- no operator-delete branch,
// and only r31 saved. Tried: the ordinary form (the global operator
// delete), a sized `operator delete(void*, unsigned long)` (mwcc called it
// with 8), and a private one (mwcc emitted the branch anyway).
class hkpSingleShapeContainer : public hkpShapeContainer {
public:
    ~hkpSingleShapeContainer();

    hkpShape* m_childShape;
};

class hkpConvexShape : public hkpShape {
public:
    ~hkpConvexShape();

    float m_radius;
};

class hkpConvexTransformShapeBase : public hkpConvexShape {
public:
    hkpConvexTransformShapeBase(hkpShapeType type, float radius, const hkpConvexShape* childShape,
                                hkpShapeContainer::ReferencePolicy policy);
    ~hkpConvexTransformShapeBase();

    hkpSingleShapeContainer m_childShape;
    int m_childShapeSize;
};

class hkpConvexTransformShape : public hkpConvexTransformShapeBase {
public:
    hkpConvexTransformShape(const hkpConvexShape* childShape, const hkTransform& transform,
                            hkpShapeContainer::ReferencePolicy policy);

    hkTransform m_transform;
};

class hkpBoxShape : public hkpConvexShape {
public:
    hkpBoxShape(const hkVector4& halfExtents, float radius);

    hkVector4 m_halfExtents;
};

class hkpSphereShape : public hkpConvexShape {
public:
    hkpSphereShape(float radius);

    unsigned char _pad14[0x20 - 0x14];
};

class hkpCapsuleShape : public hkpConvexShape {
public:
    hkpCapsuleShape(const hkVector4& vertexA, const hkVector4& vertexB, float radius);

    hkVector4 m_vertexA;
    hkVector4 m_vertexB;
};

class hkpCylinderShape : public hkpConvexShape {
public:
    hkpCylinderShape(const hkVector4& vertexA, const hkVector4& vertexB, float cylRadius,
                     float radius);

    hkVector4 m_vertexA;
    hkVector4 m_vertexB;
    unsigned char _pad40[0x60 - 0x40];
};

class hkpConvexTranslateShape;
class hkpConvexVerticesShape;
class hkpListShape;

// The cylinder's own radius accessor lost its name to linker folding: the
// address the branch names is GMatrix2D::GetY's, so it is called through
// that name.
class GMatrix2D {
public:
    float GetY() const;
};

// The compiler's own: the member container with the don't-delete flag, the
// base with the flag clear, then the shape's own operator delete.
hkpConvexTransformShapeBase::~hkpConvexTransformShapeBase() {}

// Below its caller: retail calls it out of line.
hkpSingleShapeContainer::~hkpSingleShapeContainer()
{
    if (m_childShape) {
        m_childShape->removeReference();
    }
}

// ---------------------------------------------------------------------------
// CHavokShapeBuilder

class CHavokShapeBuilder {
public:
    class ScaledShape {
    public:
        hkpShape* m_shape;
        hkVector4 m_scale;
    };

    void Release();
    void collectGarbage(unsigned int flags);
    hkpShape* scaleShape(const hkpShape* shape, const hkVector4& scale);
    hkpShape* getUncachedShape(const hkpShape* shape, const hkVector4& scale);
    hkpShape* scaleBoxShape(const hkpBoxShape* shape, const hkVector4& scale);
    hkpShape* scaleSphereShape(const hkpSphereShape* shape, const hkVector4& scale);
    hkpShape* scaleCapsuleShape(const hkpCapsuleShape* shape, const hkVector4& scale);
    hkpShape* scaleCylinderShape(const hkpCylinderShape* shape, const hkVector4& scale);
    hkpShape* scaleTranslateShape(const hkpConvexTranslateShape* shape, const hkVector4& scale);
    hkpShape* scaleConvexTransformShape(const hkpConvexTransformShape* shape,
                                        const hkVector4& scale);
    hkpShape* scaleMoppShape(const hkpMoppBvTreeShape* shape, const hkVector4& scale);
    hkpShape* scaleConvexVerticesShape(const hkpConvexVerticesShape* shape,
                                       const hkVector4& scale);
    hkpShape* scaleListShape(const hkpListShape* shape, const hkVector4& scale);

    unsigned char _pad0[8];
    hkPointerMap<const hkpShape*, hkArray<ScaledShape>*> m_shapeToGroupMap;
    hkArray<hkpShapeShrinker::ShapePair> m_shrunkenShapes;
};

typedef hkArray<CHavokShapeBuilder::ScaledShape> ScaledShapeArray;

class CollectedTriangle {
public:
    int index;
    hkVector4 verts[3];
    float dist;
};

// The shape cache's own timer. m_running_flag is an hkBool, a char, which
// is why the elapsed-time test sign-extends it.
class hkStopwatch {
public:
    hkStopwatch(const char* name);

    void start();
    void stop();
    float getElapsedSeconds() const;

    static hkUint64 getTickCounter();
    static hkUint64 getTicksPerSecond();
    static float divide64(hkUint64 a, hkUint64 b);

    hkUint64 m_ticks_at_start;
    hkUint64 m_ticks_total;
    hkUint64 m_ticks_at_split;
    hkUint64 m_split_total;
    hkBool m_running_flag;
    int m_num_timings;
    const char* m_name;
};

hkStopwatch::hkStopwatch(const char* name)
{
    m_name = name;
    m_ticks_at_start = 0;
    m_ticks_total = 0;
    m_ticks_at_split = 0;
    m_split_total = 0;
    m_running_flag = 0;
    m_num_timings = 0;
}

void hkStopwatch::start()
{
    m_running_flag = 1;
    m_ticks_at_split = m_ticks_at_start = getTickCounter();
}

void hkStopwatch::stop()
{
    m_running_flag = 0;

    hkUint64 now = getTickCounter();

    m_ticks_total += now - m_ticks_at_start;
    m_split_total += now - m_ticks_at_split;
    m_num_timings++;
}

float hkStopwatch::getElapsedSeconds() const
{
    hkUint64 ticks = m_ticks_total;

    if (m_running_flag) {
        ticks += getTickCounter() - m_ticks_at_start;
    }

    return divide64(ticks, getTicksPerSecond());
}

// The largest of the three components, a file static of the unity build.
static float _maxElement(const hkVector4& v)
{
    float m = v.x;

    if (v.y > m) {
        m = v.y;
    }

    if (v.z > m) {
        m = v.z;
    }

    return m;
}

// NEAR MISS: 22 of 80 words and the RIGHT length. The walk, the reverse
// scan, both reference-count tests and the written-out delete are all in
// place; every remaining word is callee-saved register colouring (this in
// r30 where retail has r29, the group in r27 where retail has r30, the
// scaled index in r29 where retail has r28). The group array's destructor
// and its operator delete are written out because retail has them in line
// here and defining hkArray's destructor for this instantiation would emit
// a copy this object does not have.
void CHavokShapeBuilder::collectGarbage(unsigned int flags)
{
    Dummy* it = m_shapeToGroupMap.getIterator();

    while (((const FoldedIsValidMap*)&m_shapeToGroupMap)->isValid(it)) {
        ScaledShapeArray* group = (ScaledShapeArray*)m_shapeToGroupMap.m_map.m_elem[(int)it].val;
        const hkpShape* shape = (const hkpShape*)m_shapeToGroupMap.m_map.m_elem[(int)it].key;

        for (int i = group->m_size - 1; i >= 0; i--) {
            hkpShape* scaled = group->m_data[i].m_shape;

            if (scaled->m_referenceCount == 1) {
                scaled->removeReference();
                group->removeAt(i);
            }
        }

        bool removeEntry = false;

        if (group->isEmpty()) {
            if (shape->m_referenceCount == 1) {
                removeEntry = true;
            }
        }

        if (removeEntry) {
            if (group) {
                if (group->dontDeallocate() == 0) {
                    hkThreadMemory::getInstance().deallocateChunk(
                        group->m_data,
                        group->m_capacityAndFlags * sizeof(CHavokShapeBuilder::ScaledShape),
                        HK_MEMORY_CLASS_ARRAY);
                }

                if (group) {
                    hkThreadMemory::getInstance().deallocateChunkConstSize(
                        group, sizeof(ScaledShapeArray), HK_MEMORY_CLASS_ARRAY);
                }
            }

            m_shapeToGroupMap.remove(it);
        }

        it = m_shapeToGroupMap.getNext(it);
    }
}

void CHavokShapeBuilder::Release()
{
    collectGarbage(1);
    m_shapeToGroupMap.clear();
    m_shrunkenShapes.m_size = 0;
}

hkpShape* CHavokShapeBuilder::getUncachedShape(const hkpShape* shape, const hkVector4& scale)
{
    hkpShape* scaled = scaleShape(shape, scale);
    hkpShape* shrunk = hkpShapeShrinker::shrinkByConvexRadius(scaled, 0);

    if (shrunk) {
        scaled->removeReference();
        scaled = shrunk;
    }

    return scaled;
}

// The case order below is the block order of the image; the jump table at
// 0x806CEE9C is what gives each case its value.
hkpShape* CHavokShapeBuilder::scaleShape(const hkpShape* shape, const hkVector4& scale)
{
    hkpShape* result = 0;

    switch (shape->m_type) {
    case HK_SHAPE_CONVEX_TRANSLATE:
        result = scaleTranslateShape((const hkpConvexTranslateShape*)shape, scale);
        break;

    case HK_SHAPE_CONVEX_TRANSFORM:
        result = scaleConvexTransformShape((const hkpConvexTransformShape*)shape, scale);
        break;

    case HK_SHAPE_MOPP:
        result = scaleMoppShape((const hkpMoppBvTreeShape*)shape, scale);
        break;

    case HK_SHAPE_CONVEX_VERTICES:
        result = scaleConvexVerticesShape((const hkpConvexVerticesShape*)shape, scale);
        break;

    case HK_SHAPE_CYLINDER:
        result = scaleCylinderShape((const hkpCylinderShape*)shape, scale);
        break;

    case HK_SHAPE_BOX:
        result = scaleBoxShape((const hkpBoxShape*)shape, scale);
        break;

    case HK_SHAPE_SPHERE:
        result = scaleSphereShape((const hkpSphereShape*)shape, scale);
        break;

    case HK_SHAPE_CAPSULE:
        result = scaleCapsuleShape((const hkpCapsuleShape*)shape, scale);
        break;

    case HK_SHAPE_LIST:
        result = scaleListShape((const hkpListShape*)shape, scale);
        break;
    }

    return result;
}

hkpShape* CHavokShapeBuilder::scaleConvexTransformShape(const hkpConvexTransformShape* shape,
                                                        const hkVector4& scale)
{
    hkpShape* child = scaleShape(shape->m_childShape.m_childShape, scale);
    hkTransform transform(shape->m_transform);
    hkVector4 translation;

    Math::Mul((Math::Vector4&)translation, (const Math::Vector4&)transform.m_translation,
              (const Math::Vector4&)scale);
    transform.m_translation = translation;

    hkpShape* result = new hkpConvexTransformShape((const hkpConvexShape*)child, transform,
                                                   hkpShapeContainer::REFERENCE_POLICY_INCREMENT);

    child->removeReference();
    return result;
}

hkpShape* CHavokShapeBuilder::scaleBoxShape(const hkpBoxShape* shape, const hkVector4& scale)
{
    hkVector4 halfExtents;

    halfExtents = shape->m_halfExtents;
    halfExtents.mul4(scale);

    return new hkpBoxShape(halfExtents, hkConvexShapeDefaultRadius);
}

// The radius is read into a saved FPU register BEFORE the call, so it is a
// statement of its own: spelled as one expression mwcc calls first.
hkpShape* CHavokShapeBuilder::scaleSphereShape(const hkpSphereShape* shape, const hkVector4& scale)
{
    float radius = shape->m_radius;

    radius = radius * _maxElement(scale);

    return new hkpSphereShape(radius);
}

hkpShape* CHavokShapeBuilder::scaleCapsuleShape(const hkpCapsuleShape* shape,
                                                const hkVector4& scale)
{
    float radius = shape->m_radius;
    hkVector4 vertexA;
    hkVector4 vertexB;

    radius = radius * _maxElement(scale);

    Math::Mul((Math::Vector4&)vertexA, (const Math::Vector4&)scale,
              (const Math::Vector4&)shape->m_vertexA);
    Math::Mul((Math::Vector4&)vertexB, (const Math::Vector4&)scale,
              (const Math::Vector4&)shape->m_vertexB);

    return new hkpCapsuleShape(vertexA, vertexB, radius);
}

hkpShape* CHavokShapeBuilder::scaleCylinderShape(const hkpCylinderShape* shape,
                                                 const hkVector4& scale)
{
    float radius = ((const GMatrix2D*)shape)->GetY() * _maxElement(scale);
    hkVector4 vertexA;
    hkVector4 vertexB;

    Math::Mul((Math::Vector4&)vertexA, (const Math::Vector4&)scale,
              (const Math::Vector4&)shape->m_vertexA);
    Math::Mul((Math::Vector4&)vertexB, (const Math::Vector4&)scale,
              (const Math::Vector4&)shape->m_vertexB);

    return new hkpCylinderShape(vertexA, vertexB, radius, hkConvexShapeDefaultRadius);
}

// ---------------------------------------------------------------------------
// GFx values

// The channels are stored red, green, blue, alpha -- alpha is the byte at
// +0 and the constructor writes it last.
class GColor {
public:
    class Rgb32 {
    public:
        unsigned char Alpha;
        unsigned char Red;
        unsigned char Green;
        unsigned char Blue;
    };

    GColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

    union {
        Rgb32 Channels;
        unsigned long Raw;
    };
};

class GViewport {
public:
    GViewport(int bufferWidth, int bufferHeight, int left, int top, int width, int height,
              unsigned int flags);

    int BufferWidth;
    int BufferHeight;
    int Left;
    int Top;
    int Width;
    int Height;
    int ScissorLeft;
    int ScissorTop;
    int ScissorWidth;
    int ScissorHeight;
    float Scale;
    float AspectRatio;
    unsigned int Flags;
};

GViewport::GViewport(int bufferWidth, int bufferHeight, int left, int top, int width, int height,
                     unsigned int flags)
{
    BufferWidth = bufferWidth;
    BufferHeight = bufferHeight;
    Left = left;
    Top = top;
    Width = width;
    Height = height;
    Flags = flags;
    ScissorLeft = ScissorTop = ScissorWidth = ScissorHeight = 0;
    Scale = AspectRatio = 1.0f;
}

// Its only member is the vtable pointer: the bag asks its implementation
// for itself (slot 0) and sets the state on it (slot 2).
class GFxStateBagImpl {
public:
    virtual void _v0();
    virtual void _v1();
    virtual void SetState(GFxState::StateType state, GFxState* pstate);
};

class GFxStateBag {
public:
    virtual GFxStateBagImpl* GetStateBagImpl() const;

    void SetState(GFxState::StateType state, GFxState* pstate);
};

void GFxStateBag::SetState(GFxState::StateType state, GFxState* pstate)
{
    GFxStateBagImpl* pimpl = GetStateBagImpl();

    if (pimpl) {
        pimpl->SetState(state, pstate);
    }
}

// ---------------------------------------------------------------------------
// GPtr. Three of its four members are needed here, each for a different
// instantiation, so they are instantiated one member at a time rather than
// by the class: instantiating the class would emit the members retail does
// not have in this object.

class GFxDrawText : public GRefCounted {};
class GImageInfo : public GRefCounted {};

template <class C>
class GPtr {
public:
    GPtr(C* pobj);
    ~GPtr();

    GPtr<C>& operator=(C& pobj);
    GPtr<C>& operator=(const GPtr<C>& other);

    C* pObject;
};

template <class C>
GPtr<C>::GPtr(C* pobj)
{
    if (pobj) {
        pobj->pRefCountImpl->AddRef(pobj);
    }

    pObject = pobj;
}

template <class C>
GPtr<C>::~GPtr()
{
    if (pObject) {
        pObject->pRefCountImpl->Release(pObject, 0);
    }
}

// Takes the object by REFERENCE and does not add a count: only the one it
// drops is released.
template <class C>
GPtr<C>& GPtr<C>::operator=(C& pobj)
{
    if (pObject) {
        pObject->pRefCountImpl->Release(pObject, 0);
    }

    pObject = &pobj;
    return *this;
}

template <class C>
GPtr<C>& GPtr<C>::operator=(const GPtr<C>& other)
{
    if (other.pObject) {
        other.pObject->pRefCountImpl->AddRef(other.pObject);
    }

    if (pObject) {
        pObject->pRefCountImpl->Release(pObject, 0);
    }

    pObject = other.pObject;
    return *this;
}

// ---------------------------------------------------------------------------
// Scaleform's text renderer and handlers

class GFxMovieView_data {
public:
    unsigned char _pad0[0xC];
};

// Only the slot the external-interface handler calls back on matters; the
// vtable pointer sits at +0xC, after the three words of the data-only base.
class GFxMovieView : public GFxMovieView_data {
public:
    virtual void _v0();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void _v7();
    virtual void _v8();
    virtual void _v9();
    virtual void _v10();
    virtual void _v11();
    virtual void _v12();
    virtual void _v13();
    virtual void _v14();
    virtual void _v15();
    virtual void _v16();
    virtual void _v17();
    virtual void _v18();
    virtual void _v19();
    virtual void _v20();
    virtual void _v21();
    virtual void _v22();
    virtual void _v23();
    virtual void _v24();
    virtual void _v25();
    virtual void _v26();
    virtual void _v27();
    virtual void _v28();
    virtual void _v29();
    virtual void _v30();
    virtual void _v31();
    virtual void _v32();
    virtual void _v33();
    virtual void _v34();
    virtual void _v35();
    virtual void _v36();
    virtual void _v37();
    virtual void _v38();
    virtual void _v39();
    virtual void _v40();
    virtual void _v41();
    virtual void _v42();
    virtual void _v43();
    virtual void _v44();
    virtual void _v45();
    virtual void _v46();
    virtual void SetExternalInterfaceRetVal(const class GFxValue& v);
};

// 24 bytes: only the type word is initialised, and the frame the caller
// reserves is 16 bytes larger than the word alone would need.
class GFxValue {
public:
    GFxValue() { Type = 0; }

    unsigned int Type;
    unsigned char _pad4[0x18 - 4];
};

namespace Scaleform {

class CustomTextRendererFontParams {
public:
    CustomTextRendererFontParams();

    CustomTextRendererFontParams& operator=(const CustomTextRendererFontParams& p);

    char* fontName;
    float fontSize;
    unsigned char fontColorRed;
    unsigned char fontColorGreen;
    unsigned char fontColorBlue;
    unsigned char fontColorAlpha;
    float autoScaleDownMinSize;
    bool allowAutoScaleDown;
};

// The file the opener hands back: the valid flag is the top bit of the
// byte at +0x24, so it is the first bitfield declared there.
class GSFMemoryFile {
public:
    void setValid(bool v);

    int RefCount;
    GRefCountImpl* pRefCountImpl;
    unsigned char _pad8[0x10 - 8];
    char* filename;
    void* filemem;
    int fileIndex;
    int fileSize;
    int headerSize;
    unsigned char valid : 1;
};

class MovieRec;

// What a movie record's +4 points at: the FS command in slot 0 and the
// external-interface call in slot 1.
class MovieCallbacks {
public:
    virtual void FSCommand(MovieRec* movie, const char* pcommand, const char* parg);
    virtual void ExternalInterface(GFxValue* presult, MovieRec* movie, const char* methodName,
                                   const GFxValue* pargs, unsigned int argCount);
};

class MovieRec {
public:
    unsigned char _pad0[4];
    MovieCallbacks* handler;
};

class Coordinator {
public:
    MovieRec* GetMovie(GFxMovieView* pmovieView);
};

class CustomCommandHandler : public GFxState {
public:
    CustomCommandHandler(Coordinator* c);

    void Callback(GFxMovieView* pmovieView, const char* pcommand, const char* parg);

    Coordinator* coord;
};

class ExternalInterfaceHandler : public GFxState {
public:
    ExternalInterfaceHandler(Coordinator* c);

    void Callback(GFxMovieView* pmovieView, const char* methodName, const GFxValue* pargs,
                  unsigned int argCount);

    Coordinator* coord;
};

}  // namespace Scaleform

// --------------------------------------------------------- GFx constructors
//
// Each stores ONE vtable pointer and calls ONE base constructor, and mwcc
// never inlines that call -- so each class written here derives DIRECTLY
// from the class whose constructor retail's listing names.
// GRefCountBaseStatImpl's is defined at the end, below its caller.

Scaleform::CustomCommandHandler::CustomCommandHandler(Coordinator* c)
    : GFxState(State_FSCommandHandler)
{
    coord = c;
}

Scaleform::ExternalInterfaceHandler::ExternalInterfaceHandler(Coordinator* c)
    : GFxState(State_ExternalInterface)
{
    coord = c;
}

template <> GRefCountBaseStatImpl<2>::GRefCountBaseStatImpl() {}

// --------------------------------------------------------------------------

Scaleform::CustomTextRendererFontParams::CustomTextRendererFontParams()
{
    fontName = 0;
    fontSize = 28.0f;
    fontColorRed = 255;
    fontColorGreen = 255;
    fontColorBlue = 255;
    fontColorAlpha = 255;
    autoScaleDownMinSize = 7.0f;
    allowAutoScaleDown = false;
}

// Member by member: the four colour bytes are loaded and stored one byte
// at a time, which a flat copy of the 20 bytes would not do.
Scaleform::CustomTextRendererFontParams& Scaleform::CustomTextRendererFontParams::operator=(
    const CustomTextRendererFontParams& p)
{
    fontName = p.fontName;
    fontSize = p.fontSize;
    fontColorRed = p.fontColorRed;
    fontColorGreen = p.fontColorGreen;
    fontColorBlue = p.fontColorBlue;
    fontColorAlpha = p.fontColorAlpha;
    autoScaleDownMinSize = p.autoScaleDownMinSize;
    allowAutoScaleDown = p.allowAutoScaleDown;
    return *this;
}

void Scaleform::GSFMemoryFile::setValid(bool v)
{
    valid = v && filemem != 0 && fileSize > headerSize;
}

void Scaleform::CustomCommandHandler::Callback(GFxMovieView* pmovieView, const char* pcommand,
                                               const char* parg)
{
    MovieRec* movie = coord->GetMovie(pmovieView);

    movie->handler->FSCommand(movie, pcommand, parg);
}

void Scaleform::ExternalInterfaceHandler::Callback(GFxMovieView* pmovieView,
                                                   const char* methodName, const GFxValue* pargs,
                                                   unsigned int argCount)
{
    MovieRec* movie = coord->GetMovie(pmovieView);
    GFxValue result;

    movie->handler->ExternalInterface(&result, movie, methodName, pargs, argCount);
    pmovieView->SetExternalInterfaceRetVal(result);
}

// Below its callers: retail calls it out of line.
GColor::GColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    Channels.Red = r;
    Channels.Green = g;
    Channels.Blue = b;
    Channels.Alpha = a;
}

// ---------------------------------------------------------------------------
// The weak copies the image has that only UNWRITTEN functions in this
// fragment call. Without a use mwcc emits none of them; the explicit
// instantiations and the addresses here keep them in the object, and
// nothing reads these pointers.

template GPtr<GFxDrawText>::GPtr(GFxDrawText*);
template GPtr<GFxDrawText>::~GPtr();
template GPtr<GImageInfo>& GPtr<GImageInfo>::operator=(GImageInfo&);
template GPtr<Scaleform::GSFMemoryFile>& GPtr<Scaleform::GSFMemoryFile>::operator=(
    const GPtr<Scaleform::GSFMemoryFile>&);

typedef hkArray<CollectedTriangle> CollectedTriangleArray;
typedef hkPointerMap<const hkpShape*, ScaledShapeArray*> ShapeToGroupMap;

static void (hkVector4::*const kKeepSetAbs4)(const hkVector4&) = &hkVector4::setAbs4;
static CollectedTriangle& (CollectedTriangleArray::*const kKeepExpandOne)() =
    &CollectedTriangleArray::expandOne;
static void (ScaledShapeArray::*const kKeepRemoveAt)(int) = &ScaledShapeArray::removeAt;
static Dummy* (ShapeToGroupMap::*const kKeepGetIterator)() const = &ShapeToGroupMap::getIterator;
static Dummy* (ShapeToGroupMap::*const kKeepGetNext)(Dummy*) const = &ShapeToGroupMap::getNext;
static float (*const kKeepMaxElement)(const hkVector4&) = &_maxElement;
