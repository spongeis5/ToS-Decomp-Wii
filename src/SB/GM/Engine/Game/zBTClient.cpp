// C:/branches/SB09/main/GM/Engine/Game/zBTClient.cpp
//
// Nine functions, 740 bytes, no data of its own. Read from the image with
// tools/disasm.py; the layouts come from tools/dwarf_types.py and the
// virtual slots from the retail vtable, not from guessing.
//
//   SetupInterpreter        interpreter.Setup(512, 256), a tail call
//   SetupClient             the virtual at slot 0, the back pointer, the
//                           blackboard, the virtual at slot 6, the tree
//                           set, then the observer list cleared
//   Update                  interpreter.Update(dt), a tail call
//   Reset                   scheduler, tasks, blackboard, observers
//   Exit                    scheduler and tasks
//   StartBehavior           on a non-null tree: the scheduler's limits and
//                           -1.0f, then the root node opened
//   RegisterEventObserver   a counted scan, then append
//   UnregisterEventObserver a rescanned loop that swaps the last down
//   HandleEvent             every observer called; true if any returned it
//
// THE VTABLE SAYS WHAT IS VIRTUAL, and it was read rather than assumed.
// `__vt__9zBTClient` at 0x806BA630 is 48 bytes: two leading zero words,
// then ten slots at +8 through +44 --
//
//   0 SetupInterpreter   1 SetupClient   2 Update   3 Reset   4 Exit
//   5 StartBehavior      6 <empty>       7 <null>   8 <null>  9 HandleEvent
//
// -- so seven of this unit's nine functions are virtual and the two
// observer-list functions are not. Slot 6 holds 0x800075C0, the lone `blr`
// the image names `__ct__Q24Math8Matrix33Fv`: the weak empty function every
// empty body in the game folded onto, so slot 6 is a virtual with an empty
// body whose real name the image cannot give. Slots 7 and 8 are zero, which
// is how this compiler writes a pure virtual. All four are DECLARED here
// and not defined; only their INDEX matters, and mwcc puts the Nth virtual
// at 8 + 4N, which is what makes SetupClient's `lwz r12,8(r12)` slot 0 and
// its `lwz r12,32(r12)` slot 6.
//
// THE VPTR IS AT THE END, at +0x19C. `lwz r12,412(r3)` is 0x19C, one word
// past eventObserverCount at 0x198, and the DWARF's size of 0x1A0 leaves
// room for exactly that word. NOTES.md records the same shape for
// TRCMsgBox, System::Module, zVariableBase and zPOWManager without saying
// what puts it there; this unit measured it.
//
//   A LEVER: THE VPTR LANDS WHERE THE FIRST `virtual` IS DECLARED,
//   relative to the data members. Written with the ten virtuals ahead of
//   the members -- the ordinary way to write a class -- mwcc put the vptr
//   at offset 0 and shifted every member up four bytes: all nine
//   functions came out the RIGHT LENGTH with only their displacements
//   wrong (SetupInterpreter `addi r3,r3,16` against retail's `addi
//   r3,r3,12`, 1 of 4 words; Update 1 of 2; SetupClient 10 of 36).
//   Moving the block of virtual declarations BELOW the data members, and
//   changing nothing else, took the unit from 0 of 9 to 9 of 9 in one
//   compile. So a retail displacement that is uniformly four bytes short
//   of the DWARF's layout is not a wrong member list -- it is the
//   virtuals declared on the wrong side of them.
//
// LAYOUTS (tools/dwarf_types.py): zBTClient 0x1A0 -- treeSet 0x0,
// interpreter 0xC, blackboard 0x88, actionBuilder 0x90, conditionBuilder
// 0x94, eventObservers[64] 0x98, eventObserverCount 0x198. zBTSet 0xC is
// root, goalCount, goals; zBT 0x18 is id, root, nodes, nodeCount, so
// StartBehavior's `lwz r4,8(r4)` is treeSet.root->root. zBTInterpreter
// 0x7C opens with zScheduler 0x34, hence `addi r3,r3,12` serving as both
// &interpreter and &interpreter.scheduler; schedulerMaxTask 0x34 and
// schedulerAddLimit 0x38 are StartBehavior's two loads at +0x40 and +0x44.
// zBlackboard is 0x8.
//
// The float in StartBehavior is read out of the image, not guessed:
// @207370 at 0x806893C8 is 0xBF800000, which is -1.0f.
//
// EXACT: 9 of the 9 functions the object defines are byte-identical
// (`python tools/unitcmp.py SB/GM/Engine/Game/zBTClient`).
//
// Two things the bytes settled, each one compile:
//
//   * THE OBSERVER COUNT IS RELOADED BECAUSE THE ARRAY STORE COULD ALIAS
//     IT. RegisterEventObserver reads eventObserverCount once into r7,
//     uses it for the counted loop AND for the append index, and then
//     reloads it for the increment -- because the store through
//     `eventObservers[...]` sits between, and the compiler cannot prove a
//     runtime index into a member array misses another member of the same
//     object. UnregisterEventObserver reloads it three separate times for
//     the same reason. Both fall straight out of the plain source; no
//     caching local is needed, and adding one would be wrong.
//   * THE UNREGISTER LOOP HAS NO `break`. Retail falls through to the
//     increment after handling a match and re-tests the (now smaller)
//     count, so the scan continues past the element it removed.
//
// Not a matching question but worth recording for whoever links this:
// slot 0 of the vtable is SetupInterpreter, which this file defines, so
// this translation unit IS the vtable's home and our object emits
// `__vt__9zBTClient`. Retail's copy lives in the WAD01 blob's .data,
// which this 740-byte text-only split does not cover, so the unit needs
// its data before it can be flipped to Matching -- the fifth link gate in
// NOTES.md, arriving from the other side.

