// C:/branches/SB09/main/GM/Engine/Game/zNPCType.cpp
//
// Layout and signature from the Wii build's DWARF:
//
//   class zNPCType  /* 0x3C bytes */
//   {
//       /* +0x0  */ enum eNPCType npcTypeEnum;
//       /* +0x4  */ enum eAnimSetType animSetType;
//       /* +0x8  */ char* typeName;
//       /* +0xC  */ zNPCBase* (*allocateNPCFunction)(...);
//       /* +0x10 */ unsigned int npcTID_ID;
//       /* +0x14 */ bool hasEntity;
//       /* +0x18 */ CreatorI* logicCreator;
//       /* +0x1C */ CreatorI* steeringCreator;
//       /* +0x20 */ CreatorI* perceptionCreator;
//       /* +0x24 */ CreatorI* combatCreator;
//       /* +0x28 */ CreatorI* quickTimeCombatCreator;
//       /* +0x2C */ CreatorI* fxCreator;
//       /* +0x30 */ CreatorI* extraModelCreator[2];
//   };
//
// The store order is 0, 4, 8, C, 10, 14, 18, 1C, 20, 2C, then a 2-iteration
// loop over 0x30. So combatCreator (0x24) and quickTimeCombatCreator (0x28)
// are NOT cleared, and that is not an omission here -- the retail code
// skips them, and writing them would add two stores.
//
// NEAR-MISS at 84.53%, 19 words against retail's 19 -- the STRUCTURE is
// identical and only the register NUMBERS differ:
//
//   retail                ours
//   li  r10, 0            li  r9, 0
//   mr  r9, r10           mr  r4, r9
//   add r4, r3, r9        add r5, r3, r4
//   stw r10, 0x30(r4)     stw r9, 0x30(r5)
//
// Retail keeps the zero in r10 and the loop offset in r9; we get r9 and r4,
// because mwcc frees r4 as soon as its parameter has been stored and reuses
// it. Five spellings were compiled and scored -- loop variable declared
// first, initialised first, a named null local, both, and a while loop --
// and all five sit at 8 of 19 words. So it is not declaration order either.
//
// It is NOT the optimisation setting: -O4,s is what took this function from
// 14 words to the correct 19, and took zPlayerContainer to exact. What is
// left here is allocation order alone.

namespace Sext {
namespace AnimationSet {
enum eAnimSetType {
    eAnimSetType_Zero
};
}
}

namespace World {
class EntityHandleBase;
}

class zNPCBase;
class CreatorI;

enum eNPCType {
    eNPCType_Zero
};

class zNPCType {
public:
    void Setup(eNPCType type, Sext::AnimationSet::eAnimSetType animSet,
               const char* name,
               zNPCBase* (*allocate)(World::EntityHandleBase*),
               unsigned int tid);

    eNPCType npcTypeEnum;
    Sext::AnimationSet::eAnimSetType animSetType;
    char* typeName;
    zNPCBase* (*allocateNPCFunction)(World::EntityHandleBase*);
    unsigned int npcTID_ID;
    bool hasEntity;
    CreatorI* logicCreator;
    CreatorI* steeringCreator;
    CreatorI* perceptionCreator;
    CreatorI* combatCreator;
    CreatorI* quickTimeCombatCreator;
    CreatorI* fxCreator;
    CreatorI* extraModelCreator[2];
};

void zNPCType::Setup(eNPCType type, Sext::AnimationSet::eAnimSetType animSet,
                     const char* name,
                     zNPCBase* (*allocate)(World::EntityHandleBase*),
                     unsigned int tid) {
    int i;
    npcTypeEnum = type;
    animSetType = animSet;
    typeName = (char*)name;
    allocateNPCFunction = allocate;
    npcTID_ID = tid;
    hasEntity = false;
    logicCreator = 0;
    steeringCreator = 0;
    perceptionCreator = 0;
    fxCreator = 0;

    for (i = 0; i < 2; i++) {
        extraModelCreator[i] = 0;
    }
}
