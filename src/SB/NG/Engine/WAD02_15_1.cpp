// WAD02_15_1: Scaleform GFx's texture node and its handler lists
// (GRendererCommonImpl.cpp), the Wii renderer's texture
// (GRendererWiiImpl.cpp), and System's core job processor.
//
// The DWARF gives each function's source lines, so the statement order
// below is the file's; it describes no locals for this library code.
// Class layouts are the DWARF's.
//
// The functions are not in retail's order. `#pragma always_inline on` is a
// region that takes in every function defined above it, so GTextureImplNode's
// constructor and CallHandlers, which retail calls, sit below the two regions
// that must not take them, and ResizeNoConstruct below the PushBack
// instantiations.
//
// Walls, not written: GRenderSyncWii_DrawSync's constructor, DrawSyncHandler
// and WaitFence reach their statics as displacements from the unity file's
// .bss base; CoreTaskingModule's constructor and GetPriority are in
// WAD02.cpp's anonymous namespace. Not yet written: InitTexture(GImageBase*),
// Update, MakeNextMiplevel, CreateRenderer, GRendererWii's destructor, the
// BlendType array's two destructors, GRenderer::Stats::Clear,
// GRect<int>::Height and SyncEvent::Create.

typedef unsigned long UPInt;

void operator delete(void* mem);
inline void* operator new(unsigned long, void* p) { return p; }

extern "C" {
void* memcpy(void* dst, const void* src, unsigned long n);
}

// __dt__12hkBaseObjectFv is the trivial destructor body every
// trivial destructor in the image folded onto, so a member's real
// type is gone with the fold and it is spelled as the surviving
// name -- which is what makes the relocation name retail's symbol.
class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

// A pointer array's DestructArray does nothing, and every copy of it
// was folded onto Math::Matrix33's empty constructor, the name that
// survived. Retail calls it with the array and its count; the call is
// spelled with that name so the branch names retail's symbol.
extern "C" void __ct__Q24Math8Matrix33Fv(void* p, UPInt count);

// ---------------------------------------------------------------- memory

struct GAllocDebugInfo {
    unsigned StatId;
    GAllocDebugInfo(unsigned statId) : StatId(statId) {}
};

class GMemoryHeap {
public:
    void* Alloc(UPInt size, const GAllocDebugInfo* info = 0);
    void* Alloc(UPInt size, UPInt align, const GAllocDebugInfo* info = 0);
    static void* Realloc(void* oldPtr, UPInt newSize);
    static void Free(void* ptr);
};

class GMemory {
public:
    static GMemoryHeap* pGlobalHeap;

    static void* Alloc(UPInt size, const GAllocDebugInfo& info)
    {
        return pGlobalHeap->Alloc(size, &info);
    }
    static void Free(void* p) { GMemoryHeap::Free(p); }
};

// A GFx class DELETES THROUGH ITS OWN OPERATOR. Retail's branch here
// is Free__11GMemoryHeapFPv and not the global __dl__FPv, which is
// what reloc_audit caught when these were first written the ordinary
// way: every word was equal, report.json credited them, and the call
// went somewhere else. A one-line `operator delete` is taken by
// -inline auto at the call site, so the heap's Free lands in the
// destructor itself, which is what the bytes have.
class GNewOverrideBase {
public:
    static void* operator new(UPInt sz) { return GMemory::pGlobalHeap->Alloc(sz); }
    static void operator delete(void* p) { GMemoryHeap::Free(p); }
};

// The raw atomic operations work on unsigned words through a volatile
// pointer: CompareAndSet compares with cmplw, and ExchangeAdd reads the
// value twice.
class GAtomicInt {
public:
    volatile long Value;

    operator long() const { return Value; }
    void operator=(long v) { Value = v; }

    long ExchangeAdd_NoSync(long delta)
    {
        volatile unsigned long* p = (volatile unsigned long*)&Value;
        unsigned long old = *p;
        *p += delta;
        return old;
    }
    bool CompareAndSet_NoSync(long c, long value)
    {
        volatile unsigned long* p = (volatile unsigned long*)&Value;
        if (*p == (unsigned long)c) {
            *p = value;
            return 1;
        }
        return 0;
    }
};

// ----------------------------------------------------------------- GArray

template <class T, int SID>
class GAllocatorGH {
public:
    static void* Alloc(const void*, UPInt size)
    {
        return GMemory::Alloc(size, GAllocDebugInfo(SID));
    }
    static void* Realloc(void* p, UPInt newSize) { return GMemoryHeap::Realloc(p, newSize); }
    static void Free(void* p) { GMemoryHeap::Free(p); }

    static void Construct(void* p, const T& source) { ::new (p) T(source); }
    static void Destruct(T* p) {}
    static void DestructArray(T* p, UPInt count) { __ct__Q24Math8Matrix33Fv(p, count); }
    static void CopyArrayForward(T* dst, const T* src, UPInt count)
    {
        memcpy(dst, src, sizeof(T) * count);
    }
};

class GArrayDefaultPolicy {
public:
    GArrayDefaultPolicy() : Capacity(0) {}

    bool NeverShrinking() const { return 0; }
    UPInt GetCapacity() const { return Capacity; }
    void SetCapacity(UPInt capacity) { Capacity = capacity; }
    UPInt GetGranularity() const { return 4; }

    UPInt Capacity;
};

template <class T, class Allocator, class SizePolicy>
struct GArrayDataBase {
    typedef T ValueType;
    typedef Allocator AllocatorType;

    GArrayDataBase() : Data(0), Size(0) {}
    ~GArrayDataBase();

    UPInt GetCapacity() const { return Policy.GetCapacity(); }

    void Reserve(const void* pheapAddr, UPInt newCapacity);
    void ResizeNoConstruct(const void* pheapAddr, UPInt newSize);

    T* Data;
    UPInt Size;
    SizePolicy Policy;
};

template <class T, class Allocator, class SizePolicy>
GArrayDataBase<T, Allocator, SizePolicy>::~GArrayDataBase()
{
    Allocator::DestructArray(Data, Size);
    Allocator::Free(Data);
}

template <class T, class Allocator, class SizePolicy>
void GArrayDataBase<T, Allocator, SizePolicy>::Reserve(const void* pheapAddr, UPInt newCapacity)
{
    if (Policy.NeverShrinking() && newCapacity < GetCapacity())
        return;

    if (newCapacity == 0) {
        if (Data) {
            Allocator::Free(Data);
            Data = 0;
        }
        Policy.SetCapacity(0);
    } else {
        UPInt gran = Policy.GetGranularity();
        newCapacity = (newCapacity + gran - 1) / gran * gran;
        if (Data) {
            Data = (T*)Allocator::Realloc(Data, sizeof(T) * newCapacity);
        } else {
            Data = (T*)Allocator::Alloc(pheapAddr, sizeof(T) * newCapacity);
        }
        Policy.SetCapacity(newCapacity);
    }
}


