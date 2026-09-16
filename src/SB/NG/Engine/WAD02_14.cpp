// WAD02_14 -- ScaleformCoord.cpp: Scaleform::Coordinator, its ViewNode /
// MovieNode / TextureNode / SFEventNode lists, and the GFx glue they call.
//
// Class layouts are the DWARF's (alltypes.h). The two flag bytes are
// BITFIELDS, not the unions the type dumper prints: Coordinator+0x2D4 is
// firstPlay 0x80, texturesLoaded 0x40, moviesStopped 0x20, firstMovie 0x10,
// isWidescreen 0x08, sfEnabled 0x04, and ViewNode+0x44 / +0x45 likewise.
// Every bit was read back off the rlwinm/ori masks in the listing.
//
// Memory::AllocGlobalHeap and FreeGlobalHeap take the heap BY VALUE, but
// retail loads that value from a fresh anonymous .data word at every call
// site (`lwz r4,@92481`) instead of materialising it. That is a
// `const H&` parameter on an inline helper: binding the enumerator to the
// reference makes mwcc place a const temporary in .data per call site and
// the inlined body loads it back. The same reference is what the
// out-of-line Delete<GlobalHeapEnum,SFEventNode> reads through r30.
//
// GFx refcounting goes through the table at +4, not a vtable: AddRef is
// one load (`lwz r12,0(impl)`) and Release one at +4, so the member is a
// pair of function pointers rather than a virtual base.
//
// The Coordinator's critical section is mBlockAllocator.heapLock --
// HeapAllocatorWii is 0x1AC bytes with its CriticalSection at +0x18C,
// which is the 0x204 every Enter/Exit in the listing names.

typedef unsigned long long uid;

class zBTTask;

void operator delete(void* mem);
inline void* operator new(unsigned long, void* p) { return p; }

extern "C" {
int strcmp(const char* a, const char* b);
unsigned long strlen(const char* s);
char* strcpy(char* dst, const char* src);
}

// ----------------------------------------------------------------- memory

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeap = 0, GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap,
                      unsigned long align, eMemMgrTag tag, bool clear);
void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

class StaticStackAllocator {
public:
    void Create(void* buffer, int size);

    unsigned char* mark;
    unsigned char* buffer;
    unsigned char* bufferEnd;
};

}  // namespace Memory

// ------------------------------------------------------------------ system

namespace System {

struct OSThread;

struct OSThreadQueue {
    OSThread* head;
    OSThread* tail;
};

struct OSMutexLink {
    void* next;
    void* prev;
};

struct OSMutex {
    OSThreadQueue queue;
    OSThread* thread;
    long count;
    OSMutexLink link;
};

class CriticalSection {
public:
    void Enter();
    void Exit();

    int refcount;
    OSMutex mutex;
    OSThread* owner;
};

}  // namespace System

// ------------------------------------------------------------------- util

namespace Util {

class Referrer {
public:
    Referrer(void* owner) {}
};

class PoolListBase {
public:
    class NodeHeader {
    public:
        NodeHeader* prev;
        NodeHeader* next;
    };

    void PushBack();
    NodeHeader* Erase(NodeHeader* node);
    NodeHeader* Insert(NodeHeader* node);

    int size;
    NodeHeader tail;
    NodeHeader* freeList;
    NodeHeader* poolHead;
    int poolSize;
    int peakSize;
};

template <class T>
class PoolList : public PoolListBase {
public:
    class NodeType : public NodeHeader {
    public:
        T data;
    };

    class ConstIterator {
    public:
        NodeType* node;
    };

    class Iterator : public ConstIterator {
    public:
        Iterator operator++(int);
    };

    void PushBack(const T& value);

    NodeType* Begin() const { return (NodeType*)tail.next; }
    NodeType* End() const { return (NodeType*)&tail; }
};

}  // namespace Util

// ------------------------------------------------------------------ world

namespace World {

class ScaleformOGAsset {
public:
    virtual void vt0();
    virtual void vt1();
    virtual void vt2();
    virtual void OnMovieStopped();
};

class EntityHandleBase {
public:
    void Deactivate(const Util::Referrer& ref);

    unsigned char _pad0[0x3C];
    int refCount;
};

}  // namespace World

// --------------------------------------------------------------------- GFx

class GRefCountBaseImpl;

// AddRef is `lwz r12,0(impl)` and Release `lwz r12,4(impl)` -- one load
// each, so the object at +4 is a table of function pointers.
struct GRefCountTable {
    void (*AddRef)(GRefCountBaseImpl* p);
    void (*Release)(GRefCountBaseImpl* p, unsigned int flags);
};

class GRefCountBaseImpl {
public:
    void AddRef() { pRefCountImpl->AddRef(this); }
    void Release(unsigned int flags) { pRefCountImpl->Release(this, flags); }
    void SetRefCountMode(unsigned int mode);

    int RefCount;
    GRefCountTable* pRefCountImpl;
    void* pWeakProxy;

    // Declared, never defined: the vtable is emitted where this is.
    virtual ~GRefCountBaseImpl();
};

class GFxDrawText;

template <class T>
class GPtr {
public:
    GPtr(T* p) { pObject = p; }
    ~GPtr();

    GPtr<T>& operator=(T* p);

    T* pObject;
};

