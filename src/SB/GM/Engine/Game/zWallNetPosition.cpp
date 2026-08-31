// C:/branches/SB09/main/GM/Engine/Game/zWallNetPosition.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class zWallNetPosition /* 0x24 */ {
//       /* +0x0  */ xVec3 curPos;
//       /* +0xC  */ xVec3 curPosOnWallNet;
//       /* +0x18 */ zWallNet* curWallNet;
//       /* +0x1C */ int curTriangleID;
//       /* +0x20 */ bool isOnWallNet;
//   };
//   class zWallNet /* 0x60 */ { ... /* +0x40 */ bool isOn; ... };
//   enum eWallNetUserType { Player = 0, NPC = 1 };
//
// xVec3 has an out-of-line operator= AND operator== here -- every
// assignment is a `bl`, which is why this reads as call after call. The
// other units in this tree declare xVec3 as a plain three-float struct and
// get memberwise assignment; that spelling cannot produce these bytes.
//
// zIWallNet::FindWallNet is static: r3 holds the position, not a `this`.
//
// UpdatePosition calls it TWICE, from two different stack slots for the
// out-parameter (0xC and 8), so the two are separate text -- not one call
// after a merged condition. They also assign the five members in DIFFERENT
// ORDERS, which is what proves it: the on-wall-net path writes curPos and
// curPosOnWallNet first, the off-wall-net path writes curWallNet and
// curTriangleID first.
//
// The early-out `curPos == newPos` needs no register moves at all: r3 is
// still `this` (curPos is at +0) and r4 is still the argument.

class xBase;

class xVec3 {
public:
    xVec3& operator=(const xVec3& v);
    bool operator==(const xVec3& v) const;

    float x;
    float y;
    float z;
};

enum eWallNetUserType {
    eWallNetUserType_Player = 0,
    eWallNetUserType_NPC = 1
};

class zWallNet {
public:
    bool IsInTriangle(int triangleID, const xVec3& pos) const;
    int FindTriangleID(const xVec3& pos) const;

    unsigned char _head[0x40];
    bool isOn;
};

class zIWallNet {
public:
    static zWallNet* FindWallNet(const xVec3& pos, eWallNetUserType type,
                                 xBase* user, int* triangleID, bool clamp);
};

class zWallNetPosition {
public:
    zWallNetPosition();

    bool SetupPosition(const xVec3& newPos, eWallNetUserType type,
                       xBase* user);
    bool UpdatePosition(const xVec3& newPos, eWallNetUserType type,
                        xBase* user);

    xVec3 curPos;
    xVec3 curPosOnWallNet;
    zWallNet* curWallNet;
    int curTriangleID;
    bool isOnWallNet;
};

zWallNetPosition::zWallNetPosition() {
    curWallNet = 0;
    curTriangleID = 255;
    isOnWallNet = false;
}

bool zWallNetPosition::SetupPosition(const xVec3& newPos,
                                     eWallNetUserType type, xBase* user) {
    int triangleID;
    zWallNet* wallNet =
        zIWallNet::FindWallNet(newPos, type, user, &triangleID, true);

    if (wallNet != 0) {
        curWallNet = wallNet;
        curTriangleID = triangleID;
        curPosOnWallNet = newPos;
        isOnWallNet = true;
        curPos = newPos;
    } else {
        curWallNet = 0;
        curTriangleID = 255;
        curPosOnWallNet = newPos;
        isOnWallNet = false;
        curPos = newPos;
    }

    return isOnWallNet;
}

bool zWallNetPosition::UpdatePosition(const xVec3& newPos,
                                      eWallNetUserType type, xBase* user) {
    if (isOnWallNet) {
        int triangleID;
        zWallNet* wallNet;

        if (curWallNet->isOn) {
            int newTriangleID;

            if (curPos == newPos) {
                return true;
            }

            if (curWallNet->IsInTriangle(curTriangleID, newPos)) {
                curPos = newPos;
                curPosOnWallNet = newPos;
                return true;
            }

            newTriangleID = curWallNet->FindTriangleID(newPos);
            if (newTriangleID != 255) {
                curPos = newPos;
                curPosOnWallNet = newPos;
                curTriangleID = newTriangleID;
                return true;
            }
        }

        wallNet = zIWallNet::FindWallNet(newPos, type, user, &triangleID, true);
        if (wallNet != 0) {
            curPos = newPos;
            curPosOnWallNet = newPos;
            curWallNet = wallNet;
            curTriangleID = triangleID;
            isOnWallNet = true;
            return true;
        }

        curPos = newPos;
        isOnWallNet = false;
        return false;
    } else {
        int triangleID;
        zWallNet* wallNet =
            zIWallNet::FindWallNet(newPos, type, user, &triangleID, true);

        if (wallNet != 0) {
            curWallNet = wallNet;
            curTriangleID = triangleID;
            curPosOnWallNet = newPos;
            curPos = newPos;
            isOnWallNet = true;
            return true;
        }

        curPos = newPos;
        isOnWallNet = false;
        return false;
    }
}