template <class T, class Allocator, class SizePolicy>
struct GArrayData : GArrayDataBase<T, Allocator, SizePolicy> {
    typedef GArrayDataBase<T, Allocator, SizePolicy> BaseType;

    GArrayData() : BaseType() {}

    void Resize(const void* pheapAddr, UPInt newSize)
    {
        BaseType::ResizeNoConstruct(pheapAddr, newSize);
    }

    void PushBack(const T& val)
    {
        BaseType::ResizeNoConstruct(this, this->Size + 1);
        Allocator::Construct(this->Data + this->Size - 1, val);
    }
};

template <class ArrayData>
class GArrayBase : public GNewOverrideBase {
public:
    typedef typename ArrayData::ValueType ValueType;
    typedef typename ArrayData::AllocatorType AllocatorType;

    GArrayBase() : Data() {}
    ~GArrayBase();

    UPInt GetSize() const { return Data.Size; }
    ValueType& operator[](UPInt index) { return Data.Data[index]; }

    void Resize(UPInt newSize) { Data.Resize(this, newSize); }
    void Clear() { Resize(0); }

    void PushBack(const ValueType& val);

    void RemoveAt(UPInt index)
    {
        if (Data.Size == 1) {
            Clear();
        } else {
            AllocatorType::Destruct(Data.Data + index);
            AllocatorType::CopyArrayForward(Data.Data + index, Data.Data + index + 1,
                                            Data.Size - 1 - index);
            --Data.Size;
        }
    }

    ArrayData Data;
};

template <class T, int SID, class SizePolicy>
class GArray : public GArrayBase<GArrayData<T, GAllocatorGH<T, SID>, SizePolicy> > {
public:
    typedef GArrayBase<GArrayData<T, GAllocatorGH<T, SID>, SizePolicy> > BaseType;

    GArray() : BaseType() {}
    ~GArray();
};

template <class T, int SID, class SizePolicy>
GArray<T, SID, SizePolicy>::~GArray() {}

// ------------------------------------------------------------- the renderer

class GFxState;
class GImageBase;
class GRenderer;
struct _GXTexObj {
    unsigned long dummy[8];
};
enum _GXTexWrapMode { GX_CLAMP, GX_REPEAT, GX_MIRROR, GX_MAX_TEXWRAPMODE };

extern "C" {
void GXInitTexObjWrapMode(_GXTexObj* obj, _GXTexWrapMode s, _GXTexWrapMode t);
void GXInvalidateTexAll(void);
void DCFlushRange(void* startAddr, unsigned long nBytes);
}

// 0x10 bytes: three words, then the vtable pointer (CreateRenderer
// stores GRendererWiiImpl's at +0xC).
class GRefCountBaseImpl {
public:
    int RefCount;
    void* pRefCountImpl;
    void* pWeakProxy;

    virtual ~GRefCountBaseImpl();
};

template <class T, int N>
class GRefCountBase : public GRefCountBaseImpl {
public:
    static void operator delete(void* p) { GMemoryHeap::Free(p); }

    ~GRefCountBase();
};

class GRenderer : public GRefCountBase<GFxState, 2> {
public:
    class EventHandler {
    public:
        enum EventType { Event_None };

        virtual ~EventHandler() {}
        virtual void OnEvent(GRenderer* prenderer, EventType eventType) = 0;
    };

    struct Stats {
        void Clear();

        unsigned int Triangles;
        unsigned int Lines;
        unsigned int Primitives;
        unsigned int Masks;
    };

    enum ResizeImageType { ResizeRgbToRgb, ResizeRgbaToRgba, ResizeRgbToRgba, ResizeGray };

    static void ResizeImage(unsigned char* pDst, int dstWidth, int dstHeight, int dstPitch,
                            const unsigned char* pSrc, int srcWidth, int srcHeight, int srcPitch,
                            ResizeImageType type);

    ~GRenderer();
};

GRenderer::~GRenderer() {}

void GRenderer::Stats::Clear()
{
    Triangles = 0;
    Lines = 0;
    Primitives = 0;
    Masks = 0;
}

template <class T>
class GRect {
public:
    T Left, Top, Right, Bottom;
};

class GImageBase {
public:
    enum ImageFormat {
        Image_None = 0,
        Image_ARGB_8888 = 1,
        Image_RGB_888 = 2,
        Image_A8 = 9,
        Image_DXT1 = 10,
        Image_DXT3 = 11,
        Image_DXT5 = 12
    };
};

class GRendererNode {
public:
    union {
        GRendererNode* pPrev;
        GRendererNode* pLast;
    };
    union {
        GRendererNode* pNext;
        GRendererNode* pFirst;
    };

    inline GRendererNode(GRendererNode* plist);
    inline void RemoveNode();
};

class GTexture : public GNewOverrideBase {
public:
    GAtomicInt RefCount;

    typedef void* Handle;

    class ChangeHandler {
    public:
        enum EventType { Event_DataChange, Event_DataLost, Event_RendererReleased };

        virtual ~ChangeHandler() {}
        virtual void OnChange(GRenderer* prenderer, EventType changeType) = 0;
    };

    struct UpdateRect {
        int destX, destY;
        GRect<int> src;
    };
    struct MapRect {
        unsigned width, height;
        unsigned char* pData;
        unsigned pitch;
    };

    GTexture() { RefCount = 1; }
    virtual ~GTexture() {}

    virtual bool InitTexture(GImageBase* pim, int targetWidth = 0, int targetHeight = 0) = 0;
    virtual bool InitTextureFromFile(const char* pfilename, int targetWidth = 0,
                                     int targetHeight = 0) = 0;
    virtual bool InitTexture(int width, int height, GImageBase::ImageFormat format,
                             int mipmaps, int targetWidth = 0, int targetHeight = 0) = 0;
    virtual void Update(int level, int n, const UpdateRect* rects, const GImageBase* pim) = 0;
    virtual bool InitMappableTexture(int width, int height, GImageBase::ImageFormat format,
                                     int mipmaps, int targetWidth = 0,
                                     int targetHeight = 0) = 0;
    virtual bool Map(int level, int n, MapRect* maps, int flags = 0) = 0;
    virtual bool Unmap(int level, int n, MapRect* maps, int flags = 0) = 0;
    virtual GRenderer* GetRenderer() const = 0;
    virtual bool IsDataValid() const = 0;
    virtual Handle GetUserHandle() const = 0;
    virtual void SetUserHandle(Handle hdata) = 0;
    virtual void AddChangeHandler(ChangeHandler* phandler) = 0;
    virtual void RemoveChangeHandler(ChangeHandler* phandler) = 0;

    bool AddRef_NotZero()
    {
        while (1) {
            long refCount = RefCount;
            if (refCount == 0)
                return 0;
            long next = refCount + 1;
            if (RefCount.CompareAndSet_NoSync(refCount, next))
                break;
        }
        return 1;
    }

    void Release()
    {
        if ((RefCount.ExchangeAdd_NoSync(-1) - 1) == 0)
            delete this;
    }
};