class GFxState {
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
        State_ImageCreator = 12,
        State_ParseControl = 13,
        State_ProgressHandler = 14,
        State_ImagePackParamsState = 15,
        State_MeshCacheManager = 16,
        State_FontPackParamsState = 17,
        State_FontCacheManager = 18,
        State_FontLib = 19,
        State_FontProvider = 20,
        State_FontMap = 21
    };

    GFxState(StateType state);

    GRefCountBaseImpl _base0;
    StateType SType;
};

// GetStateBagImpl is declared first and never defined, so this unit emits
// no __vt__11GFxStateBag; GetStateAddRef is the virtual at slot 3 that the
// image DOES place here.
class GFxStateBag {
public:
    virtual GFxStateBag* GetStateBagImpl() const;
    virtual void SetState(GFxState::StateType state, GFxState* pstate);
    virtual void vt2();
    virtual GFxState* GetStateAddRef(GFxState::StateType state) const;

    GPtr<GFxDrawText> GetMeshCacheManager() const;
    GPtr<GFxDrawText> GetFontCacheManager() const;
    GPtr<GFxDrawText> GetFontLib() const;
};

class GFxMovieView : public GRefCountBaseImpl {
public:
    virtual void vt1();
    virtual void vt2();
    virtual void vt3();
    virtual void vt4();
    virtual void vt5();
    virtual void vt6();
    virtual void vt7();
    virtual void SetPlayState(int state);
};

struct GFxValueUnion {
    unsigned int Data0;
    unsigned int Data1;
};

class GFxResource {
public:
    void Release();
};

class GFxValue {
public:
    GFxValue() { Type = 0; }

    GFxValue& operator=(const GFxValue& src);
    void SetNumber(double v);

    int Type;
    unsigned int TypePad;
    GFxValueUnion Value;
};

class GFxEvent {
public:
    enum EventType { Event_None = 0 };
};

class GFxKey {
public:
    enum Code { VoidSymbol = 0 };
};

class GFxKeyEvent {
public:
    GFxKeyEvent(GFxEvent::EventType eventType, GFxKey::Code keyCode,
                unsigned char asciiCode, unsigned long wcharCode);

    GFxEvent::EventType Type;
    unsigned char Id;
    unsigned char _pad5[3];
    GFxKey::Code KeyCode;
    unsigned char AsciiCode;
    unsigned char _padD[3];
    unsigned long WcharCode;
};

class GFxMouseEvent {
public:
    GFxMouseEvent(GFxEvent::EventType eventType, unsigned int button, float x,
                  float y, float scrollDelta, unsigned int mouseIndex);

    GFxEvent::EventType Type;
    unsigned char Id;
    unsigned char _pad5[3];
    float x;
    float y;
    float ScrollDelta;
    unsigned int Button;
    unsigned int MouseIndex;
};

class GMemoryHeap {
public:
    class HeapDesc {
    public:
        HeapDesc(unsigned int flags, unsigned long minAlign,
                 unsigned long granularity, unsigned long reserve,
                 unsigned long threshold, unsigned long limit,
                 unsigned long heapId);

        unsigned int Flags;
        unsigned long MinAlign;
        unsigned long Granularity;
        unsigned long Reserve;
        unsigned long Threshold;
        unsigned long Limit;
        unsigned long HeapId;
    };

    static void Free(void* p);
};

class GNewOverrideBase {
public:
    static void operator delete(void* p) { GMemoryHeap::Free(p); }
};

class GFxFSCommandHandler : public GNewOverrideBase {
public:
    ~GFxFSCommandHandler();
};

// __dt__12hkBaseObjectFv is the eight-byte trivial destructor every trivial
// destructor in the image folded onto, so a member's real type is gone with
// the fold and is spelled as the object the branch reaches.
class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

// ---------------------------------------------------------------- the unit

namespace Scaleform {

enum eSFPlayerState {
    eState_Stopped = 0,
    eState_Started = 1,
    eState_Unloading = 2
};

enum eSFEventType { eEvent_None = 0 };

enum eSFBufferType { eBuffer_Update = 0, eBuffer_Render = 1 };

class Coordinator;
class CustomTextRenderer;
class ScaleformInternalEntity;

class ScaleformListener {
public:
    virtual void OnPlayerStateChanged(eSFPlayerState state);
};

class HeapAllocatorWii : public hkBaseObject {
public:
    virtual void __key();
    ~HeapAllocatorWii();

    void Create(Memory::GlobalHeapEnum heap);

    unsigned char _pad0[0xD0 - 0x4];
    hkBaseObject fD0;
    unsigned char _padD4[0x18C - 0xD4];
    System::CriticalSection heapLock;
};

class MovieLog : public GFxFSCommandHandler {
public:
    ~MovieLog();
};

class ScaleformAsset {
public:
    unsigned int textureListOffset;
    unsigned int numTextures;
    unsigned int isFont;
    unsigned int dataOffset;
};

class MovieNode {
public:
    MovieNode();
    ~MovieNode();

    void* curNGEntity;
    int size;
    void* data;
    ScaleformAsset* sfAsset;
    char assetFont;
    unsigned char loaded : 1;
    unsigned char _bits : 7;
    unsigned char _pad12[2];
    unsigned int pad;
    MovieLog log;
};

class TextureNode {
public:
    TextureNode();
    ~TextureNode();

