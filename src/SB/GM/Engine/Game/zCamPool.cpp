// zCamPool.cpp -- eight functions, read from the image with
// tools/disasm.py. A camera pool owns up to six xCam pointers and
// registers itself in the game's eight-slot table of pools. The
// constructor clears the count and the round-robin cursor and takes the
// first free slot of gCamPools; the destructor gives that slot back and
// then runs the compiler's own delete-if-asked tail. StopAllPools is a
// static that walks the eight slots and stops every pool that exists.
// SceneExit calls the virtual in each camera's +28 slot and empties the
// pool.
// AddCamera takes a camera while there is room and the pointer is not
// null, calls its create and appends it. GetCamera answers the first
// camera whose owner uid matches and whose bit 0 is set. GetNextCamera
// walks the pool from the cursor, wrapping it, and returns the first
// camera whose bit 0 is CLEAR; if every one is running it stops them
// all, stops the first again, sets the cursor to 1 and returns it.
// StopAll stops every camera whose bit 0 is set.
//
// Layouts from the DWARF (tools/dwarf_types.py): zCamPoolBase is 0x20 --
// xCam* mCameras[6] at +0, mNumCameras at +0x18 and mNextCamera at
// +0x1C -- and xCam is 0x148 with flags at +0x94 and the 64-bit owner
// at +0xA0. gCamPools is the retail symbol at 0x80732A28, 0x20 bytes,
// which is the eight pointers the loops walk. The camera's own fields
// are all this unit reads; the rest of xCam is padding here.
//
// The vtable slots are recovered fact, the names only partly.
// __vt__4xCam at 806B4AC8 is 0x50 bytes: two leading zero words, then
// the virtuals. Slot +24 is create__4xCamFv, +32 start__4xCamFv, +36
// stop__4xCamFv, +56 get_next and +60 find_camera. SceneExit calls +28,
// which the image fills with the weak empty body every empty function
// folded into, so that method's NAME is gone from the image; it sits
// between create and start and is spelled `destroy` here. Nothing in
// these bytes depends on the spelling -- a virtual call is an index --
// so the slot is the fact and the name is a guess, said out loud.
//
// Four shapes the bytes fixed. The destructor's loop has no early exit
// at all, and compares the slot against `this` with cmplw, an unsigned
// compare, because both sides are pointers. GetCamera and StopAllPools
// are counted loops (mtctr/bdnz) because their bounds are a member read
// once and a literal 8; GetNextCamera and StopAll re-read mNumCameras
// at the BOTTOM of every iteration, so their bound is the member itself
// and the loop is entered by a branch to that test. AddCamera reads
// mNumCameras once for the append -- `mCameras[mNumCameras++] = cam`,
// one load, where two reads would reload after the create call. And
// GetCamera's 64-bit equality puts the PARAMETER first: retail is
// `xor r0,r5,r0`, the argument pair against the loaded member, and
// `cam->owner == owner` gives both xors the other way round -- two
// words, the whole of that function's first miss.
//
// Two spellings swept rather than assumed, and only one of them is a
// lever. The constructor's early exit is written `return`, and `break`
// compiles to the same fifteen words -- the bytes do not say which was
// written, and only rewriting the search as a while loop with a test
// after it moves anything (8 of 15, and 21 words). The append is a
// lever: it really is the post-increment subscript, since two
// statements give 22 of 26 words and one more than retail has, a named
// count 21 of 26, and incrementing before the store 24 of 26.
//
// NO NEAR MISS. `python tools/unitcmp.py SB/GM/Engine/Game/zCamPool.cpp`
// reports 8 of 8 byte-identical, 820 bytes, which is every function the
// split holds. That is unitcmp's answer and not the oracle's: the unit
// has not been through ninja, is not marked Matching in configure.py,
// and nothing here has been linked or placed.

typedef unsigned long long uid;

class xCam;
class zCamPoolBase;

// The game's table of pools: eight slots at 0x80732A28.
extern zCamPoolBase* gCamPools[8];

class xCam {
public:
    virtual void _v0();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void create();
    virtual void destroy();
    virtual void start();
    virtual void stop();

    unsigned char _pad0[0x94 - 0x4];
    int flags;
    unsigned char _pad1[0xA0 - 0x98];
    uid owner;
    unsigned char _pad2[0x148 - 0xA8];
};

class zCamPoolBase {
public:
    zCamPoolBase();
    ~zCamPoolBase();

    static void StopAllPools();

    void SceneExit();
    void AddCamera(xCam* cam, const char* name);
    xCam* GetCamera(uid owner);
    xCam* GetNextCamera();
    void StopAll();

    xCam* mCameras[6];
    int mNumCameras;
    int mNextCamera;
};

zCamPoolBase::zCamPoolBase() {
    int i;

    mNumCameras = 0;
    mNextCamera = 0;

    for (i = 0; i < 8; i++) {
        if (gCamPools[i] == 0) {
            gCamPools[i] = this;

            return;
        }
    }
}

zCamPoolBase::~zCamPoolBase() {
    int i;

    for (i = 0; i < 8; i++) {
        if (gCamPools[i] == this) {
            gCamPools[i] = 0;
        }
    }
}

void zCamPoolBase::StopAllPools() {
    int i;

    for (i = 0; i < 8; i++) {
        zCamPoolBase* pool = gCamPools[i];

        if (pool != 0) {
            pool->StopAll();
        }
    }
}

void zCamPoolBase::SceneExit() {
    int i;

    for (i = 0; i < mNumCameras; i++) {
        mCameras[i]->destroy();
    }

    mNumCameras = 0;
}

void zCamPoolBase::AddCamera(xCam* cam, const char* name) {
    if (mNumCameras < 6 && cam != 0) {
        cam->create();

        mCameras[mNumCameras++] = cam;
    }
}

xCam* zCamPoolBase::GetCamera(uid owner) {
    int i;

    for (i = 0; i < mNumCameras; i++) {
        xCam* cam = mCameras[i];

        if (owner == cam->owner && (cam->flags & 1)) {
            return cam;
        }
    }

    return 0;
}

xCam* zCamPoolBase::GetNextCamera() {
    int i;

    for (i = 0; i < mNumCameras; i++) {
        xCam* cam = mCameras[mNextCamera++];

        if (mNextCamera >= mNumCameras) {
            mNextCamera = 0;
        }

        if (!(cam->flags & 1)) {
            return cam;
        }
    }

    StopAll();

    {
        xCam* cam = mCameras[0];

        cam->stop();

        mNextCamera = 1;

        return cam;
    }
}

void zCamPoolBase::StopAll() {
    int i;

    for (i = 0; i < mNumCameras; i++) {
        xCam* cam = mCameras[i];

        if (cam->flags & 1) {
            cam->stop();
        }
    }
}
