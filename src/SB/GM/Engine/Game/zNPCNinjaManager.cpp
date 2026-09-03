// zNPCNinjaManager.cpp -- seven functions, read from the image with
// tools/disasm.py. The manager that hands out four "ninja attack" slots
// to NPCs and queues the ones that miss out. Each slot keeps the current
// attacker, the time left on its attack and a delay before the slot is
// released; beside them sits a 32-entry intrusive list of the NPCs
// waiting for that slot.
//
// Layouts from the DWARF (tools/dwarf_types.py): zNPCNinjaManager 0x8A8
// on a 0x18 zModule, with waitingQueue[4] at +0x18 (0x218 each),
// curAttacker[4] at +0x878, remainingAttackTime[4] at +0x888 and
// delayReleaseTime[4] at +0x898; NinjaSlot 0x8 (a zNPCBase* and a word);
// fixed_stack_list 0x218 with _size at 0, head at +4, tail at +0xC,
// stack at +0x14 and buffer[32] at +0x18; node_type 0x10, an
// empty_node_type (prev, next) with the value at +8.
//
// zModule's five words are followed by its vtable pointer at +0x14 --
// the compiler puts the pointer AFTER the members of a class with no
// polymorphic base, which is what makes 5 words come to 0x18. The
// virtual is declared and never defined so no vtable lands here;
// retail's __vt__16zNPCNinjaManager lives in WAD02's data and its
// constructor at 0x80103950 is in another unit.
//
// The template's own members were not guessed at. The image carries a
// SECOND instantiation of fixed_stack_list, over zNPCBase*, whose
// reset() (0x800F5780) and push_back() (0x800F5730) are emitted
// out-of-line -- so both bodies here are read off retail's own code for
// the same template, and the NinjaSlot instantiation's ModulePrepUse is
// that reset() inlined store for store.
//
// MEASURED with tools/unitcmp.py: 4 of the 7 functions the object defines
// are byte-identical -- _RequestAttack (54 words), push_back (19),
// _ReleaseAttack (15) and free (4), 368 of the unit's 964 bytes. The
// object defines exactly the 7 functions the split holds: nothing EXTRA
// and nothing missing.
//
// Four things the bytes said outright, each worth reusing:
//
// The two flag bits ModulePrepUse clears are one lwz / rlwinm 0xF3FFFFFF
// / stw, so the two bitfield assignments fold into a single mask.
//
// Timestep's two "give the slot back" blocks store in OPPOSITE orders --
// delayReleaseTime then curAttacker in the first, curAttacker then
// delayReleaseTime in the second -- so each is written in the order the
// stores come out.
//
// MWCC INLINES A SYNTHESISED operator= ONLY AT OFFSET ZERO. push_back's
// last statement copies a T into tail.prev->value, which is +8 of the
// node; written `tail.prev->value = value;` mwcc emits
// __as__Q216zNPCNinjaManager9NinjaSlotFRC... as an EXTRA function retail
// does not have and TAIL-CALLS it (16 words against 19). Written through
// a reference or pointer local -- `T& slot = tail.prev->value; slot =
// value;` -- the copy inlines and the symbol is never emitted. Probed
// eight ways in a scratch file: a destination at +0 of a parameter
// inlines, one at +8 does not, and a cast, a one-expression helper taking
// a pointer, and a helper taking a reference all still call. `struct`
// instead of `class`, and `#pragma always_inline on` around the template,
// do not move it (the pragma inlines push_back itself instead).
//
// free() is CALLED in Timestep and written OUT in _RequestAttack, because
// that is what retail does and -inline auto here calls it at both sites:
// left as a call, _RequestAttack grows a 48-byte frame, f31 and four
// callee-saved registers, 69 words against 54.
//
// NEAR MISS -- Timestep, 82 of retail's 83 words differ, ours 85 words
// (340 bytes against 332). The STRUCTURE is identical instruction for
// instruction; what differs is register allocation plus TWO extra words,
// and the two extra words are one thing: retail keeps the constant zero
// in ONE callee-saved register (r29) for all three of its uses --
// curAttacker[i] = 0 in each of the two release blocks and
// requestedThisFrame = 0 in the second walk -- and ours materialises
// THREE (r26, r27, r28), each `mr rX,r22` from the loop counter's own
// initial zero. Retail spells four independent `li rN,0` (i, i*536, i*4,
// the constant); ours spells two `li` and three `mr`. That is 11
// callee-saved registers against retail's 9, hence _savegpr_21 against
// _savegpr_23 and a 96-byte frame against 80.
//
// Tried and rejected for Timestep, each compiled and read back:
//   * `Queue& list = waitingQueue[i];` and `Queue* list =
//     &waitingQueue[i];` inside the size test. Both make -inline auto
//     INLINE free() -- the call needs an extra addi for the list address
//     where retail's already sits in r26 -- and the whole function
//     collapses to a 63-word leaf with a bdnz loop and no call at all.
//   * a shared `zNPCBase* none = 0;` local for the two curAttacker
//     stores: no word moves, the three copies stay.
//   * two reads of n->next / n->prev instead of the `next` and `prev`
//     locals: each costs a re-load, since the store to the neighbour may
//     alias (86 words). The locals are kept.
//
// NEAR MISS -- ModulePrepUse, 24 of 36 words differ, and the LENGTH and
// the whole instruction sequence are retail's: every difference is a
// register number. Retail spells the loop-end sentinel off the buffer
// pointer (addi r6,r8,496) and puts &head in r0; ours puts &head and the
// sentinel in named registers. Tried: the sentinel as
// `&waitingQueue[i].buffer[31]` (gives addi r6,r7,544 off the list base
// instead, one step further from retail); an index loop `for (k = 0; k <
// 31; k++) buffer[k].next = &buffer[k+1];` (NOT strength-reduced -- 46
// words, a stack frame and a slwi in the body); declaring the sentinel
// before the iterator and after it (swaps two register numbers, nothing
// else); calling a fixed_stack_list::reset() member (not inlined -- a
// bl, a 48-byte frame, f31 and five saved registers, and an EXTRA
// reset__51fixed_stack_list<...>Fv symbol retail does not carry);
// `#pragma always_inline on` around that member in the class body and
// around an out-of-class definition of it (neither inlines it).
//
// NEAR MISS -- _AddToWaitingQueue, 7 of 30 words differ, same length.
// Two differences: retail loads head.next BEFORE the addi that turns the
// base into &list (`lwz r5,32(r3)` then `addi r3,r3,24`), ours after it;
// and the iterator and the requestor temp are in each other's registers
// (retail n=r5, temp=r6; ours n=r6, temp=r5). Tried: the iterator
// declared before the reference (the base then stays in r5 and &list
// goes to r3, no better); an explicit `end` local for the tail sentinel
// (puts the iterator in r5 as retail has it but moves the sentinel out
// of r0 into r6, same count); no reference at all, `waitingQueue[index]`
// throughout (17 of 30 -- the tail is then addressed off the base at +36
// and _size is read with a lwzu that doubles as the address for
// push_back).