    void* texPtr;
    World::EntityHandleBase* texHandle;
    uid id;
    char* pFilename;
    int width;
    int height;
    bool loaded : 1;
    int refCount : 31;
};

class SFEventNode {
public:
    SFEventNode(unsigned int numArgs);
    ~SFEventNode();

    eSFEventType e;
    GFxValue* arg;
    unsigned int numArgs;
    char str[52];
};

class ViewNode {
public:
    ViewNode();
    ~ViewNode();

    void RefreshViewport();

    GPtr<GFxDrawText> pMovieUpd;
    World::ScaleformOGAsset* curOGAsset;
    Util::PoolList<SFEventNode*>* pendingEvents;
    Util::PoolList<SFEventNode*> pendingEventsList;
    Memory::StaticStackAllocator memAllocator;
    int top;
    int left;
    int width;
    int height;

    bool destroyed : 1;
    bool refreshViewport : 1;
    bool refreshNextFrame : 1;
    bool renderMovie : 1;
    bool paused : 1;
    bool pauseCaughtUp : 1;
    bool loaded : 1;
    bool cleared : 1;

    bool firstUpdate : 1;
    bool pausedByTRC : 1;
    bool persist : 1;
    bool loadPage : 1;
    bool viewportOffsetEnabled : 1;
    bool _pad45 : 3;

    unsigned char _pad46[2];
    unsigned int viewnum : 21;
    int offsetX;
    int offsetY;
    int buttonState;
    float buttonRepeater[4];
    float mouseX[4];
    float mouseY[4];
    int mouseButton[4];
};

class Coordinator {
public:
    typedef Util::PoolList<MovieNode*> MovieList;
    typedef Util::PoolList<TextureNode*> TextureList;
    typedef Util::PoolList<ViewNode*> ViewList;
    typedef Util::PoolList<ScaleformListener*> ListenerList;

    Coordinator();
    ~Coordinator();

    static Coordinator* GetCoord();

    void AddListener(ScaleformListener* listener);
    void BroadcastEvent(eSFEventType toEvent);
    void ChangeSFPlayerState(eSFPlayerState state);
    void CleanupMovies();
    void ClearCustomText();
    void ClearMovie(World::ScaleformOGAsset* asset);
    void DisplayCustomText();
    TextureList::NodeType* FindTextureNode(const char* name);
    TextureList::NodeType* FindTextureNode(uid id);
    ViewNode* GetMovie(GFxMovieView* pmovie);
    ViewNode* GetMovieByAsset(World::ScaleformOGAsset* asset);
    MovieNode* GetMovieByFilename(const char* name);
    ViewNode* GetNextMovie(eSFBufferType buf);
    TextureNode* GetTextureNode(const char* name);
    void GraphicsStopAllMovies();
    void GraphicsStopMovie(ViewNode* view);
    bool IsDeadMovie(World::ScaleformOGAsset* asset);
    bool IsDyingMovie(World::ScaleformOGAsset* asset);
    bool IsWidescreen();
    void InvokeASFunc(World::ScaleformOGAsset* asset, const char* funcName,
                      unsigned int numArgs, const GFxValue* args);
    void InvokeASFunc(World::ScaleformOGAsset* asset, const char* funcName,
                      unsigned int numArgs, float* args);
    void InvokeASFunc(World::ScaleformOGAsset* asset, const char* funcName,
                      unsigned int numArgs, int* args);
    void InvokeASFunc(World::ScaleformOGAsset* asset, const char* funcName,
                      unsigned int numArgs, const char** args);
    void PauseMovie(World::ScaleformOGAsset* asset);
    void RefreshViewports();
    void SetASArray(World::ScaleformOGAsset* asset, const char* varName,
                    unsigned int numArgs, const GFxValue* args);
    void SetASVariable(World::ScaleformOGAsset* asset, const char* varName,
                       float varValue);
    void SwitchMovies();
    ViewList::NodeType* Tail() const { return viewTail.node; }
    void UnloadMovie(const char* filename);
    void ResetViewIter(eSFBufferType buf);
    void SetPausedByTRC(World::ScaleformOGAsset* asset, bool trcPause);
    void StopMovie(World::ScaleformOGAsset* asset);
    void UnclearMovie(World::ScaleformOGAsset* asset);
    void UnloadTextures(const uid* ids, int count);
    void UnpauseMovie(World::ScaleformOGAsset* asset);

    MovieList pMovieDefs;
    TextureList pTextures;
    ViewList pMovieViews;
    ListenerList listeners;
    ViewList::Iterator viewIterRend;
    ViewList::Iterator viewTail;
    HeapAllocatorWii mBlockAllocator;
    ScaleformInternalEntity* sfIntEnt;
    GFxStateBag* mLoader;
    unsigned char mActionControl[0x18];
    CustomTextRenderer* customTextRenderer;
    GPtr<GFxDrawText> pFileOpener;
    GPtr<GFxDrawText> fsHandler;
    GPtr<GFxDrawText> eiHandler;
    GPtr<GFxDrawText> strTranslator;
    GPtr<GFxDrawText> pFontMap[6];
    unsigned char fontCacheConfig[0x1C];
    unsigned char mat[0x18];
    GPtr<GFxDrawText> pRenderer;
    GPtr<GFxDrawText> pRenderConfig;
    GPtr<GFxDrawText> pImageCreator;
    int* pDevice;
    ScaleformAsset** updULMovies;
    ScaleformAsset** rendULMovies;
    char prevLangCode[6];
    unsigned int numUpdULMovies;
    unsigned int numRendULMovies;
    unsigned int updArraySize;
    unsigned int rendArraySize;

