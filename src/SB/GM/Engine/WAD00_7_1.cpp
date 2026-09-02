// WAD00_7_1 -- one function, read from the image with tools/disasm.py:
// Graphics::LightPointSpot::SetRadius stores a radius under 1e-4f as
// zero with a zero reciprocal, and otherwise the radius and its
// reciprocal. The three literals are the pool's 1e-4f (@227578), 0.0f
// (@217794) and 1.0f (@216698), each loaded from its own base; the
// generated header carries the .rodata the unity unit puts ahead of
// them. Layout from the DWARF (LightPointSpot 0x5C on Node 0xC); the
// two floats are at +0x38, inside `data`, and +0x4C, the bound's first
// word.

#include "SB/GM/Engine/WAD00_7_1.pool.h"

namespace Graphics {

class LightPointSpot {
public:
    void SetRadius(float radius);

    unsigned char _pad0[0x38];
    float invRadius;
    unsigned char _pad1[0x10];
    float boundRadius;
    unsigned char _pad2[0xC];
};

}  // namespace Graphics

void Graphics::LightPointSpot::SetRadius(float radius) {
    if (radius < 1e-4f) {
        invRadius = 0.0f;
        boundRadius = 0.0f;
    } else {
        boundRadius = radius;
        invRadius = 1.0f / radius;
    }
}
