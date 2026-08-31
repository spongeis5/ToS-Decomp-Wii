// C:/branches/SB09/main/GM/Engine/Core/x/xSpringy.cpp
//
// xSpringyVec3 is not in the recovered type table; its layout is read off
// the offsets the function touches, and the four fields land exactly:
//
//   +0x00 float damp      (loaded as a scalar, reloaded before each spring)
//   +0x04 xVec3 vel       (0x04 / 0x08 / 0x0C, one per component)
//   +0x10 xVec3 dest
//   +0x1C xVec3 val
//
// The decay term is computed ONCE in double precision and rounded back:
// `fmuls` for -damp * dt, a call to `exp` (not expf -- the image has no
// expf), then `frsp`. That `frsp` is the assignment to a float local; it
// is what pins the term to a variable rather than an expression used three
// times, and the three springs then share f31.
//
// xVec3::Sub is a member on the DESTINATION: `d.Sub(a, b)` is d = a - b,
// with d in r3. The second call writes val in place from dest and the
// delta the springs just moved.

class xVec3 {
public:
    void Sub(const xVec3& a, const xVec3& b);

    float x;
    float y;
    float z;
};

extern "C" double exp(double x);

void xDampSpring(float& val, float& vel, float dt, float damp, float decay);

class xSpringyVec3 {
public:
    void Update(float dt);

    float damp;
    xVec3 vel;
    xVec3 dest;
    xVec3 val;
};

void xSpringyVec3::Update(float dt) {
    xVec3 delta;
    float decay = exp(-damp * dt);

    delta.Sub(dest, val);

    xDampSpring(delta.x, vel.x, dt, damp, decay);
    xDampSpring(delta.y, vel.y, dt, damp, decay);
    xDampSpring(delta.z, vel.z, dt, damp, decay);

    val.Sub(dest, delta);
}