class GTextureImplNode : public GTexture, public GRendererNode {
public:
    Handle UserHandle;
    bool HandlerArrayFlag;
    union {
        ChangeHandler* pHandler;
        GArray<ChangeHandler*, 2, GArrayDefaultPolicy>* pHandlerArray;
    };

    GTextureImplNode(GRendererNode* plistRoot);
    virtual ~GTextureImplNode();

    virtual Handle GetUserHandle() const;
    virtual void SetUserHandle(Handle hdata);
    virtual void AddChangeHandler(ChangeHandler* phandler);
    virtual void RemoveChangeHandler(ChangeHandler* phandler);

    void CallHandlers(ChangeHandler::EventType event);
};

class GRendererEventHandlerImpl {
public:
    bool HandlerArrayFlag;
    union {
        GRenderer::EventHandler* pHandler;
        GArray<GRenderer::EventHandler*, 2, GArrayDefaultPolicy>* pHandlerArray;
    };

    ~GRendererEventHandlerImpl();

    bool AddHandler(GRenderer::EventHandler* phandler);
    void RemoveHandler(GRenderer::EventHandler* phandler);
    void CallHandlers(GRenderer* prenderer, GRenderer::EventHandler::EventType event);
};

// ------------------------------------------------ GRendererCommonImpl.cpp

GTextureImplNode::~GTextureImplNode()
{
    if (pHandler && HandlerArrayFlag)
        delete pHandlerArray;
    if (pFirst)
        RemoveNode();
}

void GTextureImplNode::AddChangeHandler(ChangeHandler* phandler)
{
    if (pHandler) {
        if (!HandlerArrayFlag) {
            ChangeHandler* ptemp = pHandler;
            pHandlerArray = new GArray<ChangeHandler*, 2, GArrayDefaultPolicy>;
            if (pHandlerArray) {
                pHandlerArray->PushBack(ptemp);
                HandlerArrayFlag = 1;
            } else
                return;
        }
        pHandlerArray->PushBack(phandler);
        return;
    } else {
        pHandler = phandler;
    }
}

void GTextureImplNode::RemoveChangeHandler(ChangeHandler* phandler)
{
    if (HandlerArrayFlag) {
        for (UPInt i = 0; i < pHandlerArray->GetSize(); i++)
            if ((*pHandlerArray)[i] == phandler) {
                // RemoveAt, written out: a class template's member is never
                // inlined into this function, and retail has it in line.
                {
                    GArray<ChangeHandler*, 2, GArrayDefaultPolicy>* a = pHandlerArray;
                    if (a->Data.Size == 1) {
                        a->Data.ResizeNoConstruct(a, 0);
                    } else {
                        memcpy(a->Data.Data + i, a->Data.Data + i + 1,
                               sizeof(ChangeHandler*) * (a->Data.Size - 1 - i));
                        --a->Data.Size;
                    }
                }
                if (pHandlerArray->GetSize() == 1) {
                    ChangeHandler* ph = (*pHandlerArray)[0];
                    delete pHandlerArray;
                    pHandler = ph;
                    HandlerArrayFlag = 0;
                    break;
                }
            }
    } else if (pHandler == phandler) {
        pHandler = 0;
    }
}

GRendererEventHandlerImpl::~GRendererEventHandlerImpl()
{
    if (pHandler && HandlerArrayFlag)
        delete pHandlerArray;
}

bool GRendererEventHandlerImpl::AddHandler(GRenderer::EventHandler* phandler)
{
    if (pHandler) {
        if (!HandlerArrayFlag) {
            GRenderer::EventHandler* ptemp = pHandler;
            pHandlerArray = new GArray<GRenderer::EventHandler*, 2, GArrayDefaultPolicy>;
            if (pHandlerArray) {
                pHandlerArray->PushBack(ptemp);
                HandlerArrayFlag = 1;
            } else
                return 0;
        }
        pHandlerArray->PushBack(phandler);
        return 1;
    } else {
        pHandler = phandler;
        return 1;
    }
}

void GRendererEventHandlerImpl::RemoveHandler(GRenderer::EventHandler* phandler)
{
    if (HandlerArrayFlag) {
        for (UPInt i = 0; i < pHandlerArray->GetSize(); i++)
            if ((*pHandlerArray)[i] == phandler) {
                // RemoveAt, written out (see RemoveChangeHandler).
                {
                    GArray<GRenderer::EventHandler*, 2, GArrayDefaultPolicy>* a = pHandlerArray;
                    if (a->Data.Size == 1) {
                        a->Data.ResizeNoConstruct(a, 0);
                    } else {
                        memcpy(a->Data.Data + i, a->Data.Data + i + 1,
                               sizeof(GRenderer::EventHandler*) * (a->Data.Size - 1 - i));
                        --a->Data.Size;
                    }
                }
                if (pHandlerArray->GetSize() == 1) {
                    GRenderer::EventHandler* ph = (*pHandlerArray)[0];
                    delete pHandlerArray;
                    pHandler = ph;
                    HandlerArrayFlag = 0;
                    break;
                }
            }
    } else if (pHandler == phandler) {
        pHandler = 0;
    }
}

void GRendererEventHandlerImpl::CallHandlers(GRenderer* prenderer,
                                             GRenderer::EventHandler::EventType event)
{
    if (HandlerArrayFlag) {
        for (UPInt i = 0; i < pHandlerArray->GetSize(); i++)
            (*pHandlerArray)[i]->OnEvent(prenderer, event);
    } else if (pHandler) {
        pHandler->OnEvent(prenderer, event);
    }
}

template class GArray<GTexture::ChangeHandler*, 2, GArrayDefaultPolicy>;
template class GArray<GRenderer::EventHandler*, 2, GArrayDefaultPolicy>;

// Below the handlers that call them and below the instantiation, so
// neither is inlined where retail calls it.
template <class ArrayData>
GArrayBase<ArrayData>::~GArrayBase() {}

template <class ArrayData>
void GArrayBase<ArrayData>::PushBack(const ValueType& val)
{
    Data.PushBack(val);
}

// Instantiated here inside an always_inline region, so GArrayData's
// PushBack goes in line as retail has it; ResizeNoConstruct, which retail
// calls, is defined below the region.
#pragma always_inline on
template void GArrayBase<GArrayData<GTexture::ChangeHandler*,
                                    GAllocatorGH<GTexture::ChangeHandler*, 2>,
                                    GArrayDefaultPolicy> >::PushBack(
    GTexture::ChangeHandler* const&);
template void GArrayBase<GArrayData<GRenderer::EventHandler*,
                                    GAllocatorGH<GRenderer::EventHandler*, 2>,
                                    GArrayDefaultPolicy> >::PushBack(
    GRenderer::EventHandler* const&);
#pragma always_inline off

