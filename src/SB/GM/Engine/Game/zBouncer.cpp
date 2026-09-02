// zBouncer.cpp -- two functions, read from the image with
// tools/disasm.py. Setup stores the owner, the normal (through xVec3's
// out-of-line operator=), the two bounce speeds and a quarter turn as
// the angle limit. BouncePlayer reflects the player's frame velocity
// about the normal, normalises it (falling back to the normal and the
// minimum speed when there is nothing to normalise), notes the bounce
// on a player of name 6 with two events when the bouncer changed,
// takes the normal and the maximum speed for action 0x15, clamps the
// speed between the two limits and the direction to the angle limit
// about the normal, writes the velocity back to the frame with its
// flag, hands Havok the new velocity as a four-vector by whichever
// character controller the player has, and fires the owner's event.
// The compiler copies a local initialised from a function result out of
// a temporary, so the reflection is one expression and the velocity is
// assigned from the product: only `dir` is a named copy, as retail has.
// The min/max are macros, which is why the inner expression is
// evaluated twice each time, dot3 included. The 16-byte four-vector is
// the dynamic frame alignment. Layouts from the DWARF (zBouncer 0x1C,
// zPlayer's frame at +0x58, action manager at +0xC0, character
// controller at +0x20C, name at +0x2EC); the four fields past 0x9E4
// are the stores.
//
// NEAR MISS, 136 of 184 words in BouncePlayer, every one of them the
// literal base: the function loads 2.0f, 1e-5f, 1.0f and 0.0f, retail
// with a `lis` for each, ours with an `addis` base past the padding
// header, which costs a callee-saved register and shifts `this` and
// `player` down one. Ruled out on 2026-09-02: the literals introduced
// by a function ahead of this one (reused rather than new) and
// introduced scattered among other literals as retail's sit. The same
// wall as zPlayerWalkSB::AddActionTransitions.

#include "SB/GM/Engine/Game/zBouncer.pool.h"

#define xmin(a, b) ((a) < (b) ? (a) : (b))
#define xmax(a, b) ((a) > (b) ? (a) : (b))

extern "C" double acos(double x);

class xBase;

namespace Sext {
class EventAny;
}

enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* any, ForceEvent force);

class hkVector4 {
public:
    float dot3(const hkVector4& other) const;

    float x;
    float y;
    float z;
    float w;
};

namespace Math {

// Sixteen-byte aligned, which is the dynamic frame alignment in the
// prologue of a function with one on its stack.
class Vector4 {
public:
    void Assign(float x, float y, float z, float w);

    operator const hkVector4&() const { return *(const hkVector4*)this; }

    float v[4] __attribute__((aligned(16)));
};

}  // namespace Math

class hkContactPoint {
public:
    void setSeparatingNormal(const hkVector4& normal);
};

class hkpCharacterRigidBody {
public:
    void setLinearVelocity(const hkVector4& velocity, float dt);
};

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    float NormalizeSafe();

    float x;
    float y;
    float z;
};

xVec3 operator*(const xVec3& v, float s);
xVec3 operator-(const xVec3& a, const xVec3& b);

void xVec3Rotate(xVec3* out, const xVec3* axis, const xVec3* v, float angle);

namespace Globals {
extern float dt;
}  // namespace Globals

class xEntFrame {
public:
    unsigned char _pad0[0x88];
    xVec3 vel;
    unsigned int flags;
};

class xOGModel {
public:
    unsigned char _pad0[0x34];
    float f34;
};

class zPlayerActionManager {
public:
    unsigned int GetCurrentActionID() const;

    unsigned char _pad0[0x38];
};

class xHavokCharacterController {
public:
    void* controller;
    int type;
};

class zPlayer {
public:
    virtual void __key();

    unsigned char _pad0[0x30];
    xOGModel* ogModel;
    unsigned char _pad1[0x20];
    xEntFrame* frame;
    unsigned char _pad2[0x64];
    zPlayerActionManager actionManager;
    unsigned char _pad3[0x114];
    xHavokCharacterController characterController;
    unsigned char _pad4[0xD8];
    int eName;
    unsigned char _pad5[0x6F4];
    xBase* lastBouncer;
    xBase* bouncer;
    float bounceHeight;
    bool bounced;
};

class zBouncer {
public:
    void Setup(xBase* ent, const xVec3& n, float minB, float maxB);
    void BouncePlayer(zPlayer* player, xVec3& outVel);

    xBase* owner;
    xVec3 normal;
    float minBounce;
    float maxBounce;
    float maxBounceAngle;
};

static inline void AssignXYZ(Math::Vector4& v, const xVec3& p) {
    v.Assign(p.x, p.y, p.z, 0.0f);
}

void zBouncer::Setup(xBase* ent, const xVec3& n, float minB, float maxB) {
    owner = ent;
    normal = n;
    minBounce = minB;
    maxBounce = maxB;
    maxBounceAngle = 0.7853982f;
}

void zBouncer::BouncePlayer(zPlayer* player, xVec3& outVel) {
    if (player->lastBouncer == owner) {
        return;
    }

    xVec3 vel = player->frame->vel;
    float d = ((const hkVector4&)vel).dot3((const hkVector4&)normal);
    xVec3 dir = vel - normal * (2.0f * d);
    float speed = dir.NormalizeSafe();

    if (speed < 1e-5f) {
        dir = normal;
        speed = minBounce;
    }

    if (player->eName == 6) {
        player->bounced = true;

        if (player->bouncer != owner) {
            zEntEvent((xBase*)player, 0, owner, 0x12E439D5, 0, (ForceEvent)1);
            zEntEvent((xBase*)player, 0, player->bouncer, 0x2B22FDD3, 0,
                      (ForceEvent)1);
        }

        player->bouncer = owner;
        player->lastBouncer = owner;
        player->bounceHeight = player->ogModel->f34;
    }

    if (player->actionManager.GetCurrentActionID() == 0x15) {
        dir = normal;
        speed = maxBounce;
    }

    speed = xmax(minBounce, xmin(speed, maxBounce));

    float c = xmin(((const hkVector4&)dir).dot3((const hkVector4&)normal), 1.0f);

    if ((float)acos(c) > maxBounceAngle) {
        xVec3Rotate(&dir, &normal, &dir, maxBounceAngle);
    }

    player->frame->flags |= 4;

    player->frame->vel = dir * speed;

    Math::Vector4 hv;

    AssignXYZ(hv, player->frame->vel);

    xHavokCharacterController cc = player->characterController;

    switch (cc.type) {
    case 1:
        ((hkContactPoint*)cc.controller)->setSeparatingNormal(hv);
        break;
    case 2:
        ((hkpCharacterRigidBody*)cc.controller)->setLinearVelocity(hv, Globals::dt);
        break;
    }

    zEntEvent(owner, 0, owner, 0x8DC39991, 0, (ForceEvent)1);
}
