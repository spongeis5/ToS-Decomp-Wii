// xEvent.cpp -- five functions, read from the image with
// tools/disasm.py. This is the event router. zEntEvent(uid, event, any)
// looks the target up in the scene and sends to it. The six-argument
// zEntEvent is the whole of the routing: it delivers the event to the
// target's own callback when the target is enabled, records two pairs
// of events in a four-bit field, walks the target's link array sending
// each matching link on, and then walks the chain of template parents
// doing the same for links whose destination is one of the template's
// own widgets. LinkParametersMatch decides whether a link's recorded
// source parameters match the ones the event carries, either byte-wise
// or word-wise under a bit mask. The two zEntEventInternal functions
// deliver to one entity without any of the link walking.
//
// Layouts from the DWARF (tools/dwarf_types.py). xBase is 0x38 on
// Entity 0x18, with the 64-bit id at +0x18, baseType +0x20, baseFlags
// +0x26, linkArray +0x28, templateParent +0x2C and eventFunc +0x30.
// LinkAsset is 0x8 and holds one __EventLinksArray__: count at +0 and
// the data pointer at +4. Sext::LinkAssetBaseNew is 0x28 --
// srcEvent{type,v} at +0, dstEvent{type,v} at +8, dstAssetID at +0x10,
// chkAssetID at +0x18, chkSourceParams +0x20, disabled +0x21 and
// chkSourceMask +0x24 -- which is the 40 the second loop variable adds
// each iteration. TemplateEntity is 0x58 with asset at +0x3C and the
// event tree at +0x50, whose comparator is an empty base sharing the
// offset with the count and whose m_root is at +0x54; TemplateEventSent
// is 0xC, eventType at +0 and the tree node at +4 (left +4, the tagged
// right at +8). TemplateAsset is 0x28 with widgetCount at +0x10 and
// childStartUID at +0x18. The DWARF's Pointer32 wrapper is a plain
// pointer on this target and is spelled as one here.
//
// TWO THINGS ARE NOT IN THE DWARF and are said out loud rather than
// guessed at. Sext::EventAny has no layout at all -- one opaque byte --
// so what is written below is only what the bytes fix: two floats at +0
// and +4, and an object that is otherwise compared byte for byte
// against the link's source parameters. And the four-bit field at +0x4C
// of the target belongs to no type the DWARF names: it is written with
// lbz / rlwimi 28..31 / stb, so it is four bits at the BOTTOM of that
// byte, and the obvious candidate (xEnt, whose +0x4C the DWARF gives as
// a plain unsigned char union) does not have a bitfield there. A scan
// of every sized function in the game's text found exactly one that
// touches it -- this one, twice -- so nothing else in the image says
// what it is. The offset, the width and the two values, 1 and 2, are
// the recovered facts; the class below carries those and nothing else.
//
// The event ids are recovered from addis/cmplwi pairs and are exact:
// 0x2C9D0683 sets the enable bit, 0x71E42988 clears it, 0x389E01C0 is
// the one event a forced send still delivers, 0x4B1B1469 and 0x34716A29
// put 1 in the four-bit field, 0x6EDB6DE9 and 0xD6094F29 put 2, and
// 0xC0648E27 is the one event whose parameters are compared as floats.
//
// Six shapes the bytes fixed.
//
// The delivery test is `(baseFlags & 1) || !force || event == K`: the
// branch on `force` goes to the CALL, not past it, so the argument
// suppresses delivery rather than compelling it, whatever its name.
//
// The two mask tables are function-local statics, byte table first --
// the image has them eight bytes apart in that order -- with no guard
// symbol, which is what a static with no constructor gets.
//
// LinkParametersMatch has FIVE separate `li r3,1 ; blr` and three
// `li r3,0 ; blr`: nothing is tail-merged, so every exit is written as
// its own return, one per loop.
//
// The template-parent loop tests its parent for null AGAIN inside the
// body, and the four conditions that guard its link walk all branch to
// the same place, so the body is one `if (parent != 0)` around a second
// `if` of three ands.
//
// The tree lookup is written OUT in the caller with a goto: a helper
// with a loop is not inlined at these flags, and the `li r4,0` after
// the loop is the `return 0` of the helper it was copied from. Its
// comparator is `(int)event < node->eventType ? -1 : ((int)event >
// node->eventType)` -- SIGNED (`cmpw`), where the link test's compare
// of the same two fields is unsigned, and the `>` is materialised
// branchlessly.
//
// The template-widget test is a 64-bit AND with 0x00000FFFFFFFFFFF:
// mwcc folds the high half's 0xFFF into an rlwinm and needs a register
// for the low half's 0xFFFFFFFF, which is the `li r31,-1` hoisted out
// of the whole parent loop and ANDed with a value it cannot change.
//
// NEAR MISS -- see the paragraph at the foot of this file.