// The linker folded the EventHandler array's Reserve onto the
// ChangeHandler array's, the name that survived, so a pointer array's
// ResizeNoConstruct is spelled calling that one; for the ChangeHandler
// array the cast changes nothing.
typedef GArrayDataBase<GTexture::ChangeHandler*, GAllocatorGH<GTexture::ChangeHandler*, 2>,
                       GArrayDefaultPolicy>
    GFoldedPointerArrayData;

template <class T, class Allocator, class SizePolicy>
void GArrayDataBase<T, Allocator, SizePolicy>::ResizeNoConstruct(const void* pheapAddr,
                                                                 UPInt newSize)
{
    UPInt oldSize = Size;

    if (newSize < oldSize) {
        Allocator::DestructArray(Data + newSize, oldSize - newSize);
        if (newSize < (Policy.GetCapacity() >> 1)) {
            ((GFoldedPointerArrayData*)this)->Reserve(pheapAddr, newSize);
        }
    } else if (newSize >= Policy.GetCapacity()) {
        ((GFoldedPointerArrayData*)this)->Reserve(pheapAddr, newSize + (newSize >> 2));
    }
    Size = newSize;
}

// -------------------------------------------------- GRendererWiiImpl.cpp

struct OSThreadQueue;

// Slots as __vt__23GRenderSyncWii_DrawSync gives them: the destructor
// (defined elsewhere), BeginFrame, an empty EndFrame, SetFence and
// WaitFence, which Update and Map call at +0x18.
class GRenderSyncWii {
public:
    virtual ~GRenderSyncWii();
    virtual void BeginFrame() = 0;
    virtual void EndFrame();
    virtual void SetFence() = 0;
    virtual void WaitFence(unsigned long fence) = 0;
};

class GRenderSyncWii_DrawSync : public GRenderSyncWii {
public:
    unsigned long fence;

    static bool init;
    static int count;
    static unsigned long waiton;
    static unsigned long lastv;
    static unsigned long lastw;
    static OSThreadQueue waitq;

    GRenderSyncWii_DrawSync();
    virtual ~GRenderSyncWii_DrawSync();
    virtual void BeginFrame();
    virtual void SetFence();
    virtual void WaitFence(unsigned long fence);
    static void DrawSyncHandler(unsigned short token);
};

class GRendererWii : public GRenderer {
public:
    ~GRendererWii();
};

GRendererWii::~GRendererWii() {}

class GRendererWiiImpl : public GRendererWii {
public:
    static void MakeNextMiplevel(unsigned int* pwidth, unsigned int* pheight,
                                 unsigned char* pdata, int bpp);

    unsigned char _pad10[0x4C - 0x10];
    GRendererNode Textures;
    unsigned char _pad54[0x13C - 0x54];
    GRenderSyncWii* pRenderSync;
    GRendererEventHandlerImpl Handlers;
};

class GTextureWii : public GTextureImplNode {
public:
    GTextureWii(GRendererNode* plistRoot) : GTextureImplNode(plistRoot) {}
    virtual ~GTextureWii() {}

    virtual bool InitTextureFromTexObj(_GXTexObj* ptex, int width, int height, bool alpha,
                                       _GXTexObj* palphatex) = 0;
};

class GTextureWiiImpl : public GTextureWii {
public:
    GRendererWiiImpl* pRenderer;
    int Width;
    int Height;
    void* pTexData;
    void* pAlphaTexData;
    int TexWidth;
    int TexHeight;
    int TexSize;
    _GXTexObj Tex;
    _GXTexObj AlphaTex;
    unsigned long LastUse;
    bool IsAlpha;
    bool IsTransparent;

    GTextureWiiImpl(GRendererWiiImpl* prenderer);
    ~GTextureWiiImpl();

    virtual bool InitTexture(GImageBase* pim, int targetWidth, int targetHeight);
    virtual bool InitTextureFromFile(const char* pfilename, int targetWidth, int targetHeight);
    virtual bool InitTexture(int width, int height, GImageBase::ImageFormat format, int mipmaps,
                             int targetWidth, int targetHeight);
    virtual void Update(int level, int n, const UpdateRect* rects, const GImageBase* pim);
    virtual bool InitMappableTexture(int width, int height, GImageBase::ImageFormat format,
                                     int mipmaps, int targetWidth, int targetHeight);
    virtual bool Map(int level, int n, MapRect* maps, int flags);
    virtual bool Unmap(int level, int n, MapRect* maps, int flags);
    virtual GRenderer* GetRenderer() const;
    virtual bool IsDataValid() const;
    virtual bool InitTextureFromTexObj(_GXTexObj* ptex, int width, int height, bool alpha,
                                       _GXTexObj* palphatex);

    void RemoveFromRenderer();
    void ReleaseTexture();
    void SetWrapMode(_GXTexWrapMode mode);
};

// always_inline takes GTextureWii's constructor in line; GTextureImplNode's,
// which retail calls, is defined below the region.
#pragma always_inline on
GTextureWiiImpl::GTextureWiiImpl(GRendererWiiImpl* prenderer)
    : GTextureWii(&prenderer->Textures)
{
    pRenderer = prenderer;
    Width = Height = 0;
    pTexData = 0;
    TexSize = 0;
    pAlphaTexData = 0;
    IsTransparent = 0;
}

#pragma always_inline off

GTextureImplNode::GTextureImplNode(GRendererNode* plistRoot)
    : GRendererNode(plistRoot)
{
    UserHandle = 0;
    HandlerArrayFlag = 0;
    pHandler = 0;
}

GTextureWiiImpl::~GTextureWiiImpl()
{
    ReleaseTexture();
    if (!pRenderer)
        return;

    if (pFirst)
        RemoveNode();
}

GRenderer* GTextureWiiImpl::GetRenderer() const
{
    return pRenderer;
}

bool GTextureWiiImpl::IsDataValid() const
{
    return Width != 0;
}

// The six words that used to differ here were all in AddRef_NotZero's
// compare-and-set. Retail computes count + 1 into its own register BEFORE
// the compare -- addi r4,r5,1 sits between the load and the cmplw -- and
// that is what a NAMED LOCAL gives; folding the increment into the call
// argument materialises it at the store instead. Naming it after the zero
// test matches; naming it before the zero test leaves 5 of 60. Reading the
// new value into a local inside CompareAndSet_NoSync matches too, and the
// local here is the smaller change of the two.
//
// always_inline takes AddRef_NotZero and Release in line, as retail has
// them. The region takes every function defined above it, so
// CallHandlers, ReleaseTexture and RemoveNode are defined below.
#pragma always_inline on
void GTextureWiiImpl::RemoveFromRenderer()
{
    pRenderer = 0;
    if (AddRef_NotZero()) {
        ReleaseTexture();
        CallHandlers(ChangeHandler::Event_RendererReleased);
        if (pFirst)
            RemoveNode();
        Release();
    } else {
        if (pFirst)
            RemoveNode();
    }
}

#pragma always_inline off