    bool firstPlay : 1;
    bool texturesLoaded : 1;
    bool moviesStopped : 1;
    bool firstMovie : 1;
    bool isWidescreen : 1;
    bool sfEnabled : 1;
    bool _pad2D4 : 2;
    unsigned char _pad2D5[3];

    virtual void __key();
};

class CustomTextRenderer {
public:
    void ClearText();
    void RenderText();
};

class FileOpener {
public:
    void RemoveFile(const char* filename);
};

}  // namespace Scaleform

// ------------------------------------------------------- the pooled lists
//
// GFx's own allocation: the heap is bound to a const reference so the
// enumerator becomes a per-call-site const object in .data.

template <class H>
inline void Free(const H& heap, void* block) {
    H h = heap;

    if (block != 0) {
        Memory::FreeGlobalHeap(block, h);
    }
}

template <class H>
inline void* Alloc(const H& heap, unsigned long size, eMemMgrTag tag) {
    return Memory::AllocGlobalHeap(size, heap, 16, tag, false);
}

inline GFxValue* NewGFxValues(const Memory::GlobalHeapEnum& heap,
                              eMemMgrTag tag, unsigned long count) {
    GFxValue* array = (GFxValue*)Memory::AllocGlobalHeap(
        count * sizeof(GFxValue), heap, 16, tag, false);

    if (array != 0) {
        for (unsigned long i = 0; i < count; i++) {
            new (&array[i]) GFxValue;
        }
    }

    return array;
}

// Declared here, DEFINED at the foot: retail CALLS this from ~ViewNode, and
// the auto-inliner only takes a body it has already read.
template <class H, class T>
void Delete(const H& heap, T* p);

// ---------------------------------------------------------------- GFx glue

GMemoryHeap::HeapDesc::HeapDesc(unsigned int flags, unsigned long minAlign,
                                unsigned long granularity,
                                unsigned long reserve, unsigned long threshold,
                                unsigned long limit, unsigned long heapId) {
    Flags = flags;
    MinAlign = minAlign;
    Granularity = granularity;
    Reserve = reserve;
    Threshold = threshold;
    Limit = limit;
    HeapId = heapId;
}

GFxState* GFxStateBag::GetStateAddRef(GFxState::StateType state) const {
    GFxStateBag* p = GetStateBagImpl();

    return p != 0 ? p->GetStateAddRef(state) : 0;
}

// The GPtr constructor is one store; retail has no out-of-line copy of it.
#pragma always_inline on
GPtr<GFxDrawText> GFxStateBag::GetFontCacheManager() const {
    return GPtr<GFxDrawText>(
        (GFxDrawText*)GetStateAddRef(GFxState::State_FontCacheManager));
}

GPtr<GFxDrawText> GFxStateBag::GetMeshCacheManager() const {
    return GPtr<GFxDrawText>(
        (GFxDrawText*)GetStateAddRef(GFxState::State_MeshCacheManager));
}

GPtr<GFxDrawText> GFxStateBag::GetFontLib() const {
    return GPtr<GFxDrawText>(
        (GFxDrawText*)GetStateAddRef(GFxState::State_FontLib));
}
#pragma always_inline off

// ------------------------------------------------------ Coordinator: movies

void Scaleform::Coordinator::PauseMovie(World::ScaleformOGAsset* asset) {
    ViewNode* curNode = GetMovieByAsset(asset);

    if (curNode != 0) {
        curNode->paused = true;
    }
}

void Scaleform::Coordinator::UnpauseMovie(World::ScaleformOGAsset* asset) {
    ViewNode* curNode = GetMovieByAsset(asset);

    if (curNode != 0) {
        curNode->paused = false;
    }
}

void Scaleform::Coordinator::SetPausedByTRC(World::ScaleformOGAsset* asset,
                                            bool trcPause) {
    ViewNode* curNode = GetMovieByAsset(asset);

    if (curNode != 0) {
        curNode->pausedByTRC = trcPause;
    }
}

void Scaleform::Coordinator::ClearMovie(World::ScaleformOGAsset* asset) {
    ViewNode* curNode = GetMovieByAsset(asset);

    if (curNode != 0) {
        curNode->cleared = true;
    }
}

void Scaleform::Coordinator::UnclearMovie(World::ScaleformOGAsset* asset) {
    ViewNode* curNode = GetMovieByAsset(asset);

    if (curNode != 0) {
        curNode->cleared = false;
    }
}

bool Scaleform::Coordinator::IsDyingMovie(World::ScaleformOGAsset* asset) {
    ViewNode* curNode = GetMovieByAsset(asset);

    return curNode != 0 ? curNode->destroyed : false;
}

bool Scaleform::Coordinator::IsDeadMovie(World::ScaleformOGAsset* asset) {
    ViewNode* curNode = GetMovieByAsset(asset);

    return curNode == 0;
}

