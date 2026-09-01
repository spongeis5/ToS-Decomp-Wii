// RTTID_TableInit -- sort the type hashes, then link each type under its
// parent, then attach the creators.
//
// Read from the image with tools/disasm.py. Three things it says outright:
//
//   mulli r0,r0,36            an entry of the data table is 36 bytes
//   addi  r3,r3,-36           and the first loop counts DOWN
//   addi  r4,r4,-4            with a second induction variable on the
//                             parent table, which is 4 bytes an entry
//   addi  r3,r3,8             the create table's entry is 8
//
//   lwz r0,20(r8) ; stw r0,24(r7) ; stw r7,20(r8)
//                             push onto a list: the node takes the
//                             parent's current head as its own next, and
//                             becomes the new head
//
//   addic. r0,r31,-1 ... beq  the down-count runs maxType-1 times and is
//                             skipped entirely when that is zero

namespace Util {

struct RTTIDData {
    unsigned char _pad0[0x14];
    RTTIDData* child;
    RTTIDData* next;
    unsigned int order;
    void* create;
};

struct RTTIDCreate {
    unsigned int type;
    void* create;
};

extern unsigned int g_rttidHash[];
extern unsigned int g_rttidMaxType;
extern unsigned int g_rttidParentTable[];
extern RTTIDData g_rttidDataTable[];
extern RTTIDCreate g_rttidCreateTable[];
extern unsigned int g_rttidCreateCount;

void QuickSortUint(void* base, int count, int width, int flag);

void RTTID_TableInit() {
    // Read ONCE into a variable: retail keeps it in r31 across the call
    // and reuses it for the loop bound, where reading the global twice
    // reloads it. And the counter is UNSIGNED -- `i > 0` compiles to the
    // `beq` retail has, where a signed one gives `ble`.
    unsigned int maxType = g_rttidMaxType;

    QuickSortUint(g_rttidHash, maxType, 8, 0);

    for (unsigned int i = maxType - 1; i > 0; i--) {
        unsigned int parent = g_rttidParentTable[i];

        g_rttidDataTable[i].next = g_rttidDataTable[parent].child;
        g_rttidDataTable[parent].child = &g_rttidDataTable[i];
    }

    for (unsigned int i = 0; i < g_rttidCreateCount; i++) {
        unsigned int type = g_rttidCreateTable[i].type;

        g_rttidDataTable[type].order = i + 1;
        g_rttidDataTable[type].create = g_rttidCreateTable[i].create;
    }
}

}  // namespace Util
