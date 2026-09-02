// WAD00_9_1 -- one function, read from the image with tools/disasm.py:
// xMath2NearestPointOnLine projects a point onto the segment from A to
// B, writes the nearest point through the two references and returns
// the parameter along the segment, clamped to 0 at A and 1 at B. The
// literals are the pool's 0.0f (@217794) and 1.0f (@216698). Products are
// written with the first term the one the compiler folds into the
// fused multiply-add.

float xMath2NearestPointOnLine(float& ox, float& oy, float px, float py,
                               float ax, float ay, float bx, float by) {
    float dx = bx - ax;
    float dy = by - ay;
    float dpx = px - ax;
    float dpy = py - ay;
    float dot = dx * dpx + dy * dpy;

    if (dot <= 0.0f) {
        ox = ax;
        oy = ay;
        return 0.0f;
    }

    float len = dx * dx + dy * dy;

    if (dot >= len) {
        ox = bx;
        oy = by;
        return 1.0f;
    }

    float t = dot / len;

    ox = dx * t + ax;
    oy = dy * t + ay;

    return t;
}