void Scaleform::Coordinator::StopMovie(World::ScaleformOGAsset* asset) {
    ViewNode* curNode = GetMovieByAsset(asset);

    if (curNode != 0) {
        if (curNode->pMovieUpd.pObject != 0) {
            curNode->curOGAsset->OnMovieStopped();
            ((GFxMovieView*)curNode->pMovieUpd.pObject)->SetPlayState(0);
        }

        curNode->destroyed = true;
    }
}

void Scaleform::Coordinator::GraphicsStopMovie(ViewNode* view) {
    mBlockAllocator.heapLock.Enter();

    if (view->pMovieUpd.pObject != 0) {
        ((GFxMovieView*)view->pMovieUpd.pObject)->Release(0);
        *(GPtr<GFxMovieView>*)&view->pMovieUpd = 0;
    }

    view->curOGAsset = 0;

    mBlockAllocator.heapLock.Exit();
}

// NEAR MISS: 8 of 38 words; retail RE-READS viewTail (lwz r0,116) for the
// final viewIterRend assignment where ours shares the loop condition's load,
// so everything after it is a word early (tried: the loop condition through
// an inline accessor, through the raw member, and the assignment through a
// volatile view).
void Scaleform::Coordinator::GraphicsStopAllMovies() {
    mBlockAllocator.heapLock.Enter();

    for (ViewList::NodeType* it = pMovieViews.Begin(); it != Tail();
         it = (ViewList::NodeType*)it->next) {
        ViewNode* view = it->data;

        if (!view->persist) {
            if (view->pMovieUpd.pObject != 0) {
                ((GFxMovieView*)view->pMovieUpd.pObject)->Release(0);
                *(GPtr<GFxMovieView>*)&view->pMovieUpd = 0;
            }

            view->curOGAsset = 0;
        }
    }

    viewIterRend = viewTail;

    mBlockAllocator.heapLock.Exit();
}

void Scaleform::Coordinator::ResetViewIter(eSFBufferType buf) {
    if (firstMovie) {
        return;
    }

    viewIterRend.node = pMovieViews.Begin();
}

void Scaleform::Coordinator::RefreshViewports() {
    for (ViewList::NodeType* it = pMovieViews.Begin(); it != viewTail.node;
         it = (ViewList::NodeType*)it->next) {
        it->data->refreshNextFrame = true;
    }
}

void Scaleform::Coordinator::ClearCustomText() {
    CustomTextRenderer* renderer = customTextRenderer;

    if (renderer == 0) {
        return;
    }

    renderer->ClearText();
}

void Scaleform::Coordinator::DisplayCustomText() {
    CustomTextRenderer* renderer = customTextRenderer;

    if (renderer == 0) {
        return;
    }

    renderer->RenderText();
}

void Scaleform::Coordinator::AddListener(ScaleformListener* listener) {
    mBlockAllocator.heapLock.Enter();

    listeners.Util::PoolListBase::PushBack();
    ((ListenerList::NodeType*)listeners.tail.prev)->data = listener;

    mBlockAllocator.heapLock.Exit();
}

void Scaleform::Coordinator::ChangeSFPlayerState(eSFPlayerState state) {
    for (ListenerList::NodeType* it = listeners.Begin(); it != listeners.End();
         it = (ListenerList::NodeType*)it->next) {
        it->data->OnPlayerStateChanged(state);
    }
}

Scaleform::ViewNode* Scaleform::Coordinator::GetNextMovie(eSFBufferType buf) {
    if (firstMovie) {
        return 0;
    }

    ViewList::NodeType* curNode = viewIterRend.node;

    if (curNode == 0 || curNode == viewTail.node) {
        return 0;
    }

    viewIterRend = viewIterRend++;

    return curNode->data;
}

Scaleform::ViewNode* Scaleform::Coordinator::GetMovie(GFxMovieView* pmovie) {
    if (firstMovie) {
        return 0;
    }

    ViewList::Iterator it;

    it.node = pMovieViews.Begin();

    ViewList::NodeType* end = pMovieViews.End();
    ViewNode* curNode = 0;

    while (curNode == 0 && it.node != end) {
        if ((GFxMovieView*)it.node->data->pMovieUpd.pObject == pmovie) {
            curNode = it.node->data;
        }

        it = it++;
    }

    return curNode;
}

Scaleform::TextureNode* Scaleform::Coordinator::GetTextureNode(
    const char* name) {
    TextureNode* node = 0;

    TextureList::NodeType* it = FindTextureNode(name);
    TextureList::NodeType* end = pTextures.End();

    if (it != end) {
        node = it->data;
    }

    return node;
}

Scaleform::MovieNode* Scaleform::Coordinator::GetMovieByFilename(
    const char* name) {
    MovieList::NodeType* it = pMovieDefs.Begin();

    MovieNode* curNode = 0;

    while (curNode == 0 && it != pMovieDefs.End()) {
        MovieNode* node = it->data;

        if (strcmp((const char*)(node->sfAsset + 1), name) == 0) {
            curNode = node;
        }

        it = (MovieList::NodeType*)it->next;
    }

    return curNode;
}

