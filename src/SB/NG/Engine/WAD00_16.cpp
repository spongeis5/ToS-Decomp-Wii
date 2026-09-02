// WAD00_16 -- one function, read from the image with tools/disasm.py:
// World::SkeletonBlobEntity's destructor, the compiler's own -- the
// null-this test, the Havok object at +0x18 destroyed with the
// don't-delete flag, then the Havok base's destructor on `this` with
// the flag clear, and operator delete when the caller's flag says so.
// Both Havok objects are spelled as the object itself, the base as the
// direct base and the skeleton as the member: a class of its own in
// either place gets a destructor of its own emitted and called. The
// class's vtable lives in the WAD00 blob, so an undefined virtual is
// declared ahead of the destructor: the first non-inline virtual is
// where the compiler emits the vtable, and it must not be this unit.

void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

namespace World {

class SkeletonBlobEntity : public hkBaseObject {
public:
    virtual void __key();
    ~SkeletonBlobEntity();

    unsigned char _pad0[0x14];
    hkBaseObject skel;
    unsigned char _pad1[0x3C];
};

}  // namespace World

World::SkeletonBlobEntity::~SkeletonBlobEntity() {}