typedef unsigned long long uid;

class xBase;
class TemplateEntity;

enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

namespace Sext {

// No layout in the DWARF; the two floats are what the bytes fix.
class EventAny {
public:
    float value[2];
};

class __srcEvent__ {
public:
    int type;
    void* v;
};

class __dstEvent__ {
public:
    int type;
    void* v;
};

class LinkAssetBaseNew {
public:
    __srcEvent__ srcEvent;
    __dstEvent__ dstEvent;
    uid dstAssetID;
    uid chkAssetID;
    bool chkSourceParams;
    bool disabled;
    unsigned int chkSourceMask;
};

class TemplateAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short legacyLinkCount;
    unsigned short baseFlags;
    unsigned int widgetCount;
    unsigned int pad;
    uid childStartUID;
    int linkCount;
    LinkAssetBaseNew* linkArray;
};

}  // namespace Sext

class __EventLinksArray__ {
public:
    unsigned int count;
    Sext::LinkAssetBaseNew* data;
};

class LinkAsset {
public:
    __EventLinksArray__ EventLinksArray;
};

class xBase {
public:
    virtual void _v0();

    unsigned char _pad0[0x18 - 0x4];
    uid id;
    unsigned int baseType;
    unsigned char UNUSED_linkCount;
    unsigned char assertFlags;
    unsigned short baseFlags;
    LinkAsset* linkArray;
    TemplateEntity* templateParent;
    void (*eventFunc)(xBase* from, xBase* to, unsigned int event,
                      Sext::EventAny* any);
};

// The four-bit field at +0x4C, and nothing else. No DWARF type names
// it; see the head of this file.
// The pads below are measured from 0x34 and not from 0x38: this
// compiler lays a derived class out from the base's DATA size, so
// xBase's trailing alignment padding is where a derived class's first
// member goes -- which is also why the DWARF has TemplateEntity's asset
// at +0x3C inside xOGEntity's 0x40.
class xEventTarget : public xBase {
public:
    unsigned char _pad1[0x4C - 0x34];
    unsigned char _unknown : 4;
    unsigned char eventState : 4;
};

class TemplateEventSent {
public:
    int eventType;
    void* left;
    long right_color_bal;
};

class EmbeddedTreeAVL {
public:
    unsigned long size;
    TemplateEventSent* m_root;
};

class TemplateEntity : public xBase {
public:
    unsigned char _pad1[0x3C - 0x34];
    Sext::TemplateAsset* asset;
    unsigned int memberCount;
    void** memberList;
    TemplateEventSent* eventBuf;
    unsigned int eventBufSize;
    EmbeddedTreeAVL eventTree;
};

xBase* zSceneFindObject(uid id);

bool LinkParametersMatch(unsigned int event, Sext::EventAny* any,
                         const Sext::LinkAssetBaseNew* link);

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int event, Sext::EventAny* any, ForceEvent force);

void zEntEvent(uid to, unsigned int event, Sext::EventAny* any) {
    xBase* obj = zSceneFindObject(to);

    if (obj != 0) {
        zEntEvent(0, 0, obj, event, any, (ForceEvent)1);
    }
}

