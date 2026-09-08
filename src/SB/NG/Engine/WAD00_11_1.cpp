// WAD00_11_1 -- one of the nine functions in this unit:
// World::ModelPrototypeEntity's destructor. The other eight are not
// written, 3,356 bytes between them, of which the asset's Create is
// 2,880 on its own.

void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

// The compiler's own destructor: the null-this test, the member at the
// offset below destroyed with the don't-delete flag, and operator
// delete when the CALLER's flag is positive, then `return this`. There
// is no second call, so nothing it derives from has a destructor.
//
// __dt__12hkBaseObjectFv is 64 bytes of null test, conditional operator
// delete and `return this` -- what a destructor with nothing to destroy
// compiles to -- so every trivial destructor in the image folded onto
// it and the member's real type is gone with the fold. Spelling the
// member as hkBaseObject is not a claim about what it was; it is what
// makes the relocation name the symbol that survived.
namespace World {

class ModelPrototypeEntity {
public:
    ~ModelPrototypeEntity();

    unsigned char _pad0[0x70];
    hkBaseObject f70;
};

}  // namespace World

World::ModelPrototypeEntity::~ModelPrototypeEntity() {}
