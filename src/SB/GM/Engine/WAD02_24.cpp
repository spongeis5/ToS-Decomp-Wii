// WAD02_24 -- one function, read from the image with tools/disasm.py:
// the zNPCRayHitCollector constructor. The base is Havok's ray hit
// collector -- a vptr and an early-out fraction set to 1.0f -- inlined
// in front of the vtable store; then the derived members: a byte
// cleared, a word cleared, the entity kept, and a fraction of 1.0f.
// Only the vtable is named by the image; the members are spelled by
// offset. The entity is stored after the fraction, so it is assigned
// in the body rather than the initialiser list.

class xBase;

class hkpRayHitCollector {
public:
    hkpRayHitCollector() : m_earlyOutHitFraction(1.0f) {}

    virtual void __vtable_anchor();

    float m_earlyOutHitFraction;
};

class zNPCRayHitCollector : public hkpRayHitCollector {
public:
    zNPCRayHitCollector(const xBase* ent);

    unsigned char f8;
    int fC;
    const xBase* f10;
    float f14;
};

zNPCRayHitCollector::zNPCRayHitCollector(const xBase* ent)
    : hkpRayHitCollector(), f8(0), fC(0), f14(1.0f) {
    f10 = ent;
}