// Below its callers: retail CALLS this from each of the movie accessors.
Scaleform::ViewNode* Scaleform::Coordinator::GetMovieByAsset(
    World::ScaleformOGAsset* asset) {
    if (firstMovie) {
        return 0;
    }

    ViewList::NodeType* it = pMovieViews.Begin();

    ViewNode* curNode = 0;

    while (curNode == 0 && it != viewTail.node) {
        if (it->data->curOGAsset == asset) {
            curNode = it->data;
        }

        it = (ViewList::NodeType*)it->next;
    }

    return curNode;
}

// Below GetTextureNode, which retail calls it from.
Scaleform::Coordinator::TextureList::NodeType*
Scaleform::Coordinator::FindTextureNode(const char* name) {
    TextureList::NodeType* it = pTextures.Begin();
    TextureList::NodeType* end = pTextures.End();

    while (it != end) {
        if (strcmp(it->data->pFilename, name) == 0) {
            break;
        }

        it = (TextureList::NodeType*)it->next;
    }

    return it;
}

Scaleform::Coordinator::TextureList::NodeType*
Scaleform::Coordinator::FindTextureNode(uid id) {
    TextureList::NodeType* it = pTextures.Begin();
    TextureList::NodeType* end = pTextures.End();

    while (it != end) {
        TextureNode* tn = it->data;

        if (tn->id == id) {
            break;
        }

        it = (TextureList::NodeType*)it->next;
    }

    return it;
}

// ------------------------------------------------------------------- nodes

Scaleform::TextureNode::TextureNode() {
    texPtr = 0;
    refCount = 0;
    texHandle = 0;
    id = 0;
    pFilename = 0;
    width = height = refCount;
    loaded = false;
}

// --------------------------------------------------------------- GFx small

GFxValue& GFxValue::operator=(const GFxValue& src) {
    Type = src.Type;
    Value.Data0 = src.Value.Data0;
    Value.Data1 = src.Value.Data1;

    return *this;
}

GFxKeyEvent::GFxKeyEvent(GFxEvent::EventType eventType, GFxKey::Code keyCode,
                         unsigned char asciiCode, unsigned long wcharCode) {
    Id = 0;
    Type = eventType;
    KeyCode = keyCode;
    AsciiCode = asciiCode;
    WcharCode = wcharCode;
}

GFxMouseEvent::GFxMouseEvent(GFxEvent::EventType eventType, unsigned int button,
                             float x, float y, float scrollDelta,
                             unsigned int mouseIndex) {
    Id = 0;
    Type = eventType;
    Button = button;
    this->x = x;
    this->y = y;
    ScrollDelta = scrollDelta;
    MouseIndex = mouseIndex;
}

template <class T>
GPtr<T>& GPtr<T>::operator=(T* p) {
    if (p != 0) {
        p->AddRef();
    }

    if (pObject != 0) {
        pObject->Release(0);
    }

    pObject = p;

    return *this;
}

// At the foot of the file so it is not taken inline where retail calls it.
template <class T>
typename Util::PoolList<T>::Iterator Util::PoolList<T>::Iterator::operator++(
    int) {
    Iterator it;

    it.node = (NodeType*)this->node->next;

    return it;
}


// ------------------------------------------------------------ the AS queue

void Scaleform::Coordinator::BroadcastEvent(eSFEventType toEvent) {
    ViewList::NodeType* it = pMovieViews.Begin();

    while (it != viewTail.node) {
        ViewNode* curNode = GetMovieByAsset(it->data->curOGAsset);

        if (curNode == 0) {
            it = (ViewList::NodeType*)it->next;
            continue;
        }

        SFEventNode* curEvent = new (Memory::AllocGlobalHeap(
            sizeof(SFEventNode), (Memory::GlobalHeapEnum)0, 16, (eMemMgrTag)80,
            false)) SFEventNode(0);

        curEvent->e = toEvent;
        ((Util::PoolList<zBTTask*>*)curNode->pendingEvents)
            ->PushBack(*(zBTTask**)&curEvent);

        it = (ViewList::NodeType*)it->next;
    }
}

void Scaleform::Coordinator::InvokeASFunc(World::ScaleformOGAsset* asset,
                                          const char* funcName,
                                          unsigned int numArgs,
                                          const GFxValue* args) {
    ViewNode* curNode = GetMovieByAsset(asset);

    if (curNode == 0 || curNode->paused) {
        return;
    }

    {
        SFEventNode* curEvent = new (Memory::AllocGlobalHeap(
            sizeof(SFEventNode), (Memory::GlobalHeapEnum)0, 16, (eMemMgrTag)80,
            false)) SFEventNode(numArgs);

        strcpy(curEvent->str, funcName);

        for (unsigned int i = 0; i < numArgs; i++) {
            curEvent->arg[i] = args[i];
        }

        curEvent->e = (eSFEventType)20;
        ((Util::PoolList<zBTTask*>*)curNode->pendingEvents)
            ->PushBack(*(zBTTask**)&curEvent);
    }
}

