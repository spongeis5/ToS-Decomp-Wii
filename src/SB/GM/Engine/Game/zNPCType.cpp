// zNPCType.cpp -- one function, read from the image with tools/disasm.py:
// zNPCType::Setup stores its five arguments in order, clears the entity
// flag and the logic, steering, perception and fx creators, and clears
// the two extra-model creators in a counted loop the compiler did not
// unroll, walking a byte offset the bytes measure from `this`.
// The combat and quick-time combat creators are left alone.
//
// NEAR MISS, 14 of 19 words, and the five left are two register
// numbers. Retail materialises the zero in r10 and copies it into r9
// for the loop's byte offset, both before the argument stores; ours
// materialises the offset first (r10) and the zero after the first
// store (r9), so the pair is swapped and the copy is a second li.
// Walking the offset in BYTES rather than by index is what brought
// this from 11 words to 14: an index loop leaves the offset in r4,
// the argument register the first store frees. Twenty-two spellings
// sit at 14 and none above it -- the offset declared first, last and
// in the for, unsigned, as a pointer, measured from the array, from
// `this` and from a copy of the base, tested with < and !=, counted
// down beside a second variable, and the zero given first as a named
// null, as an int the stores cast, and as the value the entity flag
// is compared against. An index loop and a do/while are worse, 9 and
// 7. What is left to find is what gives retail's zero the earlier
// definition.
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
    int off = 0;

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

    for (; off < 8; off += 4) {
        *(CreatorI**)((char*)extraModelCreator + off) = 0;
    }
}
