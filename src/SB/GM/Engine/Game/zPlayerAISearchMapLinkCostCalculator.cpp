// zPlayerAISearchMapLinkCostCalculator.cpp -- one function, read from
// the image with tools/disasm.py, the sibling of zNPCSearchMapLinkCost-
// Calculator's: a disabled nav link refuses; a nav link with its
// third flag asks the calculator's own player, through its 159th virtual, and
// refuses when it says no; then a wall-net entity costs the link
// through the base class and a wall-net group costs it as the
// Hausdorff distance between the two nets, the link's node's net
// asking about the node's. -1.0f is the refusal (@174279) and the
// multiplier is the 1.0f at @174197, loaded into f31 before the switch
// and applied after each call: a local float folds the multiply away,
// a const reference bound to the literal keeps it. The three nav tests
// are one condition, which is one refusal block; the type dispatch is a
// switch, which is the compare chain; the group's net is declared ahead
// of the group so it takes the higher register.

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

class zWallNet {
public:
    float CalcHausdorffDistance(const zWallNet* other) const;
};

// The calculator's player, with the one virtual the calculator asks at its
// slot; the slots before it exist only to put it there.
class zEntVirtuals {
public:
    virtual void _v0();   virtual void _v1();   virtual void _v2();
    virtual void _v3();   virtual void _v4();   virtual void _v5();
    virtual void _v6();   virtual void _v7();   virtual void _v8();
    virtual void _v9();   virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();
    virtual void _v15();  virtual void _v16();  virtual void _v17();
    virtual void _v18();  virtual void _v19();  virtual void _v20();
    virtual void _v21();  virtual void _v22();  virtual void _v23();
    virtual void _v24();  virtual void _v25();  virtual void _v26();
    virtual void _v27();  virtual void _v28();  virtual void _v29();
    virtual void _v30();  virtual void _v31();  virtual void _v32();
    virtual void _v33();  virtual void _v34();  virtual void _v35();
    virtual void _v36();  virtual void _v37();  virtual void _v38();
    virtual void _v39();  virtual void _v40();  virtual void _v41();
    virtual void _v42();  virtual void _v43();  virtual void _v44();
    virtual void _v45();  virtual void _v46();  virtual void _v47();
    virtual void _v48();  virtual void _v49();  virtual void _v50();
    virtual void _v51();  virtual void _v52();  virtual void _v53();
    virtual void _v54();  virtual void _v55();  virtual void _v56();
    virtual void _v57();  virtual void _v58();  virtual void _v59();
    virtual void _v60();  virtual void _v61();  virtual void _v62();
    virtual void _v63();  virtual void _v64();  virtual void _v65();
    virtual void _v66();  virtual void _v67();  virtual void _v68();
    virtual void _v69();  virtual void _v70();  virtual void _v71();
    virtual void _v72();  virtual void _v73();  virtual void _v74();
    virtual void _v75();  virtual void _v76();  virtual void _v77();
    virtual void _v78();  virtual void _v79();  virtual void _v80();
    virtual void _v81();  virtual void _v82();  virtual void _v83();
    virtual void _v84();  virtual void _v85();  virtual void _v86();
    virtual void _v87();  virtual void _v88();  virtual void _v89();
    virtual void _v90();  virtual void _v91();  virtual void _v92();
    virtual void _v93();  virtual void _v94();  virtual void _v95();
    virtual void _v96();  virtual void _v97();  virtual void _v98();
    virtual void _v99();  virtual void _v100(); virtual void _v101();
    virtual void _v102(); virtual void _v103(); virtual void _v104();
    virtual void _v105(); virtual void _v106(); virtual void _v107();
    virtual void _v108(); virtual void _v109(); virtual void _v110();
    virtual void _v111(); virtual void _v112(); virtual void _v113();
    virtual void _v114(); virtual void _v115(); virtual void _v116();
    virtual void _v117(); virtual void _v118(); virtual void _v119();
    virtual void _v120(); virtual void _v121(); virtual void _v122();
    virtual void _v123(); virtual void _v124(); virtual void _v125();
    virtual void _v126(); virtual void _v127(); virtual void _v128();
    virtual void _v129(); virtual void _v130(); virtual void _v131();
    virtual void _v132(); virtual void _v133(); virtual void _v134();
    virtual void _v135(); virtual void _v136(); virtual void _v137();
    virtual void _v138(); virtual void _v139(); virtual void _v140();
    virtual void _v141(); virtual void _v142(); virtual void _v143();
    virtual void _v144(); virtual void _v145(); virtual void _v146();
    virtual void _v147(); virtual void _v148(); virtual void _v149();
    virtual void _v150(); virtual void _v151(); virtual void _v152();
    virtual void _v153(); virtual void _v154(); virtual void _v155();
    virtual void _v156(); virtual void _v157();
    virtual int AllowsPlayerAI();
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

class xVec3 {
public:
    float x;
    float y;
    float z;
};

class zSearchMapLinkCostCalculator {
public:
    virtual void __key();
};

class zPathFinderSearchMapLinkCostCalculator : public zSearchMapLinkCostCalculator {
public:
    float CalculateWallNetCost(const zWallNet* net,
                               const zSearchMapLink* link, int a, int b,
                               void* p, void* q) const;

    xVec3 startPos;
};

class zPlayerAISearchMapLinkCostCalculator
    : public zPathFinderSearchMapLinkCostCalculator {
public:
    float CalculateCost(const zSearchMap* map, const zSearchMapNode* node,
                        const zSearchMapLink* link, void* p, void* q) const;

    zEntVirtuals* thePlayer;
};

float zPlayerAISearchMapLinkCostCalculator::CalculateCost(
    const zSearchMap* map, const zSearchMapNode* node,
    const zSearchMapLink* link, void* p, void* q) const {
    zNavLink* nav = ((const zSearchMapLinkImpl*)link)->navLink;

    if (nav && !nav->IsEnabled()) {
        return -1.0f;
    }

    if (nav && (nav->info->flags & 4)) {
        if (!thePlayer->AllowsPlayerAI()) {
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
        zWallNet* linkNet;
        zWallNetGroup* group = (zWallNetGroup*)((const zSearchMapImpl*)map)->entity;

        linkNet = group->GetWallNet(((const zSearchMapLinkImpl*)link)->node->index);

        zWallNet* nodeNet = group->GetWallNet(((const zSearchMapNodeImpl*)node)->index);
        float cost = linkNet->CalcHausdorffDistance(nodeNet);

        return cost * scale;
    }
    default:
        return -1.0f;
    }
}
