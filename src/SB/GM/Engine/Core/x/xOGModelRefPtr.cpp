// C:/branches/SB09/main/GM/Engine/Core/x/xOGModelRefPtr.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class xOGModelRefPtr /* 0x4 */ { RefData* mData; };
//   class RefData        /* 0x8 */ { xOGModelRef* mParent; int mRefCount; };
//   class xOGModelRef    /* 0x8 */ { xOGModel* data; xOGModelRefPtr* autoptr; };
//   class xOGModelHandle /* 0x8 */ : xOGModelRef {};
//
// A weak pointer with the back-link kept on the referent: xOGModelRef owns
// one xOGModelRefPtr, that pointer owns a RefData, and the RefData points
// back at the ref. Destroying the handle nulls the RefData's back-link, so
// every copy of the pointer goes dead at once without walking a list.
//
// IsSet loads mData ONCE into r5 and then loads mData->mParent TWICE. That
// is the shape of an inlined `A && B` where A is a small const predicate --
// the flag it computes lands in r4 and is then re-tested by the outer `&&`,
// which is the redundant `li r4, 1` / `cmpwi r4, 0` pair at 8003AEF0. The
// predicate has no symbol of its own anywhere in the image, so its NAME is
// a guess; only its body is recovered.
//
// GetWeakPtr allocates 4 bytes BEFORE 8: `new xOGModelRefPtr(new RefData)`
// evaluates the outer operator new first, then the argument. Writing the
// inner allocation as a separate statement puts the 8 first and does not
// match.
//
// It also assigns through a const `this`. The cast is real -- the RefData
// stores a non-const back-pointer to the referent it was made from.
//
// NEAR MISS, 3 of 4. GetWeakPtr's thirty words are right except for WHERE
// the first load sits in the prologue: retail has
//   stwu / mflr / stw lr / stmw / mr r30,r3 / lwz r0,4(r3)
// and every spelling tried puts that lwz one slot earlier, ahead of the
// stmw. The body is identical in all of them.
//
// Already excluded, do not redo:
//   * seven source spellings -- test the member, `!autoptr`, cast-local
//     first, a local for the allocation, the RefData allocated inside the
//     xOGModelRefPtr constructor, cast at each use, and an early return.
//     Six emit the same bytes; the early return emits 28 of 30 different.
//   * twelve flag sets, each also re-measured against every unit that
//     already matches: -schedule off, -opt noschedule, -opt speed,
//     -opt level=3, -inline on, -inline all, -inline auto,level=2,
//     -opt nopeephole, -proc 750, -func_align 4, -opt nospace. None move
//     this function, and five of them regress units that match today.
//
// So the lever is neither the source text nor a compiler option reachable
// from here. It is the instruction scheduler's placement, and what is left
// to find is what makes it place the load later.

namespace World {
class xOGModel;
class xOGModelRef;
class xOGModelRefPtr;
}

void* operator new(unsigned long size);
void operator delete(void* mem);

namespace World {

class xOGModel {
public:
    void DeferDestroy();
};

class RefData {
public:
    RefData(xOGModelRef* parent) {
        mParent = parent;
        mRefCount = 0;
    }

    xOGModelRef* mParent;
    int mRefCount;
};

class xOGModelRefPtr {
public:
    xOGModelRefPtr(RefData* data) { mData = data; }

    bool IsValid() const { return mData != 0 && mData->mParent != 0; }
    bool IsSet() const;

    xOGModelRefPtr& operator=(const xOGModelRef* const ref);

    void incRef();
    void decRef();

    RefData* mData;
};

class xOGModelRef {
public:
    xOGModelRefPtr* GetWeakPtr() const;

    xOGModel* data;
    xOGModelRefPtr* autoptr;
};

class xOGModelHandle : public xOGModelRef {
public:
    ~xOGModelHandle();
};

bool xOGModelRefPtr::IsSet() const {
    return IsValid() && mData->mParent->data != 0;
}

xOGModelRefPtr& xOGModelRefPtr::operator=(const xOGModelRef* const ref) {
    xOGModelRefPtr* other = ref->GetWeakPtr();

    decRef();
    mData = other->mData;
    incRef();

    return *this;
}

xOGModelRefPtr* xOGModelRef::GetWeakPtr() const {
    if (autoptr == 0) {
        xOGModelRef* self = (xOGModelRef*)this;

        self->autoptr = new xOGModelRefPtr(new RefData(self));
        self->autoptr->incRef();
    }

    return autoptr;
}

xOGModelHandle::~xOGModelHandle() {
    xOGModelRefPtr* ptr = autoptr;

    if (ptr != 0) {
        if (ptr->mData != 0) {
            ptr->mData->mParent = 0;
        }
        ptr->decRef();
    }

    if (data != 0) {
        data->DeferDestroy();
        data = 0;
    }
}

}  // namespace World
