// zNPCSearchMapLinkCostCalculator::CalculateCost -- one function, read
// from the image with tools/disasm.py. -1.0f is the refusal (the literal
// at 8068BA4C) and 1.0f the multiplier (8068BA48). The three nav tests
// are one `||` condition, which is why retail has one refusal block for
// them; the type dispatch is a switch, which is the compare chain; the
// multiplier is a const reference bound to the literal, which is what
// keeps a multiply by one alive (a local float folds it, a division
// stays a division) -- the same constructs matched the sibling
// zPlayerAISearchMapLinkCostCalculator exactly on 2026-09-02. The group
// in the second case is a case-local so the entity stays in a volatile
// register; the two GetWallNet calls are the arguments of one call and
// so evaluate right to left, the link's node fetched first.
//
// NEAR MISS, 49 of 74 words, all of them register allocation. Retail
// keeps this in r31, map/node/link in r25..r27, q in r28, nav in r29 and
// p in r30; ours keeps this in r25, the five parameters in r26..r30 in
// order and nav in r31 -- which is exactly the order retail uses in the
// sibling. Whatever put this last in retail here is not the base calls
// qualified with `this->` (tried), not the switch, and not the
// reference; it is what is left to find.

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
        if (!nav->IsEnabled() || (nav->info->flags & 4) || (nav->info->flags & 8)) {
            return -1.0f;
        }
    }

    const float& scale = 1.0f;

    switch (((const zSearchMapImpl*)map)->entity->type) {
    case 0x62: {
        float cost = CalculateWallNetCost(
            (const zWallNet*)((const zSearchMapImpl*)map)->entity, link,
            ((const zSearchMapNodeImpl*)node)->index,
            ((const zSearchMapLinkImpl*)link)->node->index, p, q);

        return cost * scale;
    }
    case 0xB1: {
        zWallNetGroup* group =
            (zWallNetGroup*)((const zSearchMapImpl*)map)->entity;
        float cost = CalculateWallNetGroupCost(
            group->GetWallNet(((const zSearchMapNodeImpl*)node)->index),
            group->GetWallNet(((const zSearchMapLinkImpl*)link)->node->index));

        return cost * scale;
    }
    default:
        return -1.0f;
    }
}
