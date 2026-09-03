// zBTNodeSequence.cpp -- five functions over two classes, 468 bytes, read
// from the image with tools/disasm.py. A sequence node runs its children
// one after another: the node's CreateTask hands out a zBTSequenceTask,
// its CreateIterator hands out the child iterator the node's type asks
// for, and the task's Execute walks that iterator, opening one child at a
// time on the interpreter.
//
//   __ct__15zBTSequenceTaskFv   calls __ct__7zBTTaskFv, stores
//                               __vt__15zBTSequenceTask at 0 and zeroes
//                               24, 28 and 32
//   Execute                     Begin() when nothing is open, then
//                               IsDone()/Current()/Next() on the iterator
//                               around zBTInterpreter::Open(zBTNode*,
//                               zBTTask*)
//   Setup                       one BYTE load at +8 of the asset, stored
//                               as a word at +0x10
//   CreateTask                  zBTFactory::factory.AllocMem(36, 14), then
//                               the constructor
//   CreateIterator              one of the two zBTFactory::Create<>
//                               instantiations, then SetOwner(this)
//
// Layouts from the DWARF (tools/dwarf_types.py): zBTTask 0x18 with the
// interpreter at +4, the node at +8 and the owner at +0xC on a 4-byte
// zSchedulerTask; zBTSequenceTask 0x24 with the iterator at +0x18, the
// current child task at +0x1C and the child state at +0x20; zBTNode 0x10
// with parent, childCount and children at 0, 4 and 8 -- the fourth word is
// the vtable pointer, which the DWARF never describes, so the base is
// declared here as those twelve bytes with the virtual in the derived
// class; zBTNodeSequence 0x14 with sequenceType, an unsigned int, at
// +0x10; zBTNode::ChildIteratorBase 4 bytes, ChildIterator 0xC and
// RandomChildIterator 0x18. Sext::BTNodeBase is 0x8 (id, order) and the
// DWARF has no type for the sequence asset itself; the unsigned char at
// +8 is the BYTE LOAD in Setup, and SelectorNode and ParallelNode carry
// their own type byte the same way.
//
// The vtables are recovered fact, read out of .data:
// __vt__Q27zBTNode13ChildIterator at 806BA498 and
// __vt__Q27zBTNode19RandomChildIterator at 806BA458 are both six slots --
// SetOwner, Begin, Next, Current, IsDone, Cleanup -- so the +8, +12, +16,
// +20 and +24 dispatches in this unit are SetOwner, Begin, Next, Current
// and IsDone. __vt__15zBTSequenceTask at 806BA198 is zBTTask's six,
// Execute first. __vt__15zBTNodeSequence at 806BA178 is five -- Setup,
// CreateTask, DestroyTask, CreateIterator, DestroyIterator -- so all three
// node functions here are virtual in retail; nothing in this unit
// dispatches through them, and the vtable lives in WAD01's data, so the
// class declares one UNDEFINED virtual ahead of them and the three
// themselves plainly. The task does the same with Cleanup, which another
// unit defines: an undefined first virtual keeps the vtable's home out of
// this object, and re-declaring an override adds no slot.
//
// Three shapes the bytes fixed.
//
// Execute's guard is ONE early return with an or-chain, not a nest.
// Retail tests IsDone and branches FORWARD to a shared `return
// childState`, then tests the two states and branches BACK past it into
// the body -- which is what `if (A || (B && C)) return childState;`
// emits, the body falling out below. An `if (!A && (B || C)) { body }`
// with the return after it puts the body first instead.
//
// The store of the opened task follows its own compare: retail has
// `cmpwi r3,0 ; stw r3,28(r31) ; bne`, which is the assignment and then
// `if (currChildTask == 0) return eTaskState_4;` with the failure block
// as the fall-through.
//
// CreateTask is zBTNodeReference::CreateTask without its trailing store,
// and takes the same conditional: `!mem ? 0 : new (mem) zBTSequenceTask`
// puts the null block first (`bne ; li r3,0 ; b`) with the placement
// new's own test on the same compare after it, and the constructor's
// return value is the function's.

enum eTaskState {
    eTaskState_0,
    eTaskState_1,
    eTaskState_2,
    eTaskState_3,
    eTaskState_4,
    eTaskState_5
};

