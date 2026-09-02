// zModuleMgr_Registry.cpp -- one function, read from the image with
// tools/disasm.py: the registry's startup registers the NPC manager,
// starts the projectile, achievements and POW managers, registers the
// NPC ninja manager and starts the search manager. The strings are the
// unity pool's, hence the generated header first. The call between the
// POW manager and the second registration lands on a lone `blr` that
// the image names as Math::Matrix33's constructor -- the weak empty
// function every empty function folded into -- so it is called by that
// symbol; what the original called there had an empty body and no name
// that survived. Both managers are file statics in retail, so this
// unit matches and does not link.

#include "SB/GM/Engine/Game/zModuleMgr_Registry.pool.h"

class zModule;
class zNPCNinjaManager;

enum enModulePriority { enModulePriority_ = 0x7FFFFFFF };

void zModuleMgr_RegisterModule(zModule* module, int id, const char* name,
                               enModulePriority update,
                               enModulePriority render);

void zProjectileManager_Startup();
void zAchievementsMgr_Startup();
void zPOWManager_Startup();
void zSearchManager_Startup();

extern "C" void __ct__Q24Math8Matrix33Fv();

extern zModule gNPCManager;
extern zNPCNinjaManager gNPCNinjaManager;

void zModuleMgr_Registry_Startup() {
    zModuleMgr_RegisterModule(&gNPCManager, 'NPCM', "NPC Manager",
                              (enModulePriority)1, (enModulePriority)1);

    zProjectileManager_Startup();
    zAchievementsMgr_Startup();
    zPOWManager_Startup();

    __ct__Q24Math8Matrix33Fv();

    zModuleMgr_RegisterModule((zModule*)&gNPCNinjaManager, 'NNMG',
                              "NPC Ninja Manager", (enModulePriority)1,
                              (enModulePriority)1);

    zSearchManager_Startup();
}
