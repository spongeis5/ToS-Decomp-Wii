// zNPCSearchMapLinkCostCalculator::CalculateCost -- one function.
//
// Read from the image with tools/disasm.py. -1.0f is the refusal (the
// literal at 8068BA4C) and 1.0f the multiplier (8068BA48); every path that
// cannot cost the link returns the first.
//
// The entity is loaded ONCE into r4 at the top of the switch and is still
// there when the first branch calls CalculateWallNetCost -- that call
// never sets r4. So the same pointer is both the thing whose type is
// switched on and the zWallNet handed to the base, which is why it is
// written as one variable and cast at the call.
//
// The two GetWallNet calls happen in the opposite order to the argument
// list: the link's index is fetched first and ends up as the SECOND
// argument. That is mwcc evaluating right to left, not two statements.

// DOES NOT MATCH YET, and the reason is one multiply.
//
// Retail saves f31 -- `stfd f31,48(r1)` and `psq_st f31,56(r1)` --
// loads 1.0f into it before the switch, and multiplies each cost by
// it after the call returns. A callee-saved FPR is only needed
// because the value has to survive a call, so in the original the
// multiplier is live across one.
//
// mwcc FOLDS `x * 1.0f` away here whatever it is spelled as: as a
// literal in the return expression, and as a local `float scale =
// 1.0f` set before the switch and used after both calls. Neither
// needs a register, so neither saves f31 and the frame comes out 0x30
// where retail has 0x40 -- which shifts every word and is why the
// diff reads 70 of 70.
//
// So the multiplier is probably NOT the constant it looks like. What
// would produce this is a value the compiler cannot see through --
// read from the object, or handed in -- that happens to be 1.0f, with
// the literal pool entry belonging to something else in the unit.
// The rest of the function is settled: the two flag tests, their
// polarity, the switch values 0x62 and 0xB1, and the right-to-left
// evaluation that fetches the link's index before the node's.

class zSearchMap;
class zSearchMapNode;
class zSearchMapLink;
class zWallNet;

class zNavLinkFlags {
public:
    unsigned char _pad0[0x20];
    unsigned int flags;
};

class zNavLink {
public:
    bool IsEnabled() const;

    unsigned char _pad0[0x3C];
    zNavLinkFlags* info;
};

class zWallNetGroup {
public:
    zWallNet* GetWallNet(int index) const;
};

class zEntBase {
public:
    unsigned char _pad0[0x20];
    unsigned int type;
};

class zSearchMapNodeImpl {
public:
    int index;
};

class zSearchMapLinkImpl {
public:
    zSearchMapNodeImpl* node;
    zNavLink* navLink;
};

class zSearchMapImpl {
public:
    unsigned char _pad0[0x10];
    zEntBase* entity;
};

class zPathFinderSearchMapLinkCostCalculator {
public:
    float CalculateWallNetCost(const zWallNet* net,
                               const zSearchMapLink* link, int a, int b,
                               void* p, void* q) const;
    float CalculateWallNetGroupCost(const zWallNet* a,
                                    const zWallNet* b) const;
};

class zNPCSearchMapLinkCostCalculator
    : public zPathFinderSearchMapLinkCostCalculator {
public:
    float CalculateCost(const zSearchMap* map, const zSearchMapNode* node,
                        const zSearchMapLink* link, void* p, void* q) const;
};

float zNPCSearchMapLinkCostCalculator::CalculateCost(
    const zSearchMap* map, const zSearchMapNode* node,
    const zSearchMapLink* link, void* p, void* q) const {
    zNavLink* nav = ((const zSearchMapLinkImpl*)link)->navLink;

    if (nav) {
        if (!nav->IsEnabled()) {
            return -1.0f;
        }

        unsigned int flags = nav->info->flags;

        if (flags & 4) {
            return -1.0f;
        }

        if (flags & 8) {
            return -1.0f;
        }
    }

    zEntBase* ent = ((const zSearchMapImpl*)map)->entity;
    float scale = 1.0f;

    if (ent->type == 0x62) {
        return CalculateWallNetCost(
                   (const zWallNet*)ent, link,
                   ((const zSearchMapNodeImpl*)node)->index,
                   ((const zSearchMapLinkImpl*)link)->node->index, p, q)
               * scale;
    }

    if (ent->type == 0xB1) {
        return CalculateWallNetGroupCost(
                   ((zWallNetGroup*)ent)->GetWallNet(
                       ((const zSearchMapNodeImpl*)node)->index),
                   ((zWallNetGroup*)ent)->GetWallNet(
                       ((const zSearchMapLinkImpl*)link)->node->index))
               * scale;
    }

    return -1.0f;
}