void Scaleform::Coordinator::InvokeASFunc(World::ScaleformOGAsset* asset,
                                          const char* funcName,
                                          unsigned int numArgs, float* args) {
    ViewNode* curNode = GetMovieByAsset(asset);

    if (curNode == 0 || curNode->paused) {
        return;
    }

    {
        SFEventNode* curEvent = new (Memory::AllocGlobalHeap(
            sizeof(SFEventNode), (Memory::GlobalHeapEnum)0, 16, (eMemMgrTag)80,
            false)) SFEventNode(numArgs);

        strcpy(curEvent->str, funcName);

        for (unsigned int i = 0; i < numArgs; i++) {
            curEvent->arg[i].SetNumber(args[i]);
        }

        curEvent->e = (eSFEventType)20;
        ((Util::PoolList<zBTTask*>*)curNode->pendingEvents)
            ->PushBack(*(zBTTask**)&curEvent);
    }
}

void Scaleform::Coordinator::InvokeASFunc(World::ScaleformOGAsset* asset,
                                          const char* funcName,
                                          unsigned int numArgs, int* args) {
    ViewNode* curNode = GetMovieByAsset(asset);

    if (curNode == 0 || curNode->paused) {
        return;
    }

    {
        SFEventNode* curEvent = new (Memory::AllocGlobalHeap(
            sizeof(SFEventNode), (Memory::GlobalHeapEnum)0, 16, (eMemMgrTag)80,
            false)) SFEventNode(numArgs);

        strcpy(curEvent->str, funcName);

        for (unsigned int i = 0; i < numArgs; i++) {
            curEvent->arg[i].SetNumber(args[i]);
        }

        curEvent->e = (eSFEventType)20;
        ((Util::PoolList<zBTTask*>*)curNode->pendingEvents)
            ->PushBack(*(zBTTask**)&curEvent);
    }
}

void Scaleform::Coordinator::InvokeASFunc(World::ScaleformOGAsset* asset,
                                          const char* funcName,
                                          unsigned int numArgs,
                                          const char** args) {
    ViewNode* curNode = GetMovieByAsset(asset);

    if (curNode == 0 || curNode->paused || curNode->destroyed) {
        return;
    }

    {
        SFEventNode* curEvent = new (Memory::AllocGlobalHeap(
            sizeof(SFEventNode), (Memory::GlobalHeapEnum)0, 16, (eMemMgrTag)80,
            false)) SFEventNode(numArgs);

        strcpy(curEvent->str, funcName);

        for (unsigned int i = 0; i < numArgs; i++) {
            const char* str = args[i];

            if (str != 0) {
                GFxValue* value = &curEvent->arg[i];

                value->Type = 4;
                value->Value.Data0 = (unsigned int)str;
            } else {
                curEvent->arg[i].Type = 1;
            }
        }

        curEvent->e = (eSFEventType)20;
        ((Util::PoolList<zBTTask*>*)curNode->pendingEvents)
            ->PushBack(*(zBTTask**)&curEvent);
    }
}

void Scaleform::Coordinator::SetASArray(World::ScaleformOGAsset* asset,
                                        const char* varName,
                                        unsigned int numArgs,
                                        const GFxValue* args) {
    ViewNode* curNode = GetMovieByAsset(asset);

    if (curNode == 0 || curNode->paused) {
        return;
    }

    {
        SFEventNode* curEvent = new (Memory::AllocGlobalHeap(
            sizeof(SFEventNode), (Memory::GlobalHeapEnum)0, 16, (eMemMgrTag)80,
            false)) SFEventNode(numArgs);

        strcpy(curEvent->str, varName);

        curEvent->e = (eSFEventType)22;

        for (unsigned int i = 0; i < numArgs; i++) {
            curEvent->arg[i] = args[i];
        }

        ((Util::PoolList<zBTTask*>*)curNode->pendingEvents)
            ->PushBack(*(zBTTask**)&curEvent);
    }
}

void Scaleform::Coordinator::SetASVariable(World::ScaleformOGAsset* asset,
                                           const char* varName,
                                           float varValue) {
    ViewNode* curNode = GetMovieByAsset(asset);

    if (curNode == 0 || curNode->paused) {
        return;
    }

    {
        SFEventNode* curEvent = new (Memory::AllocGlobalHeap(
            sizeof(SFEventNode), (Memory::GlobalHeapEnum)0, 16, (eMemMgrTag)80,
            false)) SFEventNode(1);

        strcpy(curEvent->str, varName);

        GFxValue* value = curEvent->arg;

        value->Type = 3;
        *(double*)&value->Value = varValue;

        curEvent->e = (eSFEventType)21;
        ((Util::PoolList<zBTTask*>*)curNode->pendingEvents)
            ->PushBack(*(zBTTask**)&curEvent);
    }
}

// --------------------------------------------------------- unload / switch

// NEAR MISS: 8 of 71 words, all register numbers: retail holds the iterator
// in r31 and the unity .data base in r30, where this gets the base in r31 and
// the iterator in r30 (tried the three declaration orders of it / end /
// curNode; this one is the closest).
void Scaleform::Coordinator::UnloadMovie(const char* filename) {
    mBlockAllocator.heapLock.Enter();

    if (rendULMovies == 0) {
        rendArraySize = pMovieDefs.size;
        rendULMovies = (ScaleformAsset**)Alloc(
            Memory::GlobalHeap, pMovieDefs.size * 4, (eMemMgrTag)80);
    }

    MovieList::NodeType* it = pMovieDefs.Begin();
    MovieList::NodeType* end = pMovieDefs.End();
    MovieNode* curNode = 0;

    while (curNode == 0 && it != end) {
        if (strcmp((const char*)(it->data->sfAsset + 1), filename) == 0) {
            curNode = it->data;
            rendULMovies[numRendULMovies] = curNode->sfAsset;

            if (curNode != 0) {
                curNode->~MovieNode();
            }

            Free(Memory::GlobalHeap, curNode);

            pMovieDefs.Erase(it);
        } else {
            it = (MovieList::NodeType*)it->next;
        }
    }

    numRendULMovies++;
    ((FileOpener*)pFileOpener.pObject)->RemoveFile(filename);

    mBlockAllocator.heapLock.Exit();
}

