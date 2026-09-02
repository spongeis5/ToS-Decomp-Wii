// zNPCType.cpp -- one function, read from the image with tools/disasm.py:
// zNPCType::Setup stores its five arguments in order, clears the entity
// flag and the logic, steering, perception and fx creators, and clears
// the two extra-model creators in a counted loop the compiler did not
// unroll. The combat and quick-time combat creators are left alone.
//
// NEAR MISS, 11 of 19 words. Retail keeps the zero in r10 and the
// loop's byte offset in r9, copying the zero into r9 as its third
// instruction, before the argument stores; ours copies it into r4,
// the argument register the first store frees, and the copy cannot
// rise above that store. Seven spellings leave it there: the index
// declared first, zeroed at its declaration, register, unsigned, a
// named null for the creators, and the shapes between. What is
// left to find is what kept retail's index alive before the stores.
// Layout from the DWARF (tools/dwarf_types.py --type zNPCType).

class zNPCBase;
class CreatorI;

namespace World {
class EntityHandleBase;
}

enum eNPCType { eNPCType_ = 0x7FFFFFFF };

namespace Sext {

class AnimationSet {
public:
    enum eAnimSetType { eAnimSetType_ = 0x7FFFFFFF };
};

}  // namespace Sext

typedef zNPCBase* (*zNPCAllocateFunction)(World::EntityHandleBase* handle);

class zNPCType {
public:
    void Setup(eNPCType type, Sext::AnimationSet::eAnimSetType animSet,
               const char* name, zNPCAllocateFunction allocate,
               unsigned int tid);

    eNPCType npcTypeEnum;
    Sext::AnimationSet::eAnimSetType animSetType;
    const char* typeName;
    zNPCAllocateFunction allocateNPCFunction;
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
                     const char* name, zNPCAllocateFunction allocate,
                     unsigned int tid) {
    int i;

    npcTypeEnum = type;
    animSetType = animSet;
    typeName = name;
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
