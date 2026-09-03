// zBTNodeCondition.cpp -- eight functions, read from the image with
// tools/disasm.py. A condition node of a behaviour tree, and the tasks
// it makes. The node's Setup keeps the asset and its negate flag;
// CreateTask takes 40 bytes from the behaviour-tree factory (memory type
// 14) and places either an instant or a monitoring condition task on it,
// chosen by the asset's condition type, then hands the task the asset and
// the flag. The task's constructor runs the base one, installs the
// vtable and clears the condition and its builder. Setup finds the
// builder on the interpreter's client, asks it for a condition built from
// the asset's two condition words, gives that condition the second word
// again, and then asks itself for its observer. Cleanup gives the
// condition back to the builder, drops it, returns the observer to the
// factory and drops that. SelfDone tells its owner it is done, but only
// for the states outside a three-value set. The two Execute functions
// evaluate the condition, invert the answer when the node negates, and
// return one state for true and another for false -- 3 and 4 for the
// instant task, 1 and 4 for the monitoring one.
//
// Layouts from the DWARF (tools/dwarf_types.py): zBTTask 0x18 on a
// four-byte zSchedulerTask, with the interpreter at +4, the owner at
// +0xC and the observer at +0x14; zBTConditionTask 0x28 adding the asset
// at +0x18, the condition at +0x1C, the builder at +0x20 and the flag at
// +0x24; the two derived tasks add nothing; zBTNodeCondition 0x18 with
// the asset at +0x10 and the flag at +0x14; Sext::ConditionNode 0x18
// with the type at +8, the condition's two words at +0xC and +0x10 and
// the negate byte at +0x14; zBTInterpreter's client at +0x3C.
//
// The vtables are the image's, read slot by slot at 806B9E08, 806B9E28,
// 806B9E48 and 806B9E68: two leading zero words, then Execute, Cleanup,
// SetObserver, Setup, SelfDone and the shared empty. zBTConditionTask's
// Execute slot is ZERO, which is what an abstract class's pure virtual
// leaves there, and the two derived tables fill it. The condition and
// its builder carry their own vtable AFTER their data -- the builder's
// at +4 past its client, the condition's at +8 past its two -- so each
// is declared here as a plain base holding the data and a derived class
// adding the virtuals, which is the layout the bytes read.
//
// Four shapes the bytes fixed.
//
// SelfDone's three-value test is an and-chain of inequalities assigned
// to a LOCAL and then tested. mwcc lowers that to the shift-and-mask
// against 0x13 the bytes have; used directly as an if's condition the
// same chain short-circuits into three compares and three returns, 14
// words against 19.
//
// The placement new is a conditional EXPRESSION with the null case
// first, so the compiler lays the zero block before the constructor and
// jumps past it, and the choice between the two task types is a SWITCH:
// retail tests both cases before either block, where an if/else-if puts
// the second test after the first body and 45 of 53 words differ.
//
// And zSchedulerTask is declared EMPTY. The DWARF gives it four bytes
// at +0 of zBTTask, which is exactly where zBTTask's own vtable pointer
// goes; giving it a virtual of its own instead pushes every slot below
// it down by one, and the only place that shows is a self-call reading
// slot 5 where retail reads slot 4.

typedef unsigned long long uid;

class zBTClient;
class zBTCondition;
class zBTNode;
class zBTTask;
class DelegateP2;

enum eTaskState { eTaskState_ = 0x7FFFFFFF };

namespace Memory {

enum eFactoryMemType { eFactoryMemType_ = 0x7FFFFFFF };

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
    void DeallocMem(void* block);
};

}  // namespace Memory

class zBTFactory {
public:
    static Memory::Factory factory;
};

inline void* operator new(unsigned long, void* p) { return p; }

namespace Sext {

class BTNodeBase {
public:
    unsigned char _pad0[0x8];
};

class ConditionNode : public BTNodeBase {
public:
    unsigned int conditionType;
    unsigned int conditionA;
    unsigned int conditionB;
    bool Negate;
    unsigned char _pad0[0x3];
};

}  // namespace Sext

// The builder keeps its client at +0 and its vtable at +4, so the data
// is a base and the virtuals are the derived class.
class zBTConditionBuilderData {
public:
    zBTClient* client;
};

class zBTConditionBuilder : public zBTConditionBuilderData {
public:
    virtual zBTCondition* CreateCondition(unsigned int a, unsigned int b);
    virtual void DestroyCondition(zBTCondition* condition);
};

