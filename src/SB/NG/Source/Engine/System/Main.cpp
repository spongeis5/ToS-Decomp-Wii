// Main.cpp -- one function, read from the image with tools/disasm.py:
// main initialises the OS and its fast-cast mode, the physical heap if
// nothing has yet, installs the reset and power callbacks, opens the
// render module's critical section, runs the game, and returns what it
// returned. The call after GameMain, made on the render module, lands
// on a lone `blr` that the image names as Math::Matrix33's constructor
// -- the weak empty function every empty function folded into -- so it
// is called by that symbol; what the original called there had an
// empty body and no name that survived.
//
// Main.cpp is a fragment of the NG WAD02 unity build. OSInitFastCast is
// the SDK header's static inline, instantiated once for that whole
// unity unit as a local symbol right after main (0x801FBA60, 52 bytes)
// and called from HomeMenu's fragment as well; dtk names it
// OSInitFastCast_801FBA60 for the other carved objects. Defined here
// (static, an asm body) it matched and left that reference undefined,
// so this unit matches and does not link until the fragment can export
// the instance under that name.

extern "C" {
void OSInit(void);
void OSInitFastCast(void);

typedef void (*OSResetCallback)(void);
typedef void (*OSPowerCallback)(void);

OSResetCallback OSSetResetCallback(OSResetCallback callback);
OSPowerCallback OSSetPowerCallback(OSPowerCallback callback);

void __ct__Q24Math8Matrix33Fv(void* module);
}

namespace TRC {
void ResetCallback();
void PowerCallback();
}  // namespace TRC

namespace Memory {
extern bool IsHeapInitialized;
void InitializePhysicalMemory();
}  // namespace Memory

namespace Graphics {

class RenderModule {
public:
    void InitCriticalSection();
};

extern RenderModule* renderModule;

}  // namespace Graphics

int GameMain(int argc, char** argv);

int main(int argc, char** argv) {
    OSInit();
    OSInitFastCast();

    if (!Memory::IsHeapInitialized) {
        Memory::InitializePhysicalMemory();
    }

    OSSetResetCallback(TRC::ResetCallback);
    OSSetPowerCallback(TRC::PowerCallback);

    Graphics::renderModule->InitCriticalSection();

    int result = GameMain(argc, argv);

    __ct__Q24Math8Matrix33Fv(Graphics::renderModule);

    return result;
}
