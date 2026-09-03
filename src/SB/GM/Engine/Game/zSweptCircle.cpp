// zSweptCircle.cpp -- three functions, read from the image with
// tools/disasm.py. A swept circle is a 2D capsule cast across the XZ
// plane: a start point, a radius, a unit direction and a distance, plus
// the running result of the sweep so far. PrepareXZ flattens a 3D start
// and direction into XZ, works out the end point, normalises the
// direction (and turns a degenerate one into +Y with no distance),
// builds the left normal and resets the result. Reset puts the running
// distance and end back to the full sweep and clears the hit flag.
// SweptCircleToPoint sweeps against one point: it rejects a point
// behind the start, one further sideways than the radius, reports an
// immediate hit when the point is already inside the circle, and
// otherwise solves for the distance at which the circle touches it,
// keeping the hit only when that is nearer than the best so far.
//
// Layouts from the DWARF (tools/dwarf_types.py): zSweptCircle 0x40 --
// start 0x0, radius 0x8, dist 0xC, dir 0x10, end 0x18, left 0x20,
// curDist 0x28, curEnd 0x2C, hitSomething 0x34, contactPoint 0x38 --
// with xVec2 0x8 and xVec3 0xC. The parameter and local NAMES, and the
// statement structure, are the DWARF's too: tools/dwarf_locals.py gives
// start/radius/dir/dist, end and dirLength for the first, and toStart,
// startAlong, distAway, distToStart2 and backDist for the third, and
// tools/dwarf_lines.py gives which source line each instruction came
// from, which is where the statement order and grouping below come
// from. The line NUMBERS are not reproduced -- this file carries its
// own header and include in front of the code -- only the order and the
// grouping. The three literals are the unity unit's, hence the
// generated padding header first.
//
// Four shapes the bytes fixed. Both rejections are `<=` and `>=`
// against a float, and each carries the cror that an ordered compare of
// that form needs -- the lever NOTES already records, applied rather
// than re-measured, and it came out right on the first compile. The
// two-component
// writes are single source lines in the DWARF's line table, so the
// reciprocal is written twice and common-subexpression folds it to one
// fdivs; a named local for it would be a DWARF local, and there is
// none. curDist's floor is a conditional EXPRESSION written the way
// round that puts the zero in hand on the TRUE side -- `0.0f > curDist
// ? 0.0f : curDist` -- which branches to the else block and jumps past
// it with a bare ble; every <= form carries a cror and every other
// operand order picks different FPRs (eight spellings swept, scratch
// agx_sc_gen.py: 104, 102, 99, 96 and five at 95 of 104). And xVec2's
// assignment is the implicit operator=, out of line as
// __as__5xVec2FRC5xVec2, so it is declared and not defined.
//
// NEAR MISS: none. All three functions are byte-identical by
// tools/unitcmp.py.

#include "SB/GM/Engine/Game/zSweptCircle.pool.h"

namespace Math {
float sqrt(float x);
}

class xVec2 {
public:
    xVec2& operator=(const xVec2& other);
    void assign(float x, float y);
    xVec2& Sub(const xVec2& a, const xVec2& b);
    xVec2& AddScale(const xVec2& a, const xVec2& b, float s);
    float dot(const xVec2& other) const;
    float length2() const;

    float x;
    float y;
};

class xVec3 {
public:
    xVec3& AddScale(const xVec3& a, const xVec3& b, float s);

    float x;
    float y;
    float z;
};

class zSweptCircle {
public:
    void PrepareXZ(const xVec3& start, float radius, const xVec3& dir,
                   float dist);
    void Reset();
    bool SweptCircleToPoint(xVec2& point);

    xVec2 start;
    float radius;
    float dist;
    xVec2 dir;
    xVec2 end;
    xVec2 left;
    float curDist;
    xVec2 curEnd;
    bool hitSomething;
    xVec2 contactPoint;
};

void zSweptCircle::PrepareXZ(const xVec3& start, float radius,
                             const xVec3& dir, float dist) {
    this->start.assign(start.x, start.z);

    this->radius = radius;

    xVec3 end;

    end.AddScale(start, dir, dist);

    this->end.assign(end.x, end.z);

    this->dir.assign(dir.x, dir.z);

    float dirLength = Math::sqrt(this->dir.length2());

    if (dirLength < 1e-05f) {
        this->dir.x = 0.0f;
        this->dir.y = 1.0f;
        this->dist = 0.0f;
    } else {
        this->dir.x = this->dir.x * (1.0f / dirLength);
        this->dir.y = this->dir.y * (1.0f / dirLength);
        this->dist = dist * dirLength;
    }

    left.assign(-this->dir.y, this->dir.x);

    Reset();
}

void zSweptCircle::Reset() {
    curDist = dist;
    curEnd = end;
    hitSomething = false;
}

bool zSweptCircle::SweptCircleToPoint(xVec2& point) {
    xVec2 toStart;

    toStart.Sub(start, point);

    float startAlong = -dir.dot(toStart);

    if (startAlong <= 0.0f) {
        return false;
    }

    float distAway = left.dot(toStart);

    if (distAway <= -radius || distAway >= radius) {
        return false;
    }

    float distToStart2 = toStart.length2();

    if (distToStart2 <= radius * radius) {
        hitSomething = true;
        curDist = 0.0f;
        curEnd = start;
        contactPoint = point;

        return true;
    }

    float backDist =
        radius - Math::sqrt(radius * radius - distAway * distAway + 1e-05f);

    if (startAlong < curDist + radius - backDist) {
        curDist = startAlong + backDist - radius;
        curDist = 0.0f > curDist ? 0.0f : curDist;
        curEnd.AddScale(start, dir, curDist);
        hitSomething = true;
        contactPoint = point;

        return true;
    }

    return false;
}