typedef unsigned long size_t_;

class zNPCBase;

enum enModulePriority {
    MODULE_PRIORITY_EARLY = 0,
    MODULE_PRIORITY_NORMAL = 1,
    MODULE_PRIORITY_LATE = 2
};

// Five words then the vtable pointer at +0x14, which is where mwcc puts
// it for a class with no polymorphic base. 0x18 bytes.
class zModule {
public:
    unsigned int flg_skipUpdates : 1;
    unsigned int flg_skipRenders : 1;
    unsigned int flg_useBucketRender : 1;
    unsigned int flg_useLayerRender : 1;
    unsigned int flg_updateWhenPaused : 1;
    unsigned int flg_updateInCinematic : 1;
    unsigned int flg_notUsed : 26;

    int tag_module;
    char* nam_module;
    enModulePriority updatePriority;
    enModulePriority renderPriority;

    virtual void ModuleSetup();
};

template <class T, int N>
class fixed_stack_list {
public:
    class node_type;

    class empty_node_type {
    public:
        node_type* prev;
        node_type* next;
    };

    class node_type : public empty_node_type {
    public:
        T value;
    };

    size_t_ _size;
    empty_node_type head;
    empty_node_type tail;
    node_type* stack;
    node_type buffer[N];

    void free(node_type* node) {
        node->next = stack;
        stack = node;
    }

    void push_back(const T& value) {
        node_type* node = stack;

        stack = node->next;

        node->prev = tail.prev;
        node->next = (node_type*)&tail;
        tail.prev = node;
        node->prev->next = node;

        _size++;

        // A reference, not `tail.prev->value = value` outright: mwcc
        // inlines the synthesised operator= only where the destination is
        // at OFFSET ZERO of the pointer it addresses, and tail switches to
        // a tail call into __as__ at +8. Measured on both spellings.
        T& slot = tail.prev->value;
        slot = value;
    }
};


class zNPCNinjaManager : public zModule {
public:
    struct NinjaSlot {
        zNPCBase* requestor;
        unsigned int requestedThisFrame;
    };

    typedef fixed_stack_list<NinjaSlot, 32> Queue;

    void ModulePrepUse();
    void Timestep(float dt);
    bool _RequestAttack(const zNPCBase* npc, int index, float attackTime);
    void _AddToWaitingQueue(const zNPCBase* npc, int index);
    void _ReleaseAttack(const zNPCBase* npc, int index, float delay);

    Queue waitingQueue[4];
    zNPCBase* curAttacker[4];
    float remainingAttackTime[4];
    float delayReleaseTime[4];
};

