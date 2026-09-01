// zLaserScanner -- one function, the line-of-sight ray collector.
//
// Read from the image with tools/disasm.py:
//
//   li   r30,0                          the entity starts null
//   bl   getRootCollidable              on the body
//   bl   hkGetRigidBody  -> r31
//   bl   hkGetPhantom    -> r3
//   cmpwi r31,0 ; beq L1 ; lwz r30,12(r31) ; b L2
//   L1: cmpwi r3,0 ; beq L2 ; lwz r30,12(r3)
//                                       so it is rigid body ELSE phantom,
//                                       and the owner is at offset 12 of
//                                       whichever answered
//   L2: cmpwi r30,0 ; beq L3            a null entity takes the hit
//       lwz r0,12(r28) ; cmplw ; beq L4 and either of the two the
//       lwz r0,16(r28) ; cmplw ; bne L3 collector was told to ignore
//       b L4                            does not
//   L3: stb 1,8(r28) ; stfs f0,4(r28)   the literal at 8068BA28 is
//                                       0x00000000 -- 0.0f
//
// Both `hkGetRigidBody` and `hkGetPhantom` are called before either result
// is tested, which is why the phantom's is still live in r3 at L1.

class hkpCollidable;
class hkpRigidBody;
class hkpPhantom;
class hkpShapeRayCastCollectorOutput;

class hkpCdBody {
public:
    const hkpCollidable* getRootCollidable() const;
};

class xBase;

class hkpWorldObjectOwner {
public:
    unsigned char _pad[0xC];
    xBase* owner;
};

hkpRigidBody* hkGetRigidBody(const hkpCollidable* c);
hkpPhantom* hkGetPhantom(const hkpCollidable* c);

namespace zLaserScannerNS {

class zLaserScannerLOSHitCollector {
public:
    void addRayHit(const hkpCdBody& body,
                   const hkpShapeRayCastCollectorOutput& out);

    unsigned char _pad0[0x4];
    float fraction;
    unsigned char hit;
    unsigned char _pad1[0x3];
    xBase* ignoreA;
    xBase* ignoreB;
};

}  // namespace zLaserScannerNS

void zLaserScannerNS::zLaserScannerLOSHitCollector::addRayHit(
    const hkpCdBody& body, const hkpShapeRayCastCollectorOutput&) {
    xBase* ent = 0;

    const hkpCollidable* c = body.getRootCollidable();
    hkpRigidBody* rb = hkGetRigidBody(c);
    hkpPhantom* ph = hkGetPhantom(c);

    if (rb) {
        ent = ((hkpWorldObjectOwner*)rb)->owner;
    } else if (ph) {
        ent = ((hkpWorldObjectOwner*)ph)->owner;
    }

    if (ent != 0 && (ent == ignoreA || ent == ignoreB)) {
        return;
    }

    hit = 1;
    fraction = 0.0f;
}