// NEAR MISS: 2 of 71 words -- retail loads pMovieViews.Begin() one
// instruction EARLIER, before the store of updULMovies rather than after it;
// the two words are that swap. A scheduling tie: the load sits after the
// store whether the local is declared before or after the assignments.
void Scaleform::Coordinator::SwitchMovies() {
    if (firstMovie) {
        return;
    }

    mBlockAllocator.heapLock.Enter();

    if (updULMovies != 0) {
        Free(Memory::GlobalHeap, updULMovies);
    }

    ViewNode* view;
    ViewList::NodeType* it = pMovieViews.Begin();

    updULMovies = rendULMovies;
    rendULMovies = 0;

    updArraySize = rendArraySize;
    rendArraySize = 0;

    numUpdULMovies = numRendULMovies;
    numRendULMovies = 0;

    viewTail.node = pMovieViews.End();

    while (it != viewTail.node) {
        view = it->data;

        if (view->curOGAsset == 0) {
            if (view != 0) {
                view->~ViewNode();
            }

            Free(Memory::GlobalHeap, view);

            ViewList::NodeType* dead = it;
            it = (ViewList::NodeType*)it->next;
            pMovieViews.Erase(dead);
        } else {
            it = (ViewList::NodeType*)it->next;
        }
    }

    moviesStopped = (pMovieViews.size == 0);

    viewIterRend.node = pMovieViews.Begin();
    viewTail.node = pMovieViews.End();

    mBlockAllocator.heapLock.Exit();
}

void Scaleform::Coordinator::UnloadTextures(const uid* texList,
                                            int numTextures) {
    for (int i = 0; i < numTextures; i++) {
        TextureList::NodeType* node = FindTextureNode(texList[i]);
        TextureNode* curTexNode = node->data;

        curTexNode->refCount--;

        if (curTexNode->refCount == 0) {
            pTextures.Erase(node);

            if (curTexNode != 0) {
                curTexNode->~TextureNode();
            }

            Free(Memory::GlobalHeap, curTexNode);
        }
    }
}

void Scaleform::Coordinator::CleanupMovies() {
    for (unsigned int i = 0; i < numUpdULMovies; i++) {
        ScaleformAsset* asset = updULMovies[i];

        UnloadTextures((const uid*)((char*)asset + asset->textureListOffset),
                       asset->numTextures);
    }
}

// ------------------------------------------------------------- node bodies

#pragma always_inline on
Scaleform::SFEventNode::SFEventNode(unsigned int na) {
    numArgs = na;

    if (na != 0) {
        arg = NewGFxValues(Memory::GlobalHeap, (eMemMgrTag)80, na);
    } else {
        arg = 0;
    }
}
#pragma always_inline off

Scaleform::SFEventNode::~SFEventNode() {
    if (arg != 0) {
        Free(Memory::GlobalHeap, arg);
    }
}

Scaleform::TextureNode::~TextureNode() {
    if (texHandle != 0 && texHandle->refCount > 0) {
        texHandle->Deactivate(Util::Referrer(Coordinator::GetCoord()));
    }
}

Scaleform::ViewNode::~ViewNode() {
    Util::PoolList<SFEventNode*>* list = pendingEvents;

    Util::PoolList<SFEventNode*>::NodeType* end = list->End();
    Util::PoolList<SFEventNode*>::NodeType* it = list->Begin();

    while (it != end) {
        Delete(Memory::GlobalHeap, it->data);

        it = (Util::PoolList<SFEventNode*>::NodeType*)it->next;
    }

    memAllocator.mark -= 1536;

    Free(Memory::GlobalHeap, memAllocator.buffer);
}

// ---------------------------------------------------- the existing matches

// Scaleform::HeapAllocatorWii's destructor, the compiler's own. The
// null-this test, the member at +208 destroyed with the don't-delete
// flag, then the BASE on `this` with the flag CLEAR, then operator
// delete when the caller's flag is positive, and `return this`. r4 = 0
// on the second call is what says base rather than member; r4 = -1 is a
// complete subobject.
//
// Both calls folded onto __dt__12hkBaseObjectFv, the eight-byte survivor
// every trivial destructor in the image collapsed onto, so neither the
// member's real type nor the base's is in the linked image; each is
// spelled as the object the branch reaches.
Scaleform::HeapAllocatorWii::~HeapAllocatorWii() {}

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
Scaleform::MovieLog::~MovieLog() {}

// Defined at the foot, below ~ViewNode, which retail CALLS it from.
template <class H, class T>
void Delete(const H& heap, T* p) {
    if (p != 0) {
        p->~T();
    }

    Free(heap, p);
}