extern "C" void* memset(void* dst, int c, unsigned long n);

class xBase;
class zBTNode;
class zBTTask;
class zBTClient;
class zBTActionBuilder;
class zBTConditionBuilder;
class zVariableBase;

namespace Sext {
class EventAny;
}

namespace Util {

template <class R, class P1, class P2, class P3>
class DelegateP3 {
public:
    typedef R (*Func)(void*, P1, P2, P3);

    R operator()(P1 p1, P2 p2, P3 p3) const { return func(object, p1, p2, p3); }

    void* object;
    Func func;
};

}  // namespace Util

class zBT {
public:
    unsigned long long id;
    zBTNode* root;
    zBTNode** nodes;
    int nodeCount;
};

class Goal;

class zBlackboard {
public:
    void Init(unsigned int count);
    void Reset();

    unsigned int size;
    zVariableBase** variables;
};

class zBTSet {
public:
    void Setup(unsigned long long uid, zBlackboard* blackboard);

    zBT* root;
    int goalCount;
    Goal* goals;
};

class zScheduler {
public:
    void Setup(unsigned int maxTasks, float timeLimit, unsigned int entryLimit);
    void Reset();
    void Exit();

    unsigned char active[0x10];
    unsigned char entries[0x10];
    int maxTasks;
    float timeLimit;
    unsigned int entryLimit;
    unsigned int entriesAdded;
    bool updating;
    bool setupDone;
};

class zBTInterpreter {
public:
    void Setup(int maxTask, int addLimit);
    void Update(float dt);
    void FreeAllTasks();
    void Open(zBTNode* node, zBTTask* task);

    zScheduler scheduler;
    int schedulerMaxTask;
    int schedulerAddLimit;
    zBTClient* client;
    unsigned char running[0x1C];
    unsigned char dead[0x1C];
    void* releaseObserver;
};

class zBTClient {
public:
    typedef Util::DelegateP3<bool, xBase*, unsigned int, Sext::EventAny*> EventObserver;

    void RegisterEventObserver(EventObserver* observer);
    void UnregisterEventObserver(EventObserver* observer);

    zBTSet treeSet;
    zBTInterpreter interpreter;
    zBlackboard blackboard;
    zBTActionBuilder* actionBuilder;
    zBTConditionBuilder* conditionBuilder;
    EventObserver* eventObservers[64];
    int eventObserverCount;

    virtual void SetupInterpreter();
    virtual void SetupClient(unsigned long long uid);
    virtual void Update(float dt);
    virtual void Reset();
    virtual void Exit();
    virtual void StartBehavior();
    virtual void SetupBlackboardVariables();
    virtual void Slot7() = 0;
    virtual void Slot8() = 0;
    virtual bool HandleEvent(xBase* sender, unsigned int id, Sext::EventAny* event);
};

void zBTClient::SetupInterpreter() {
    interpreter.Setup(512, 256);
}

void zBTClient::SetupClient(unsigned long long uid) {
    SetupInterpreter();

    interpreter.client = this;

    blackboard.Init(48);

    SetupBlackboardVariables();

    treeSet.Setup(uid, &blackboard);

    memset(eventObservers, 0, sizeof(eventObservers[0]));
    eventObserverCount = 0;
}

void zBTClient::Update(float dt) {
    interpreter.Update(dt);
}

void zBTClient::Reset() {
    interpreter.scheduler.Reset();
    interpreter.FreeAllTasks();

    blackboard.Reset();

    memset(eventObservers, 0, sizeof(eventObservers[0]));
    eventObserverCount = 0;
}

void zBTClient::Exit() {
    interpreter.scheduler.Exit();
    interpreter.FreeAllTasks();
}

void zBTClient::StartBehavior() {
    if (treeSet.root) {
        interpreter.scheduler.Setup(interpreter.schedulerMaxTask, -1.0f,
                                    interpreter.schedulerAddLimit);
        interpreter.Open(treeSet.root->root, 0);
    }
}

void zBTClient::RegisterEventObserver(EventObserver* observer) {
    int i;

    for (i = 0; i < eventObserverCount; i++) {
        if (eventObservers[i] == observer) {
            return;
        }
    }

    eventObservers[eventObserverCount] = observer;
    eventObserverCount++;
}

void zBTClient::UnregisterEventObserver(EventObserver* observer) {
    int i;

    for (i = 0; i < eventObserverCount; i++) {
        if (eventObservers[i] == observer) {
            if (eventObserverCount == 1) {
                eventObservers[i] = 0;
            } else {
                eventObservers[i] = eventObservers[eventObserverCount - 1];
                eventObservers[eventObserverCount - 1] = 0;
            }

            eventObserverCount--;
        }
    }
}

bool zBTClient::HandleEvent(xBase* sender, unsigned int id, Sext::EventAny* event) {
    bool handled = false;
    int i;

    for (i = 0; i < eventObserverCount; i++) {
        if ((*eventObservers[i])(sender, id, event)) {
            handled = true;
        }
    }

    return handled;
}
