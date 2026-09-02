// WAD03_33 -- one function, read from the image with tools/disasm.py:
// xVec2::AddScale, this = a + b * s, one fused multiply-add per
// component.

class xVec2 {
public:
    void AddScale(const xVec2& a, const xVec2& b, float s);

    float x;
    float y;
};

void xVec2::AddScale(const xVec2& a, const xVec2& b, float s) {
    x = b.x * s + a.x;
    y = b.y * s + a.y;
}