bool LinkParametersMatch(unsigned int event, Sext::EventAny* any,
                         const Sext::LinkAssetBaseNew* link) {
    static const unsigned char s_byteMaskTable[2] = { 0x00, 0xFF };
    static const unsigned int s_wordMaskTable[16] = {
        0x00000000, 0xFF000000, 0x00FF0000, 0xFFFF0000, 0x0000FF00,
        0xFF00FF00, 0x00FFFF00, 0xFFFFFF00, 0x000000FF, 0xFF0000FF,
        0x00FF00FF, 0xFFFF00FF, 0x0000FFFF, 0xFF00FFFF, 0x00FFFFFF,
        0xFFFFFFFF
    };

    if (any == 0) {
        return true;
    }

    unsigned int mask = link->chkSourceMask;

    if (mask != 0) {
        const unsigned char* a = (const unsigned char*)any;
        const unsigned char* b = (const unsigned char*)link->srcEvent.v;

        if (((unsigned int)a | (unsigned int)b) & 3) {
            do {
                if ((*a ^ *b) & s_byteMaskTable[mask & 1]) {
                    return false;
                }

                a++;
                b++;
            } while (mask >>= 1);

            return true;
        } else {
            do {
                if ((*(const unsigned int*)a ^ *(const unsigned int*)b) &
                    s_wordMaskTable[mask & 0xF]) {
                    return false;
                }

                a += 4;
                b += 4;
            } while (mask >>= 4);

            return true;
        }
    }

    if (event == 0xC0648E27) {
        const float* p = (const float*)link->srcEvent.v;

        if (any->value[0] > p[1] && any->value[1] <= p[1]) {
            return true;
        }

        return false;
    }

    return true;
}

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int event, Sext::EventAny* any, ForceEvent force) {
    unsigned int enabled;
    TemplateEntity* parent;

    if (to == 0) {
        return;
    }

    if (event == 0x2C9D0683) {
        to->baseFlags |= 1;
    }

    if (to->eventFunc != 0) {
        if ((to->baseFlags & 1) || !force || event == 0x389E01C0) {
            to->eventFunc(from, to, event, any);

            if (to->baseFlags & 0x20) {
                if (event == 0x4B1B1469 || event == 0x34716A29) {
                    ((xEventTarget*)to)->eventState = 1;
                } else if (event == 0x6EDB6DE9 || event == 0xD6094F29) {
                    ((xEventTarget*)to)->eventState = 2;
                }
            }
        }
    }

    enabled = to->baseFlags & 1;

    if (enabled && to->linkArray != 0 &&
        to->linkArray->EventLinksArray.count != 0) {
        for (unsigned int i = 0; i < to->linkArray->EventLinksArray.count;
             i++) {
            const Sext::LinkAssetBaseNew* link =
                &to->linkArray->EventLinksArray.data[i];
            xBase* dst;

            if (link->disabled) {
                continue;
            }

            if (event != (unsigned int)link->srcEvent.type) {
                continue;
            }

            if (link->chkAssetID != 0) {
                if (from == 0) {
                    continue;
                }

                if (from->id != link->chkAssetID) {
                    continue;
                }
            }

            if (link->chkSourceParams &&
                !LinkParametersMatch(event, any, link)) {
                continue;
            }

            dst = zSceneFindObject(link->dstAssetID);

            if (dst == 0) {
                continue;
            }

            zEntEvent(to, event, dst, link->dstEvent.type,
                      (Sext::EventAny*)link->dstEvent.v, (ForceEvent)1);
        }
    }

    for (parent = to->templateParent; parent != 0;
         parent = parent->templateParent) {
        if (parent != 0) {
            TemplateEventSent* node = parent->eventTree.m_root;
            TemplateEventSent* sent;

            while (node != 0) {
                int c = (int)event < node->eventType
                            ? -1
                            : ((int)event > node->eventType);

                if (c < 0) {
                    node = (TemplateEventSent*)node->left;
                } else if (c > 0) {
                    node = (TemplateEventSent*)(node->right_color_bal & ~3);
                } else {
                    sent = node;
                    goto haveSent;
                }
            }

            sent = 0;

        haveSent:
            if (sent == 0 && enabled && (parent->baseFlags & 1)) {
                for (unsigned int i = 0;
                     i < parent->linkArray->EventLinksArray.count; i++) {
                    const Sext::LinkAssetBaseNew* link =
                        &parent->linkArray->EventLinksArray.data[i];
                    xBase* dst;

                    if (link->disabled) {
                        continue;
                    }

                    if (event != (unsigned int)link->srcEvent.type) {
                        continue;
                    }

                    bool isWidget =
                        ((link->dstAssetID ^ parent->asset->childStartUID) &
                         0x00000FFFFFFFFFFFULL) == 0 &&
                        (unsigned int)(link->dstAssetID >> 44) -
                                (unsigned int)(parent->asset->childStartUID >>
                                               44) <
                            parent->asset->widgetCount;

                    if (isWidget) {
                        continue;
                    }

                    if (link->chkAssetID != 0) {
                        if (from == 0) {
                            continue;
                        }

                        if (from->id != link->chkAssetID) {
                            continue;
                        }
                    }

                    if (link->chkSourceParams &&
                        !LinkParametersMatch(event, any, link)) {
                        continue;
                    }

                    dst = zSceneFindObject(link->dstAssetID);

                    if (dst == 0) {
                        continue;
                    }

                    zEntEvent(parent, event, dst, link->dstEvent.type,
                              (Sext::EventAny*)link->dstEvent.v,
                              (ForceEvent)1);
                }
            }
        }
    }

    if (event == 0x71E42988) {
        to->baseFlags &= ~1;
    }
}

void zEntEventInternal(xBase* from, unsigned int fromEvent, xBase* to,
                       unsigned int event, Sext::EventAny* any,
                       ForceEvent force) {
    if (to == 0) {
        return;
    }

    if (to->eventFunc == 0) {
        return;
    }

    if (!(to->baseFlags & 1) && force && event != 0x389E01C0) {
        return;
    }

    to->eventFunc(from, to, event, any);
}

void zEntEventInternal(xBase* to, unsigned int event, Sext::EventAny* any,
                       ForceEvent force) {
    zEntEventInternal(0, 0, to, event, any, force);
}
