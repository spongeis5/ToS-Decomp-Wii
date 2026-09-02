// WAD02 -- one function, read from the image with tools/disasm.py:
// xMat3x3Tolocal takes the squared lengths of the three axes, multiplies
// the vector through the matrix, and divides each component by its
// axis's squared length. The three lengths are kept across the call in
// f29..f31; the layout is xMath's (each axis padded to sixteen bytes).

class xVec3 {
public:
    float length2() const;

    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xVec3 right;
    unsigned int flags;
    xVec3 up;
    unsigned int pad1;
    xVec3 at;
    unsigned int pad2;
};

void xMat3x3LMulVec(xVec3* out, const xMat3x3* m, const xVec3* v);

void xMat3x3Tolocal(xVec3* out, const xMat3x3* m, const xVec3* v) {
    float lr = m->right.length2();
    float lu = m->up.length2();
    float la = m->at.length2();

    xMat3x3LMulVec(out, m, v);

    out->x /= lr;
    out->y /= lu;
    out->z /= la;
}