// The condition keeps two words and then its vtable at +8.
class zBTConditionData {
public:
    void* conditionAsset;
    zBTClient* btClient;
};

class zBTCondition : public zBTConditionData {
public:
    virtual void _v0();
    virtual void _v1();
    virtual bool Evaluate();
    virtual void SetAsset(unsigned int asset);
};

class zBTClient {
public:
    unsigned char _pad0[0x94];
    zBTConditionBuilder* conditionBuilder;
};

class zBTInterpreter {
public:
    unsigned char _pad0[0x3C];
    zBTClient* client;
};

// Empty: the DWARF gives it four bytes at +0 of zBTTask, which is where
// zBTTask's own vtable pointer goes. A virtual declared here would shift
// every slot below it by one.
class zSchedulerTask {
};

class zBTTask : public zSchedulerTask {
public:
    zBTTask();

    virtual eTaskState Execute(float dt);
    virtual void Cleanup();
    virtual void SetObserver();
    virtual void Setup();
    virtual void SelfDone(eTaskState state);
    virtual void ChildDone(eTaskState state);

    zBTInterpreter* interpreter;
    zBTNode* node;
    zBTTask* owner;
    zBTClient* btClient;
    DelegateP2* observer;
};

class zBTConditionTask : public zBTTask {
public:
    zBTConditionTask();

    virtual void Cleanup();
    virtual void Setup();
    virtual void SelfDone(eTaskState state);

    Sext::ConditionNode* conditionAsset;
    zBTCondition* condition;
    zBTConditionBuilder* conditionBuilder;
    bool negate;
};

class zBTInstantConditionTask : public zBTConditionTask {
public:
    virtual eTaskState Execute(float dt);
};

class zBTMonitoringConditionTask : public zBTConditionTask {
public:
    virtual eTaskState Execute(float dt);
};

class zBTNode {
public:
    virtual void _v0();
    virtual void Setup(Sext::BTNodeBase* node);
    virtual zBTTask* CreateTask();
    virtual void DestroyTask(zBTTask* task);

    unsigned char _pad0[0xC];
};

class zBTNodeCondition : public zBTNode {
public:
    virtual void Setup(Sext::BTNodeBase* node);
    virtual zBTTask* CreateTask();

    Sext::ConditionNode* conditionAsset;
    bool negate;
};

zBTConditionTask::zBTConditionTask() {
    condition = 0;
    conditionBuilder = 0;
}

void zBTConditionTask::Setup() {
    conditionBuilder = interpreter->client->conditionBuilder;
    condition = conditionBuilder->CreateCondition(conditionAsset->conditionA,
                                                 conditionAsset->conditionB);

    condition->SetAsset(conditionAsset->conditionB);

    SetObserver();
}

void zBTConditionTask::SelfDone(eTaskState state) {
    if (owner == 0) {
        return;
    }

    bool done = state != 1 && state != 2 && state != 5;

    if (done) {
        owner->ChildDone(state);
    }
}

void zBTConditionTask::Cleanup() {
    conditionBuilder->DestroyCondition(condition);

    condition = 0;

    zBTFactory::factory.DeallocMem(observer);

    observer = 0;
}

eTaskState zBTInstantConditionTask::Execute(float dt) {
    bool value = condition->Evaluate();

    if (negate) {
        value = !value;
    }

    return value ? (eTaskState)3 : (eTaskState)4;
}

eTaskState zBTMonitoringConditionTask::Execute(float dt) {
    bool value = condition->Evaluate();

    if (negate) {
        value = !value;
    }

    return value ? (eTaskState)1 : (eTaskState)4;
}

void zBTNodeCondition::Setup(Sext::BTNodeBase* node) {
    conditionAsset = (Sext::ConditionNode*)node;
    negate = ((Sext::ConditionNode*)node)->Negate;
}

zBTTask* zBTNodeCondition::CreateTask() {
    zBTConditionTask* task = 0;

    switch (conditionAsset->conditionType) {
    case 0: {
        void* mem = zBTFactory::factory.AllocMem(sizeof(zBTInstantConditionTask),
                                                 (Memory::eFactoryMemType)14);

        task = !mem ? 0 : new (mem) zBTInstantConditionTask();
        break;
    }
    case 1: {
        void* mem = zBTFactory::factory.AllocMem(
            sizeof(zBTMonitoringConditionTask), (Memory::eFactoryMemType)14);

        task = !mem ? 0 : new (mem) zBTMonitoringConditionTask();
        break;
    }
    }

    task->conditionAsset = conditionAsset;
    task->negate = negate;

    return task;
}
