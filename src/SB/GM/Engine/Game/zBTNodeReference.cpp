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
// THE BIT MASK IS NOT A SWITCH AT ALL: it is how mwcc lowers an
// and-chain of inequalities. Six spellings of the switch all gave a
// range test plus a compare (`addi r0,r4,-1 ; cmpli r0,1 ; ble ;
// cmpwi r4,5 ; bne`, 16 of 15 words), and widening the cases stopped
// the inlining. An or-chain of the three equalities gives the mask --
// `addi r6,r4,-1 ; cmpli r6,4 ; 1 << r6 ; andi. 0x13` -- with one word
// wrong at each call site: `bnelr` where retail has `beqlr`. Retail's
// helper is true for the states that are NOT done: it sets its answer
// to 1 before the range test and clears it inside the mask, so it is
// `state != 1 && state != 2 && state != 5`, and both call sites test
// its negation. Both handlers match on that (2026-09-02).
//
// CreateTask matches once the conditional's operands are the other way
// round: `!mem ? 0 : new (mem) zBTReferenceTask` puts the null block
// first with the constructor path branched over it (`bne ; li r3,0 ;
// b`), where `mem ? new (mem) T : 0` put the constructor first. The
// placement new's own null test on the same compare follows, and both
// paths join at the store through the pointer -- so on allocation
// failure retail stores to 0x18, which is in the image, not a
// misreading. The allocation is `zBTFactory::factory.AllocMem(36, 14)`.
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

    // Retail's slot order, read off __vt__16zBTReferenceTask: Execute,
    // Cleanup, SetObserver, Setup, SelfDone, ChildDone. The parent's
    // ChildDone is reached through slot 5 (+28); with the two handlers
    // declared right after Execute the call went through +20.
    virtual eTaskState Execute(float dt) = 0;
    virtual void Cleanup();
    virtual void SetObserver();
    virtual void Setup();
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

inline void* operator new(unsigned long, void* p) { return p; }

class zBTReferenceTask : public zBTTask {
public:
    zBTReferenceTask();

    // Declared first so the vtable's home is Cleanup's unit, not this
    // one: retail keeps __vt__16zBTReferenceTask in WAD01's data. The
    // slot is the base's, so the order of the others is unchanged.
    virtual void Cleanup();




    virtual eTaskState Execute(float dt);
    virtual void SelfDone(eTaskState state);
    virtual void ChildDone(eTaskState state);

    /* +0x18 */ unsigned int nodeID;
    /* +0x1C */ unsigned int openTask;
    /* +0x20 */ eTaskState childState;
};

inline static bool IsRunningState(eTaskState state) {
    return state != eTaskState_1 && state != eTaskState_2 &&
           state != eTaskState_5;
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

    if (!IsRunningState(state)) {
        return;
    }

    parent->ChildDone(state);
}

void zBTReferenceTask::ChildDone(eTaskState state) {
    if (!IsRunningState(state)) {
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
    void* mem = zBTFactory::factory.AllocMem(sizeof(zBTReferenceTask),
                                             Memory::eFactoryMemType_14);
    zBTReferenceTask* task = !mem ? 0 : new (mem) zBTReferenceTask;

    task->nodeID = nodeID;

    return task;
}
