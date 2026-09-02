// Graphics.cpp -- eight functions, read from the image with
// tools/disasm.py. The render module's constructor builds the
// System::Module base and sets the first event stage to 67; its
// InitCriticalSection clears the render-thread critical section and
// initialises its mutex. The four thread and frame functions bracket
// the module manager's render calls with the in-render-phase flag,
// StartRenderThread creating the render and post-render channels first,
// StopRenderThread destroying the channel, and FrameStepEnd doing a
// whole frame under the critical section: swap the update side,
// invalidate the GX caches, render the scene graph, end the frame,
// swap the frame buffers and the render side, then yield. The two
// critical-section wrappers tail-call Enter and Exit.
//
// System::Module's vptr follows its name and event set (+0x14), where
// the constructor stores the vtable; the first virtual is left
// undefined here so the vtable's home stays in the unity unit's data.
// PostRenderChannel::Destroy is empty and retail branches to the shared
// folded blr (__ct__Q24Math8Matrix33Fv), so that is the name called
// here, as the other units do. The in-render-phase flag is a file
// static in the unity unit's unnamed namespace, so retail's symbol
// carries WAD01's name and this one carries its own; the load is a
// masked relocation either way, and the unit matches without linking.

extern "C" {
struct OSMutex {
    unsigned char _pad0[0x18];
};

struct OSThread;

void OSInitMutex(OSMutex* mutex);
void OSYieldThread(void);
void GXInvalidateVtxCache(void);
void GXInvalidateTexAll(void);

// The shared empty function every folded empty body branches to.
void __ct__Q24Math8Matrix33Fv(void);
}

namespace System {

class EventSet {
public:
    int stage[4];
};

class Module {
public:
    Module();

    char* name;
    EventSet events;

    virtual void _v0();

    int contextFlags;
    short eventBindingIndices[60];
    bool enabled;
};

class CriticalSection {
public:
    void Enter();
    void Exit();

    int refcount;
    OSMutex mutex;
    void* owner;
};

class ModuleManager {
public:
    static void RenderStartup();
    static void RenderShutdown();
    static void RenderFrameBegin();
    static void RenderFrameEnd();
    static void SwapUpdate();
    static void SwapRender();
};

}  // namespace System

namespace Memory {
void SwapRenderFrameBuffers();
}  // namespace Memory

namespace {
bool inRenderPhase;
}  // namespace

namespace Graphics {

class Channel {
public:
    static void Create(OSThread* update, OSThread* render);
    static void Destroy();
};

class PostRenderChannel {
public:
    static void Create(OSThread* update, OSThread* render);
};

class SceneGraph {
public:
    static void RenderFrame();
};

class RenderModule : public System::Module {
public:
    RenderModule();

    void InitCriticalSection();
};

System::CriticalSection renderThreadCriticalSection;

void RenderLoopEnterCS();
void RenderLoopExitCS();
void StartRenderThread();
void StopRenderThread();
void FrameStepBegin();
void FrameStepEnd();

}  // namespace Graphics

Graphics::RenderModule::RenderModule() {
    events.stage[0] = 67;
}

void Graphics::RenderModule::InitCriticalSection() {
    renderThreadCriticalSection.refcount = 0;
    OSInitMutex(&renderThreadCriticalSection.mutex);
}

void Graphics::RenderLoopEnterCS() {
    renderThreadCriticalSection.Enter();
}

void Graphics::RenderLoopExitCS() {
    renderThreadCriticalSection.Exit();
}

void Graphics::StartRenderThread() {
    inRenderPhase = false;

    Channel::Create(0, 0);
    PostRenderChannel::Create(0, 0);

    inRenderPhase = true;
    System::ModuleManager::RenderStartup();
    inRenderPhase = false;
}

void Graphics::StopRenderThread() {
    Channel::Destroy();
    __ct__Q24Math8Matrix33Fv();  // PostRenderChannel::Destroy(), empty

    inRenderPhase = true;
    System::ModuleManager::RenderShutdown();
    inRenderPhase = false;
}

void Graphics::FrameStepBegin() {
    inRenderPhase = true;
    System::ModuleManager::RenderFrameBegin();
    inRenderPhase = false;
}

void Graphics::FrameStepEnd() {
    System::ModuleManager::SwapUpdate();

    renderThreadCriticalSection.Enter();

    inRenderPhase = true;

    GXInvalidateVtxCache();
    GXInvalidateTexAll();
    SceneGraph::RenderFrame();
    System::ModuleManager::RenderFrameEnd();
    Memory::SwapRenderFrameBuffers();
    System::ModuleManager::SwapRender();

    inRenderPhase = false;

    renderThreadCriticalSection.Exit();

    OSYieldThread();
}