// The queue set-up is written out rather than called: it is retail's own
// fixed_stack_list::reset(), read off the OTHER instantiation of the same
// template (0x800F5780, over zNPCBase*) store for store, and -inline auto
// declines a body of six stores plus a loop -- a call here costs a stack
// frame and eight saved registers that retail's leaf prologue does not
// have. `#pragma always_inline on` does not move it, in the class body or
// around an out-of-class definition; both were compiled and read back.
void zNPCNinjaManager::ModulePrepUse() {
    int i;

    flg_updateWhenPaused = 0;
    flg_updateInCinematic = 0;

    for (i = 0; i < 4; i++) {
        Queue::node_type* n;
        Queue::node_type* end;

        waitingQueue[i]._size = 0;
        waitingQueue[i].head.next = (Queue::node_type*)&waitingQueue[i].tail;
        waitingQueue[i].head.prev = 0;
        waitingQueue[i].tail.prev = (Queue::node_type*)&waitingQueue[i].head;
        waitingQueue[i].tail.next = 0;
        waitingQueue[i].stack = &waitingQueue[i].buffer[0];

        n = &waitingQueue[i].buffer[0];
        end = n + 31;

        while (n != end) {
            n->next = n + 1;
            n++;
        }

        waitingQueue[i].buffer[31].next = 0;

        curAttacker[i] = 0;
        remainingAttackTime[i] = 0.0f;
        delayReleaseTime[i] = 0.0f;
    }
}

void zNPCNinjaManager::Timestep(float dt) {
    int i;

    for (i = 0; i < 4; i++) {
        if (curAttacker[i] != 0) {
            remainingAttackTime[i] -= dt;

            if (remainingAttackTime[i] < 0.0f) {
                delayReleaseTime[i] = 0.0f;
                curAttacker[i] = 0;
            }

            if (delayReleaseTime[i] > 0.0f) {
                delayReleaseTime[i] -= dt;

                if (delayReleaseTime[i] < 0.0f) {
                    curAttacker[i] = 0;
                    delayReleaseTime[i] = 0.0f;
                }
            }
        }

        if (waitingQueue[i]._size != 0) {
            Queue::node_type* n = waitingQueue[i].head.next;
            Queue::node_type* m;

            while (n != (Queue::node_type*)&waitingQueue[i].tail) {
                if (n->value.requestedThisFrame == 0) {
                    Queue::node_type* next = n->next;
                    Queue::node_type* prev = n->prev;

                    next->prev = prev;
                    prev->next = next;

                    waitingQueue[i].free(n);
                    waitingQueue[i]._size--;

                    n = next;
                } else {
                    n = n->next;
                }
            }

            m = waitingQueue[i].head.next;

            while (m != (Queue::node_type*)&waitingQueue[i].tail) {
                m->value.requestedThisFrame = 0;
                m = m->next;
            }
        }
    }
}

bool zNPCNinjaManager::_RequestAttack(const zNPCBase* npc, int index, float attackTime) {
    if (curAttacker[index] != 0) {
        if (curAttacker[index] == npc && delayReleaseTime[index] <= 0.0f) {
            return true;
        }

        _AddToWaitingQueue(npc, index);
        return false;
    }

    if (waitingQueue[index]._size == 0) {
        curAttacker[index] = (zNPCBase*)npc;
        remainingAttackTime[index] = attackTime;
        return true;
    }

    {
        Queue::node_type* node = waitingQueue[index].head.next;

        if (node->value.requestor == npc) {
            // `next` is a LOCAL: written as two reads of node->next the
            // second cannot be folded past the store to head.next -- they
            // may alias -- and the word costs a re-load.
            Queue::node_type* next = node->next;

            waitingQueue[index].head.next = next;
            next->prev = (Queue::node_type*)&waitingQueue[index].head;

            // free()'s two stores written out: retail INLINES them here and
            // CALLS free() in Timestep, and -inline auto calls it in both.
            // Left as a call this function grows a 48-byte frame, f31 and
            // four callee-saved registers retail does not have -- 69 words
            // against 54.
            node->next = waitingQueue[index].stack;
            waitingQueue[index].stack = node;

            waitingQueue[index]._size--;

            curAttacker[index] = (zNPCBase*)npc;
            remainingAttackTime[index] = attackTime;
            return true;
        }
    }

    _AddToWaitingQueue(npc, index);
    return false;
}

void zNPCNinjaManager::_AddToWaitingQueue(const zNPCBase* npc, int index) {
    Queue& list = waitingQueue[index];
    Queue::node_type* n = list.head.next;

    while (n != (Queue::node_type*)&list.tail) {
        if (n->value.requestor == npc) {
            n->value.requestedThisFrame = 1;
            return;
        }

        n = n->next;
    }

    if (list._size != 32) {
        NinjaSlot slot;

        slot.requestor = (zNPCBase*)npc;
        slot.requestedThisFrame = 1;

        list.push_back(slot);
    }
}

void zNPCNinjaManager::_ReleaseAttack(const zNPCBase* npc, int index, float delay) {
    if (npc != curAttacker[index]) {
        return;
    }

    if (delay <= 0.0f) {
        curAttacker[index] = 0;
    } else {
        delayReleaseTime[index] = delay;
    }
}
