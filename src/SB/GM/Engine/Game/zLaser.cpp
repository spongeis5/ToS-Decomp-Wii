// C:/branches/SB09/main/GM/Engine/Game/zLaser.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class zLaserLOSChecker /* 0x1C */ : hkpRayHitCollector {
//       /* +0x8  */ xBase* ignoreStart;
//       /* +0xC  */ xBase* ignoreTarget;
//       /* +0x10 */ float minHitFraction;
//       /* +0x14 */ xBase* hitObj;
//       /* +0x18 */ bool hitIt;
//   };
//   class hkpRayHitCollector /* 0x8 */ { vptr; float m_earlyOutHitFraction; };
//   class hkpCollidable      /* +0x10 */ signed char m_ownerOffset;
//   class hkpWorldObject     /* +0xC  */ unsigned long m_userData;
//
// `lbz` / `extsb` / `add` on the collidable is hkpCollidable::getOwner()
// inlined: the owner is not a pointer, it is a SIGNED BYTE DISPLACEMENT
// stored inside the collidable, and the world object sits at that offset
// from it. The xBase comes off the owner's m_userData at +0xC.
//
// The two arms at the end store the same thing -- hitIt true, and hitObj
// either the owner or null, where the owner IS null. A compiler merges
// that; this one did not, which means the source really does branch and
// spell the null case out separately.
//
// PROLOGUE, and this was the reason the unit was left until last: it saves
// r29-r31 by CALLING __save_gpr instead of emitting `stmw`, which no other
// unit in the tree does. It comes out of the flags unchanged -- `addi r11,
// r1, 0x20` and the call are words 3 and 4 of the very first attempt. So
// `-use_lmw_stmw on` really does PERMIT rather than force, and the choice
// between stmw, stw pairs and the helper is the compiler's, made per
// function. Nothing needs to be done about it.
//
// The three ignore tests are ONE condition, not two nested ifs. Written as
// `if (base) { if (a) return; if (b) return; }` the compiler inverts the
// last test into a single `beq` to the exit and the function is one
// instruction short. Retail has `bne` over a `b`, which is the shape of a
// SHARED return block -- both arms of an `||` jumping to the same place.

class xBase;
class hkpShape;

class hkpCdBody;

class hkpWorldObject {
public:
    enum MtChecks {
        MT_CHECK_ENABLED = 0,
        MT_CHECK_DISABLED = 1
    };

    bool hasProperty(unsigned int key,
                     MtChecks checks = MT_CHECK_ENABLED) const;

    unsigned char _head[0xC];
    unsigned long m_userData;
};

class hkpCollidable {
public:
    hkpWorldObject* getOwner() const {
        return (hkpWorldObject*)((char*)this + m_ownerOffset);
    }

    unsigned char _head[0x10];
    signed char m_ownerOffset;
};

class hkpCdBody {
public:
    const hkpCollidable* getRootCollidable() const;
};

class hkVector4 {
public:
    float x;
    float y;
    float z;
    float w;
};

class hkpShapeRayCastCollectorOutput {
public:
    hkVector4 m_normal;
    float m_hitFraction;
    int m_extraInfo;
    int m_pad[2];
};

class hkpRayHitCollector {
public:
    virtual void addRayHit(const hkpCdBody& cdBody,
                           const hkpShapeRayCastCollectorOutput& hitInfo) = 0;

    float m_earlyOutHitFraction;
};

class zLaserLOSChecker : public hkpRayHitCollector {
public:
    virtual void addRayHit(const hkpCdBody& cdBody,
                           const hkpShapeRayCastCollectorOutput& hitInfo);

    xBase* ignoreStart;
    xBase* ignoreTarget;
    float minHitFraction;
    xBase* hitObj;
    bool hitIt;
};

void zLaserLOSChecker::addRayHit(
    const hkpCdBody& cdBody,
    const hkpShapeRayCastCollectorOutput& hitInfo) {
    hkpWorldObject* owner = cdBody.getRootCollidable()->getOwner();
    xBase* base = (xBase*)owner->m_userData;

    if (owner->hasProperty(0x1E61)) {
        return;
    }

    if (base != 0 && (base == ignoreStart || base == ignoreTarget)) {
        return;
    }

    if (hitInfo.m_hitFraction < minHitFraction) {
        return;
    }

    m_earlyOutHitFraction = hitInfo.m_hitFraction;

    if (base == 0) {
        hitIt = true;
        hitObj = 0;
    } else {
        hitIt = true;
        hitObj = base;
    }
}
