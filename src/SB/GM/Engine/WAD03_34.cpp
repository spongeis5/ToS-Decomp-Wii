// WAD03_34 -- two functions, read from the image with tools/disasm.py:
// xVec2::Sub, this = a - b, and xVec2::dot, with the y product formed
// first and the x product folded into the add.

class xVec2 {
public:
    void Sub(const xVec2& a, const xVec2& b);
    float dot(const xVec2& o) const;

    float x;
    float y;
};

void xVec2::Sub(const xVec2& a, const xVec2& b) {
    x = a.x - b.x;
    y = a.y - b.y;
}

float xVec2::dot(const xVec2& o) const { return x * o.x + y * o.y; }
