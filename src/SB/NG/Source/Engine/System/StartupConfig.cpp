// StartupConfig.cpp -- one function, read from the image with
// tools/disasm.py. System::LoadDefaultStartupConfig points System::config at
// the unity unit's own StartupConfig object and fills it: the two system
// thread stacks, the six graphics sizes, the media root and its path
// delimiter, the three loader file names and the pack-blob ceiling. The
// WiiHIO2Tool field is left as it is.
//
// Layout from the DWARF (tools/dwarf_types.py, and the anonymous member
// structs read straight out of the DIE): System::StartupConfig is 0x3C, four
// nested anonymous structs -- mem 0x20 (system 0x8: threadStack,
// debugThreadStack; graphics 0x18: sysStack, threadStack, debugThreadStack,
// channel, postRenderChannel, limbo), io 0x20..0x27 (mediaIO: contentRoot and
// the char pathDelim, whose three bytes of padding are why the next member is
// at 0x28), loader 0x28..0x37 (bootINIFile, domainPathDB, configWidgetPF,
// maxPackBlobs) and wiiDebug 0x38 (wiiHIO2Tool). Every value is the
// instruction's own immediate: 0xA0000, 0x20000, 0, 0x480000, 0x20000,
// 0x300000, 0x100000, 0x100000, then 47 -- a '/' -- and 100000, which retail
// builds as `addi r3,r12,-31072` off the 0x20000 register it already had.
//
// The strings are the unity unit's POOL, so the generated prefix header comes
// first: "boot.ini" is at +1667 and belongs to a file ahead of this one, and
// this unit is the first to reference ".", "domainUID.dir" and "Game.HE" at
// +1676, +1678 and +1692. They have to appear in the source in that order or
// every offset after the first disagreement is wrong.
//
// Four shapes the bytes fixed. The LAST TWO ASSIGNMENTS ARE IN THE ORDER
// RETAIL STORES THEM, maxPackBlobs before configWidgetPF, and that one swap is
// worth nine words. Written in offset order instead, the function is the same
// 37 words with the same structure but "Game.HE" takes r3 and 100000 takes r0
// where retail has them the other way round -- and reusing r3 for the last
// string kills the pool base, so the base itself moves from r6 to r3 and the
// four `addi` off it change with it. Retail keeps the base in r6 for all four
// strings and then reuses r6 for the '/' constant. The object is a file static
// in an ANONYMOUS
// NAMESPACE -- retail spells it startupConfig__19@unnamed@WAD02_cpp@, after
// the unity unit's file, and a fragment cannot spell that: CodeWarrior mangles
// an anonymous namespace with the MAIN file's basename, so ours comes out
// @unnamed@StartupConfig_cpp@. Every reference to it is a masked data
// relocation, so the bytes are unaffected and the unit's own definition is the
// honest source; what it costs is the link and the report's pairing, not the
// comparison. `System::config` is assigned FIRST, before any field, which is
// the order the stores come out in. And the first field's store is written
// off the section base with the object's own low half -- `stw r31,9960(r28)`
// is a LO relocation against startupConfig, not a displacement -- while the
// rest go through the register holding the object address; that is the
// compiler's own choice and no spelling was needed for it.

#include "SB/NG/Source/Engine/System/StartupConfig.pool.h"

enum WiiHIO2Tool { WiiHIO2Tool_NONE = 0 };

namespace System {

class StartupConfig {
public:
    struct {
        struct {
            int threadStack;
            int debugThreadStack;
        } system;

        struct {
            int sysStack;
            int threadStack;
            int debugThreadStack;
            int channel;
            int postRenderChannel;
            int limbo;
        } graphics;
    } mem;

    struct {
        struct {
            const char* contentRoot;
            char pathDelim;
        } mediaIO;
    } io;

    struct {
        const char* bootINIFile;
        const char* domainPathDB;
        const char* configWidgetPF;
        int maxPackBlobs;
    } loader;

    struct {
        WiiHIO2Tool wiiHIO2Tool;
    } wiiDebug;
};

StartupConfig* config;

void LoadDefaultStartupConfig();

}  // namespace System

namespace {

System::StartupConfig startupConfig;

}  // namespace

void System::LoadDefaultStartupConfig() {
    config = &startupConfig;

    startupConfig.mem.system.threadStack = 0xA0000;
    startupConfig.mem.system.debugThreadStack = 0x20000;

    startupConfig.mem.graphics.sysStack = 0;
    startupConfig.mem.graphics.threadStack = 0x480000;
    startupConfig.mem.graphics.debugThreadStack = 0x20000;
    startupConfig.mem.graphics.channel = 0x300000;
    startupConfig.mem.graphics.postRenderChannel = 0x100000;
    startupConfig.mem.graphics.limbo = 0x100000;

    startupConfig.io.mediaIO.contentRoot = ".";
    startupConfig.io.mediaIO.pathDelim = '/';

    startupConfig.loader.bootINIFile = "boot.ini";
    startupConfig.loader.domainPathDB = "domainUID.dir";
    startupConfig.loader.maxPackBlobs = 100000;
    startupConfig.loader.configWidgetPF = "Game.HE";
}