void GTextureImplNode::CallHandlers(ChangeHandler::EventType event)
{
    GRenderer* prenderer = GetRenderer();

    if (HandlerArrayFlag) {
        for (UPInt i = 0; i < pHandlerArray->GetSize(); i++)
            (*pHandlerArray)[i]->OnChange(prenderer, event);
    } else if (pHandler) {
        pHandler->OnChange(prenderer, event);
    }
}

void GTextureWiiImpl::ReleaseTexture()
{
    if (pTexData)
        GMemory::Free(pTexData);
    if (pAlphaTexData)
        GMemory::Free(pAlphaTexData);
    pTexData = 0;
    pAlphaTexData = 0;
    Width = Height = 0;
    TexSize = 0;
}

bool GTextureWiiImpl::InitTextureFromTexObj(_GXTexObj* ptex, int width, int height, bool alpha,
                                            _GXTexObj* palphatex)
{
    ReleaseTexture();

    Tex = *ptex;
    pTexData = 0;
    pAlphaTexData = 0;
    Width = width;
    Height = height;
    IsAlpha = alpha;

    if (palphatex) {
        AlphaTex = *palphatex;
        IsTransparent = 1;
    } else {
        IsTransparent = 0;
    }

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

void GTextureWiiImpl::SetWrapMode(_GXTexWrapMode mode)
{
    GXInitTexObjWrapMode(&Tex, mode, mode);
    if (IsTransparent)
        GXInitTexObjWrapMode(&AlphaTex, mode, mode);
}

enum _GXTexFmt { GX_TF_I8 = 1, GX_TF_IA8 = 3, GX_TF_RGBA8 = 6, GX_TF_CMPR = 14 };
enum _GXTexFilter {
    GX_NEAR,
    GX_LINEAR,
    GX_NEAR_MIP_NEAR,
    GX_LIN_MIP_NEAR,
    GX_NEAR_MIP_LIN,
    GX_LIN_MIP_LIN
};
enum _GXAnisotropy { GX_ANISO_1 };

extern "C" {
void GXInitTexObj(_GXTexObj* obj, void* image, unsigned short w, unsigned short h, _GXTexFmt fmt,
                  _GXTexWrapMode wrap_s, _GXTexWrapMode wrap_t, unsigned char mipmap);
void GXInitTexObjLOD(_GXTexObj* obj, _GXTexFilter min_filt, _GXTexFilter mag_filt, float min_lod,
                     float max_lod, float lod_bias, unsigned char bias_clamp,
                     unsigned char do_edge_lod, _GXAnisotropy max_aniso);
void* memset(void* dst, int c, unsigned long n);
}

// Defined below: retail calls it.
static int GetTextureSize(int bpp, int w, int h);

bool GTextureWiiImpl::InitTexture(int width, int height, GImageBase::ImageFormat format, int mipmaps,
                                  int targetWidth, int targetHeight)
{
    if (width > 1024 || height > 1024)
        return 0;

    ReleaseTexture();

    int bpp;
    _GXTexFmt texFormat;
    if (format == GImageBase::Image_ARGB_8888) {
        texFormat = GX_TF_RGBA8;
        bpp = 4;
        IsAlpha = 0;
    } else if (format == GImageBase::Image_RGB_888) {
        texFormat = GX_TF_RGBA8;
        bpp = 4;
        IsAlpha = 0;
    } else if (format == GImageBase::Image_A8) {
        texFormat = GX_TF_I8;
        bpp = 1;
        IsAlpha = 1;
    }

    if (targetWidth == 0)
        targetWidth = width;
    Width = targetWidth;
    if (targetHeight == 0)
        targetHeight = height;
    Height = targetHeight;

    int size = GetTextureSize(bpp, width, height);
    unsigned int w = width;
    unsigned int h = height;
    for (int i = 0; i < mipmaps; i++) {
        w >>= 1;
        h >>= 1;
        if (w < 1)
            w = 1;
        if (h < 1)
            h = 1;
        size += GetTextureSize(bpp, w, h);
    }

    pTexData = GMemory::pGlobalHeap->Alloc(size, 32);
    TexSize = size;
    TexWidth = width;
    TexHeight = height;
    LastUse = 0xFFFFFFFF;

    if (texFormat == GX_TF_IA8 || format == GImageBase::Image_RGB_888)
        memset(pTexData, 255, size);

    if (mipmaps) {
        GXInitTexObj(&Tex, pTexData, width, height, texFormat, GX_CLAMP, GX_CLAMP, 1);
        GXInitTexObjLOD(&Tex, GX_LIN_MIP_LIN, GX_LINEAR, 0.0f, (float)mipmaps, -0.5f, 0, 0,
                        GX_ANISO_1);

        if (IsTransparent) {
            GXInitTexObj(&AlphaTex, pAlphaTexData, width, height, GX_TF_I8, GX_CLAMP, GX_CLAMP, 1);
            GXInitTexObjLOD(&AlphaTex, GX_LIN_MIP_LIN, GX_LINEAR, 0.0f, (float)mipmaps, -0.5f, 0, 0,
                            GX_ANISO_1);
        }
    } else {
        GXInitTexObj(&Tex, pTexData, width, height, texFormat, GX_CLAMP, GX_CLAMP, 0);
        if (IsTransparent)
            GXInitTexObj(&AlphaTex, pAlphaTexData, width, height, GX_TF_I8, GX_CLAMP, GX_CLAMP, 0);
    }

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

bool GTextureWiiImpl::InitMappableTexture(int width, int height, GImageBase::ImageFormat format,
                                          int mipmaps, int targetWidth, int targetHeight)
{
    return InitTexture(width, height, format, mipmaps, targetWidth, targetHeight);
}

bool GTextureWiiImpl::Unmap(int level, int n, MapRect* maps, int flags)
{
    DCFlushRange(maps[0].pData, maps[0].height * maps[0].pitch);
    GXInvalidateTexAll();
    return 1;
}

extern "C" {
void GXSetDrawSync(unsigned short token);
}

static int GetTextureSize(int bpp, int w, int h)
{
    if (bpp == 1)
        return ((w + 7) & ~7) * ((h + 3) & ~3);

    if (bpp == 0)
        return ((w + 7) >> 3) * ((h + 7) >> 3) * 32;
    return bpp * ((w + 3) & ~3) * ((h + 3) & ~3);
}

// Halves the level in place: an average of two along x when a dimension
// cannot be halved, of a 2x2 box when both can. The width is read through
// the pointer inside the loop, as retail does -- a byte store can alias it.
//
// NEAR MISS: 111 of 238 words differ, at retail's exact 952 bytes. The
// prologue and epilogue, the one-channel 2x1 loop, the whole one-channel
// 2x2 box loop, both box loop set-ups and every loop tail are exact; what
// differs is register numbers and the order the byte loads and adds are
// issued inside the four multi-channel bodies. Measured: storing each
// channel as it is computed, 180 of 238 (the stores alias the source, so
// no later load can be hoisted over them); the values into locals before
// the stores, 150; the row below as its own pointer with each channel a
// pair of pairs and the loads read into locals in channel order, 111; one
// source pointer declared ahead of the branch chain, which retail keeps in
// one saved register across the branches, 111 again.
void GRendererWiiImpl::MakeNextMiplevel(unsigned int* pwidth, unsigned int* pheight,
                                        unsigned char* pdata, int bpp)
{
    unsigned int new_w = *pwidth >> 1;
    unsigned int new_h = *pheight >> 1;
    if (new_w < 1)
        new_w = 1;
    if (new_h < 1)
        new_h = 1;

    if (new_w * 2 != *pwidth || new_h * 2 != *pheight) {
        if (bpp == 1) {
            unsigned char* in = pdata;
            for (unsigned int i = 0; i < new_w * new_h; i++) {
                *pdata = (in[0] + in[1]) >> 1;
                pdata++;
                in += 2;
            }
        } else if (bpp == 3) {
            unsigned char* in = pdata;
            for (unsigned int i = 0; i < new_w * new_h; i++) {
                int a0 = in[0], b0 = in[3];
                int a1 = in[1], b1 = in[4];
                int a2 = in[2], b2 = in[5];
                pdata[0] = (a0 + b0) >> 1;
                pdata[1] = (a1 + b1) >> 1;
                pdata[2] = (a2 + b2) >> 1;
                pdata += 3;
                in += 6;
            }
        } else {
            unsigned char* in = pdata;
            for (unsigned int i = 0; i < new_w * new_h; i++) {
                int a0 = in[0], b0 = in[4];
                int a1 = in[1], b1 = in[5];
                int a2 = in[2], b2 = in[6];
                int a3 = in[3], b3 = in[7];
                pdata[0] = (a0 + b0) >> 1;
                pdata[1] = (a1 + b1) >> 1;
                pdata[2] = (a2 + b2) >> 1;
                pdata[3] = (a3 + b3) >> 1;
                pdata += 4;
                in += 8;
            }
        }
    } else {
        if (bpp == 1) {
            for (unsigned int j = 0; j < new_h; j++) {
                unsigned char* out = pdata + j * new_w;
                unsigned char* in = pdata + (j << 1) * *pwidth;
                for (unsigned int i = 0; i < new_w; i++) {
                    *out = (in[0] + in[1] + in[*pwidth] + in[*pwidth + 1]) >> 2;
                    out++;
                    in += 2;
                }
            }
        } else if (bpp == 3) {
            for (unsigned int j = 0; j < new_h; j++) {
                unsigned char* out = pdata + j * (new_w * 3);
                unsigned char* in = pdata + (j << 1) * (*pwidth * 3);
                for (unsigned int i = 0; i < new_w; i++) {
                    unsigned char* in2 = in + *pwidth * 3;
                    int v0 = ((in[3] + in[0]) + (in2[3] + in2[0])) >> 2;
                    int v1 = ((in[1] + in[4]) + (in2[1] + in2[4])) >> 2;
                    int v2 = ((in[2] + in[5]) + (in2[2] + in2[5])) >> 2;
                    out[0] = v0;
                    out[1] = v1;
                    out[2] = v2;
                    out += 3;
                    in += 6;
                }
            }
        } else {
            for (unsigned int j = 0; j < new_h; j++) {
                unsigned char* out = pdata + j * (new_w * 4);
                unsigned char* in = pdata + (j << 1) * (*pwidth * 4);
                for (unsigned int i = 0; i < new_w; i++) {
                    unsigned char* in2 = in + *pwidth * 4;
                    int v0 = ((in[4] + in[0]) + (in2[4] + in2[0])) >> 2;
                    int v1 = ((in[1] + in[5]) + (in2[5] + in2[1])) >> 2;
                    int v2 = ((in[2] + in[6]) + (in2[6] + in2[2])) >> 2;
                    int v3 = ((in[3] + in[7]) + (in2[7] + in2[3])) >> 2;
                    out[0] = v0;
                    out[1] = v1;
                    out[2] = v2;
                    out[3] = v3;
                    out += 4;
                    in += 8;
                }
            }
        }
    }

    *pwidth = new_w;
    *pheight = new_h;
}

bool GTextureWiiImpl::Map(int level, int n, MapRect* maps, int flags)
{
    if (LastUse != 0xFFFFFFFF && pRenderer->pRenderSync)
        pRenderer->pRenderSync->WaitFence(LastUse);

    unsigned char* pdata = (unsigned char*)pTexData;
    int bpp = IsAlpha ? 1 : 4;
    int w = TexWidth;
    int h = TexHeight;
    int w4 = (bpp == 1) ? ((w + 7) & ~7) : ((w + 3) & ~3);
    int h4 = (h + 3) & ~3;

    for (int i = 0; i < level; i++) {
        pdata += bpp * w4 * h4;
        w >>= 1;
        if (w < 1)
            w = 1;
        h >>= 1;
        if (h < 1)
            h = 1;
        w4 = (bpp == 1) ? ((w + 7) & ~7) : ((w + 3) & ~3);
        h4 = (h + 3) & ~3;
    }

    maps[0].width = w;
    maps[0].height = h;
    maps[0].pitch = w * bpp;
    maps[0].pData = pdata;
    return 1;
}

static unsigned char* SoftwareResample(int format, int w, int h, int pitch, unsigned char* pdata,
                                       int neww, int newh)
{
    unsigned char* pnew;

    switch (format) {
    case GImageBase::Image_RGB_888:
        pnew = (unsigned char*)GMemory::pGlobalHeap->Alloc(neww * newh * 4);
        GRenderer::ResizeImage(pnew, neww, newh, neww * 4, pdata, w, h, pitch,
                               GRenderer::ResizeRgbToRgba);
        break;

    case GImageBase::Image_ARGB_8888:
        pnew = (unsigned char*)GMemory::pGlobalHeap->Alloc(neww * newh * 4);
        GRenderer::ResizeImage(pnew, neww, newh, neww * 4, pdata, w, h, pitch,
                               GRenderer::ResizeRgbaToRgba);
        break;

    case GImageBase::Image_A8:
        pnew = (unsigned char*)GMemory::pGlobalHeap->Alloc(neww * newh);
        GRenderer::ResizeImage(pnew, neww, newh, neww, pdata, w, h, pitch, GRenderer::ResizeGray);
        break;
    }

    return pnew;
}

static unsigned char* LoadTextureTile(unsigned char* pdst, const unsigned char* psrc, unsigned int w,
                                      unsigned int h, int bpp, int pitch)
{
    if (bpp == 4)
        for (unsigned int y = 0; y < h; y += 4)
            for (unsigned int x = 0; x < w; x += 4, pdst += 64) {
                for (int by = 0; by < 4 && y + by < h; by++)
                    for (int bx = 0; bx < 4 && x + bx < w; bx++) {
                        pdst[by * 8 + bx * 2] = psrc[pitch * (y + by) + (x + bx) * 4 + 3];
                        pdst[by * 8 + bx * 2 + 1] = psrc[pitch * (y + by) + (x + bx) * 4];
                        pdst[by * 8 + bx * 2 + 32] = psrc[pitch * (y + by) + (x + bx) * 4 + 1];
                        pdst[by * 8 + bx * 2 + 33] = psrc[pitch * (y + by) + (x + bx) * 4 + 2];
                    }
            }
    else if (bpp == 2)
        for (unsigned int y = 0; y < h; y += 4)
            for (unsigned int x = 0; x < w; x += 4, pdst += 32) {
                for (int by = 0; by < 4 && y + by < h; by++)
                    for (int bx = 0; bx < 4 && x + bx < w; bx++) {
                        pdst[by * 8 + bx * 2] = psrc[pitch * (y + by) + x + bx];
                        pdst[by * 8 + bx * 2 + 1] = 0xFF;
                    }
            }
    else if (bpp == 1)
        for (unsigned int y = 0; y < h; y += 4)
            for (unsigned int x = 0; x < w; x += 8, pdst += 32) {
                for (int by = 0; by < 4 && y + by < h; by++)
                    for (int bx = 0; bx < 8 && x + bx < w; bx++) {
                        pdst[by * 8 + bx] = psrc[pitch * (y + by) + x + bx];
                    }
            }

    return pdst;
}

// Until the InitTexture functions that call them are written, their
// addresses keep these three statics in the object.
static unsigned char* (*const kKeepSoftwareResample)(int, int, int, int, unsigned char*, int,
                                                     int) = &SoftwareResample;
static unsigned char* (*const kKeepLoadTextureTile)(unsigned char*, const unsigned char*,
                                                    unsigned int, unsigned int, int,
                                                    int) = &LoadTextureTile;

void GRenderSyncWii_DrawSync::BeginFrame()
{
    if (lastw)
        fence = 0;
    lastw = 0;
}

void GRenderSyncWii_DrawSync::SetFence()
{
    GXSetDrawSync(fence);
    fence++;
}

// Weak copies retail calls: declared inline in the class and defined
// below their callers.
inline GRendererNode::GRendererNode(GRendererNode* plist)
{
    pNext = plist->pFirst;
    pPrev = plist;
    plist->pFirst->pPrev = this;
    plist->pFirst = this;
}

inline void GRendererNode::RemoveNode()
{
    pPrev->pNext = pNext;
    pNext->pPrev = pPrev;
    pPrev = 0;
    pNext = 0;
}

// ------------------------------------------------------ System's job queue

struct OSThread;
struct OSThreadQueue {
    OSThread* head;
    OSThread* tail;
};
struct OSCond {
    OSThreadQueue queue;
};
struct OSMutex;
struct OSMutexLink {
    OSMutex* next;
    OSMutex* prev;
};
struct OSMutex {
    OSThreadQueue queue;
    OSThread* thread;
    long count;
    OSMutexLink link;
};

extern "C" {
void OSInitCond(OSCond* cond);
void OSWaitCond(OSCond* cond, OSMutex* mutex);
void OSSignalCond(OSCond* cond);
void OSInitMutex(OSMutex* mutex);
void OSLockMutex(OSMutex* mutex);
void OSUnlockMutex(OSMutex* mutex);
}

namespace System {

class SyncEvent {
public:
    OSCond cond;
    OSMutex mutex;
    bool signalled;

    inline void Create();
    inline void Wait();
    inline void Signal();
};

class CriticalSection {
public:
    int refcount;
    OSMutex mutex;
    OSThread* owner;

    void Enter();
    void Exit();
};

class Thread {
public:
    class Runnable {
    public:
        virtual ~Runnable() {}
        virtual void Run() = 0;
    };

    long priority;
    void* stackBase;
    unsigned int startStackSize;
    OSThread* id;
    Runnable* runee;
    char* name;
    SyncEvent stopEvent;
    int extraStackSize;

    void Start(Runnable* runnable);
};

template <class T>
class PoolArray {
public:
    int Size() const { return size; }
    T& operator[](int i) { return pool[i]; }

    int size;
    T* pool;
    int poolSize;
};

template <class T>
class CircularQueue {
public:
    T Pop()
    {
        T item = pool[start];
        --size;
        start = (start + 1) & mask;
        return item;
    }

    unsigned int start;
    unsigned int size;
    unsigned int mask;
    T* pool;
};

class Job {
};

enum JobType {
    JOB_TYPE_EXECUTABLE,
    JOB_TYPE_START_FENCE,
    JOB_TYPE_COMPLETE_FENCE,
    JOB_TYPE_COMPLETE_BARRIER,
    JOB_TYPE_SIGNAL_BARRIER,
    MAX_JOB_TYPE
};

class CoreJobProcessor;
class CoreQueueRef;

class CoreJob : public Job {
public:
    void (*processCB)(CoreJobProcessor* processor, CoreQueueRef* ref, CoreJob* job);
    JobType type;
};

class CoreJobBarrier : public CoreJob {
};

class CoreJobCompleteFence : public CoreJob {
public:
    bool passed;
    SyncEvent event;
    CoreJobCompleteFence* prev;
    CoreJobCompleteFence* next;
    unsigned int number;
};

class CoreJobQueue {
public:
    CoreJob* Pop();

    CoreJobProcessor* owner;
    CircularQueue<CoreJob*> queue;
};

struct CoreJobPendingRef {
    unsigned int number;
    CoreJob* job;
};

class CoreQueueRef {
public:
    CoreJobQueue* queue;
    int frequency;
    int iter;
    volatile unsigned int jobNumber;
    unsigned int completeNumber;
    CoreJobPendingRef* pendingJobs;
    int pendingJobsSize;
    CoreJobBarrier* barrier;
    CoreJobCompleteFence* completeFenceHead;
    CoreJobCompleteFence* completeFenceTail;
};

// Its vtable: a destructor folded elsewhere, then Start and Stop.
class CoreJobProcessor {
public:
    CoreJobProcessor();
    virtual ~CoreJobProcessor();
    virtual void Start();
    virtual void Stop();

    // The member is spelled hkBaseObject for the destructor's sake (see
    // above); Start and Stop reach the Thread through thread().
    class Slot : public Thread::Runnable {
    public:
        ~Slot();
        virtual void Run();

        Thread& thread() { return *(Thread*)&mThread; }

        CoreJobProcessor* owner;
        int slotIndex;
        hkBaseObject mThread;
        unsigned char _pad10[0x40 - 4];
    };

    struct PriorityLevel {
        PoolArray<CoreQueueRef> queues;
        int nextQueueIndex;
    };

    bool ProcessNextJob(int slot);
    CoreQueueRef* NextQueue(int slot);
    void SignalExeComplete(int slot, CoreQueueRef* ref, CoreJob* job, unsigned int number);
    void SignalExeStart(int slot, CoreQueueRef* ref, CoreJob* job, unsigned int number);

    PoolArray<Slot> slots;
    CriticalSection jobAccessCS;
    SyncEvent addJobEvent;
    SyncEvent completeJobEvent;
    int nextQueueIndex;
    PriorityLevel levels[4];
    bool done;
};

// The pop is written out: retail has it in line, and neither
// CircularQueue<CoreJob*>::Pop nor the same Pop in a plain class was inlined
// here (both measured; each left an out-of-line Pop and a tail call).
CoreJob* CoreJobQueue::Pop()
{
    CoreJob* job = queue.pool[queue.start];
    queue.size--;
    queue.start = (queue.start + 1) & queue.mask;
    return job;
}

void CoreJobProcessor::Slot::Run()
{
    while (owner->ProcessNextJob(slotIndex))
        ;
    owner->addJobEvent.Signal();
}

CoreJobProcessor::CoreJobProcessor() {}

CoreJobProcessor::Slot::~Slot() {}

void CoreJobProcessor::Start()
{
    done = 0;
    for (int i = 0; i < slots.Size(); i++)
        slots[i].thread().Start(&slots[i]);
}

void CoreJobProcessor::Stop()
{
    done = 1;
    addJobEvent.Signal();
    for (int i = 0; i < slots.Size(); i++)
        slots[i].thread().stopEvent.Wait();
}

void CoreJobProcessor::SignalExeStart(int slot, CoreQueueRef* ref, CoreJob* job,
                                      unsigned int number)
{
    ref->pendingJobs[ref->pendingJobsSize].number = number;
    ref->pendingJobs[ref->pendingJobsSize].job = job;
    ref->pendingJobsSize++;
}

// NEAR MISS: 14 of 58 words differ, all register numbers, and the half of
// it that was fixable is fixed. Declaring the element ahead of the queue
// loop (as below) put j and the end index in retail's r9 and r10 and took
// this from 23 words to 14; declaring it at the top of the function
// instead gives the same bytes.
//
// What is left is one swap: ours has the element in r8 and the ready flag
// in r6, retail the reverse. That pair does NOT follow declaration order.
// Exchanging the two declarations, hoisting the flag to the top of the
// function, and putting both at the top of the function all produce output
// byte-identical to this -- three orders, one result -- so mwcc is choosing
// between them on something other than where they are written, and the
// next attempt should not be a fourth ordering. Earlier passes also tried
// the flag behind an accessor, the element as a pointer and through an
// index local, and the level as a pointer.
CoreQueueRef* CoreJobProcessor::NextQueue(int slot)
{
    for (int i = 0; i < 4; i++) {
        PriorityLevel& level = levels[i];
        CoreQueueRef* ref;
        bool ready;
        if (level.queues.size == 0)
            continue;

        unsigned int j = level.nextQueueIndex;
        unsigned int end = j + level.queues.size;
        while (j < end) {
            ref = &level.queues.pool[j % level.queues.size];

            if (ref->iter < 0) {
                ref->iter++;
                j++;
                continue;
            }

            ref->iter--;
            ready = ref->queue->queue.size != 0 && ref->barrier == 0;
            if (ref->iter < 0 || !ready) {
                ref->iter = ref->frequency;
                j++;
            }
            if (ready) {
                level.nextQueueIndex = j % level.queues.size;
                return ref;
            }
        }
    }
    return 0;
}

void CoreJobProcessor::SignalExeComplete(int slot, CoreQueueRef* ref, CoreJob* job,
                                         unsigned int number)
{
    CoreJobPendingRef* p;
    CoreJobPendingRef* end;

    for (p = ref->pendingJobs, end = ref->pendingJobs + ref->pendingJobsSize; p != end; ++p) {
        if (p->job == job) {
            p->number = (end - 1)->number;
            p->job = (end - 1)->job;
            ref->pendingJobsSize--;
            break;
        }
    }

    unsigned int minNumber = ref->jobNumber;
    for (p = ref->pendingJobs, end = ref->pendingJobs + ref->pendingJobsSize; p != end; ++p) {
        if (minNumber > p->number - 1)
            minNumber = p->number - 1;
    }
    ref->completeNumber = minNumber;

    CoreJobCompleteFence* fence = ref->completeFenceHead;
    for (; fence != 0; fence = fence->next) {
        if (minNumber < fence->number)
            break;
        fence->passed = 1;
        fence->event.Signal();
    }

    ref->completeFenceHead = fence;
    if (fence == 0)
        ref->completeFenceTail = 0;
    else
        fence->prev = 0;
}

// Two things make this one match, and the first is why jobNumber is
// volatile. Retail stores the incremented number and READS IT STRAIGHT
// BACK (lwz, addi, stw, lwz); mwcc never re-reads an ordinary member whose
// value it just wrote, so no arrangement of the arithmetic produces that
// load -- an earlier pass tried three declaration orders, an accessor, ++,
// += 1 and a reference local, and all of them left 47 of 84 words. The
// member's type is what produces it: volatile took the function to 10 of
// 85 at retail's exact size. Forcing the same re-read at this one site
// with a cast, leaving the member ordinary, changed nothing at all, which
// is what says the fact belongs to the member and not to the access.
//
// The ten words left after that were one register swap -- retail keeps the
// job in r29 and the number in r31 -- and the declaration order below is
// what assigns them that way. Declaring each local at its first use gives
// the same bytes; the other two orders give 16 and 18 words.
bool CoreJobProcessor::ProcessNextJob(int slot)
{
    static int waiters;
    CoreQueueRef* ref;
    unsigned int number;
    CoreJob* job;

    jobAccessCS.Enter();

    while (1) {
        if (done) {
            jobAccessCS.Exit();
            return 0;
        }
        ref = NextQueue(slot);
        if (ref) {
            addJobEvent.Signal();
            break;
        }
        waiters++;
        jobAccessCS.Exit();
        addJobEvent.Wait();
        jobAccessCS.Enter();
        waiters--;
    }

    ref->jobNumber++;
    number = ref->jobNumber;

    job = ref->queue->Pop();
    if (job->type == JOB_TYPE_EXECUTABLE) {
        SignalExeStart(slot, ref, job, number);
        jobAccessCS.Exit();
        job->processCB(this, ref, job);
        jobAccessCS.Enter();
        SignalExeComplete(slot, ref, job, number);
    } else {
        job->processCB(this, ref, job);
    }

    jobAccessCS.Exit();
    return 1;
}

inline void SyncEvent::Create()
{
    OSInitCond(&cond);
    OSInitMutex(&mutex);
    signalled = 0;
}

static void (SyncEvent::*const kKeepCreate)() = &SyncEvent::Create;

inline void SyncEvent::Wait()
{
    OSLockMutex(&mutex);
    while (!signalled)
        OSWaitCond(&cond, &mutex);
    signalled = 0;
    OSUnlockMutex(&mutex);
}

inline void SyncEvent::Signal()
{
    OSLockMutex(&mutex);
    signalled = 1;
    OSSignalCond(&cond);
    OSUnlockMutex(&mutex);
}

}  // namespace System
