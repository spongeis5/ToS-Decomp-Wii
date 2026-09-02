// C:/branches/SB09/main/GM/Engine/Game/zBTNodeReference.cpp
//
// Seven functions over two classes, 464 bytes, no data of its own.
//
// WHAT THE IMAGE SAYS, before any guessing. tools/dwarf_lines.py gives the
// original's line for every instruction and tools/dwarf_locals.py the
// register each `this` lives in; the branch and address targets are
// resolved from the symbol table:
//
//   __ct__16zBTReferenceTaskFv  calls __ct__7zBTTaskFv, stores
//                               __vt__16zBTReferenceTask at 0, zeroes
//                               24, 28 and 32
//   Execute                     reads 28, and on zero calls
//                               zBTInterpreter::Open(unsigned int, zBTTask*)
//   SelfDone                    tail-calls zScheduler::Abort, or makes a
//                               virtual call through the object at 12
//   ChildDone                   tail-calls zScheduler::Resume
//   __ct__16zBTNodeReferenceFv  stores __vt__16zBTNodeReference at 12 and
//                               zeroes 16 -- so the base is 12 bytes and
//                               NOT polymorphic, where zBTTask is
//   Setup                       one load and one store
//   CreateTask                  zBTFactory::factory.AllocMem(36, 14), then
//                               the constructor, then copies 16 into 24
//
// THE BIT MASK IS A SWITCH. SelfDone and ChildDone both contain the same
// seven instructions -- `addi r6,r4,-1`, `cmpli r6,4`, `1 << r6`,
// `andi. 0x13` -- which is how mwcc lowers a switch with sparse cases:
// 0x13 is 0b10011, so the cases are state 1, 2 and 5. Both copies are
// attributed to a SINGLE source line (43 and 49), and an inlined call
// takes the CALL SITE's line, so it is one inline function used twice
// rather than the switch written out twice.

// FOUR OF SEVEN MATCH: both constructors, Execute (96 bytes) and
// Setup, which is 208 of the unit's 464 bytes. What is left, and what
// is already ruled out:
//
// THE POLARITY IS SETTLED and it is not the obvious one. Retail sets a
// flag to 1, clears it when the state IS one of the three, and returns
// when the flag is zero -- so the body runs for states OUTSIDE
// {1, 2, 5}, and the shape is an early return rather than a wrapping
// `if`. Reading it the other way round is what the first draft did.
//
// WHAT IS LEFT IN ChildDone AND SelfDone is one thing: mwcc lowers this
// switch to a RANGE TEST plus a compare --
//     addi r0,r4,-1 ; cmpli r0,1 ; ble ; cmpwi r4,5 ; bne
// where retail uses a BIT MASK over a five-wide window --
//     addi r6,r4,-1 ; cmpli r6,4 ; 1 << r6 ; andi. 0x13
// The cases are the same three either way (0x13 is bits 0, 1 and 4),
// so it is the compiler's choice of lowering and not the case set.
//
// Already tried, do not redo -- six spellings of the switch: no
// `default`, `default` first, the cases in the opposite order, a bool
// assigned in the switch and returned after, switching on `(int)state`
// rather than the enum, and the call sites as `if (...)` instead of an
// early return. ALL SIX give the same range-test lowering and the same
// 16 of 15 words. Adding cases 3 and 4 to widen the range to a full
// 1..5 -- which is what `cmpli r6,4` looks like -- stops mwcc inlining
// the helper at all and goes to 19 of 16.
//
// CreateTask is 12 of 20 and 80 bytes against retail's 92. The
// allocation is right -- `zBTFactory::factory.AllocMem(36, 14)` through
// an in-class operator new, which is what puts the size and the type
// inline -- and what is missing is the null branch retail emits around
// the constructor call. Retail writes through the pointer without
// checking it afterwards, so on allocation failure it stores to 0x18;
// that is in the image, not a misreading.
enum eTaskState {
    eTaskState_0,
    eTaskState_1,
    eTaskState_2,
    eTaskState_3,
    eTaskState_4,
    eTaskState_5
};

namespace Sext {
class BTNodeBase;
}

namespace Memory {

enum eFactoryMemType {
    eFactoryMemType_14 = 14
};

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
};

}  // namespace Memory

class zBTFactory {
public:
    static Memory::Factory factory;
};

class zSchedulerTask;

class zScheduler {
public:
    void Abort(zSchedulerTask* task);
    void Resume(zSchedulerTask* task);
};

class zBTTask {
public:
    zBTTask();

    virtual void __vtable_anchor();
    virtual eTaskState Execute(float dt);
    virtual void SelfDone(eTaskState state);
    virtual void ChildDone(eTaskState state);

    /* +0x4  */ zScheduler* scheduler;
    /* +0x8  */ unsigned char _pad8[0x4];
    /* +0xC  */ zBTTask* parent;
    /* +0x10 */ unsigned char _pad10[0x8];
};

class zBTInterpreter {
public:
    unsigned int Open(unsigned int id, zBTTask* task);
};

class zBTReferenceTask : public zBTTask {
public:
    zBTReferenceTask();

    void* operator new(unsigned long size) {
        return zBTFactory::factory.AllocMem(size,
                                            Memory::eFactoryMemType_14);
    }



    virtual eTaskState Execute(float dt);
    virtual void SelfDone(eTaskState state);
    virtual void ChildDone(eTaskState state);

    /* +0x18 */ unsigned int nodeID;
    /* +0x1C */ unsigned int openTask;
    /* +0x20 */ eTaskState childState;
};

inline static bool IsDoneState(eTaskState state) {
    switch (state) {
    case eTaskState_1:
    case eTaskState_2:
    case eTaskState_5:
        return true;
    default:
        return false;
    }
}

zBTReferenceTask::zBTReferenceTask() : nodeID(0), openTask(0),
                                       childState(eTaskState_0) {
}

eTaskState zBTReferenceTask::Execute(float dt) {
    if (openTask == 0) {
        openTask = ((zBTInterpreter*)scheduler)->Open(nodeID, this);

        return openTask != 0 ? eTaskState_2 : eTaskState_4;
    }

    return childState;
}

void zBTReferenceTask::SelfDone(eTaskState state) {
    if (state == eTaskState_5) {
        scheduler->Abort((zSchedulerTask*)openTask);
        return;
    }

    if (parent == 0) {
        return;
    }

    if (IsDoneState(state)) {
        return;
    }

    parent->ChildDone(state);
}

void zBTReferenceTask::ChildDone(eTaskState state) {
    if (IsDoneState(state)) {
        return;
    }

    childState = state;
    scheduler->Resume((zSchedulerTask*)this);
}

class zBTNodeBase {
public:
    unsigned char _pad0[0xC];
};

class zBTNodeReference : public zBTNodeBase {
public:
    zBTNodeReference();

    virtual void __vtable_anchor();
    void Setup(Sext::BTNodeBase* node);
    zBTReferenceTask* CreateTask();

    /* +0x10 */ unsigned int nodeID;
};

zBTNodeReference::zBTNodeReference() : nodeID(0) {
}

void zBTNodeReference::Setup(Sext::BTNodeBase* node) {
    nodeID = *(unsigned int*)((char*)node + 8);
}

zBTReferenceTask* zBTNodeReference::CreateTask() {
    zBTReferenceTask* task = new zBTReferenceTask;

    task->nodeID = nodeID;

    return task;
}
