// zPlayerInputBase.cpp -- one function, read from the image with
// tools/disasm.py: zPlayerInput::GetViewportIndex asks the pad manager
// for this input's pad owner, twice as the bytes do, and returns the
// owner's word at +0xFC, or zero without an owner. Nothing in these
// twenty-two instructions names the owner's type or that word.

class zPlayerInput;

class zPlayerInputPadOwner {
public:
    unsigned char _pad0[0xFC];
    int viewportIndex;
};

class zPlayerInputPadMgr {
public:
    zPlayerInputPadOwner* GetPadOwner(zPlayerInput* input);
};

namespace zPlayerInputNS {
extern zPlayerInputPadMgr padManager;
}

class zPlayerInput {
public:
    int GetViewportIndex();
};

int zPlayerInput::GetViewportIndex() {
    if (zPlayerInputNS::padManager.GetPadOwner(this)) {
        return zPlayerInputNS::padManager.GetPadOwner(this)->viewportIndex;
    }

    return 0;
}