namespace Sext {

class BTNodeBase {
public:
    int id;
    int order;
};

// No DWARF type; the byte at +8 is Setup's `lbz r0,8(r4)`.
class SequenceNode : public BTNodeBase {
public:
    unsigned char sequenceType;
};

}  // namespace Sext

namespace Memory {

enum eFactoryMemType {
    eFactoryMemType_14 = 14
};

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
};

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

class zBTTask;
class zBTClient;
class DelegateP2;

class zBTNode {
public:
    class ChildIteratorBase {
    public:
        virtual void SetOwner(zBTNode* node);
        virtual void Begin();
        virtual void Next();
        virtual zBTNode* Current() const;
        virtual bool IsDone() const;
        virtual void Cleanup();
    };

    class ChildIterator : public ChildIteratorBase {
    public:
        /* +0x4 */ zBTNode* owner;
        /* +0x8 */ int current;
    };

    class RandomChildIterator : public ChildIteratorBase {
    public:
        /* +0x4  */ zBTNode* owner;
        /* +0x8  */ int currIdx;
        /* +0xC  */ int current;
        /* +0x10 */ int validCount;
        /* +0x14 */ int* indices;
    };

    /* +0x0 */ zBTNode* parent;
    /* +0x4 */ int childCount;
    /* +0x8 */ zBTNode** children;
};

class zBTInterpreter {
public:
    zBTTask* Open(zBTNode* node, zBTTask* task);
};

class zBTTask {
public:
    zBTTask();

    // Retail's slot order, read off __vt__15zBTSequenceTask.
    virtual eTaskState Execute(float dt) = 0;
    virtual void Cleanup();
    virtual void SetObserver();
    virtual void Setup();
    virtual void SelfDone(eTaskState state);
    virtual void ChildDone(eTaskState state);

    /* +0x4  */ zBTInterpreter* interpreter;
    /* +0x8  */ zBTNode* node;
    /* +0xC  */ zBTTask* owner;
    /* +0x10 */ zBTClient* btClient;
    /* +0x14 */ DelegateP2* observer;
};

class zBTFactory {
public:
    static Memory::Factory factory;

    template <class T>
    static T* Create();
};

class zBTSequenceTask : public zBTTask {
public:
    zBTSequenceTask();

    // Declared first and left undefined so the vtable's home stays where
    // retail has it, in WAD01's data.
    virtual void Cleanup();

    virtual eTaskState Execute(float dt);

    /* +0x18 */ zBTNode::ChildIteratorBase* iterator;
    /* +0x1C */ zBTTask* currChildTask;
    /* +0x20 */ eTaskState childState;
};

zBTSequenceTask::zBTSequenceTask() : iterator(0), currChildTask(0),
                                     childState(eTaskState_0) {
}

eTaskState zBTSequenceTask::Execute(float dt) {
    if (currChildTask == 0) {
        iterator->Begin();
    }

    if (iterator->IsDone() ||
        (childState != eTaskState_3 && childState != eTaskState_0)) {
        return childState;
    }

    currChildTask = interpreter->Open(iterator->Current(), this);

    if (currChildTask == 0) {
        return eTaskState_4;
    }

    iterator->Next();

    return eTaskState_2;
}

class zBTNodeSequence : public zBTNode {
public:
    virtual void __vtable_anchor();

    void Setup(Sext::BTNodeBase* nodeAsset);
    zBTSequenceTask* CreateTask();
    zBTNode::ChildIteratorBase* CreateIterator();

    /* +0x10 */ unsigned int sequenceType;
};

void zBTNodeSequence::Setup(Sext::BTNodeBase* nodeAsset) {
    sequenceType = ((Sext::SequenceNode*)nodeAsset)->sequenceType;
}

zBTSequenceTask* zBTNodeSequence::CreateTask() {
    void* mem = zBTFactory::factory.AllocMem(sizeof(zBTSequenceTask),
                                             Memory::eFactoryMemType_14);

    return !mem ? 0 : new (mem) zBTSequenceTask;
}

zBTNode::ChildIteratorBase* zBTNodeSequence::CreateIterator() {
    zBTNode::ChildIteratorBase* iterator;

    if (sequenceType == 1) {
        iterator = zBTFactory::Create<zBTNode::RandomChildIterator>();
    } else {
        iterator = zBTFactory::Create<zBTNode::ChildIterator>();
    }

    iterator->SetOwner(this);

    return iterator;
}
