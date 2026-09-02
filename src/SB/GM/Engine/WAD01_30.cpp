// WAD01_30 -- one function, read from the image with tools/disasm.py:
// xMat4x3FromTransform builds the four rows of the matrix from the
// columns of the transform, each by calling Math::Vector's three-float
// constructor on the row with no null test. Placement new adds the
// test and this compiler rejects the explicit constructor-call syntax
// (error 10409), so the constructor is called by its retail symbol.
// Layouts from the DWARF (xMat4x3 0x40 on xMat3x3 0x30, each axis
// padded to sixteen bytes; xTransform 0x30 in column-major order).

extern "C" void __ct__Q24Math6VectorFfff(void* vector, float x, float y,
                                         float z);

class xVec3 {
public:
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

class xMat4x3 : public xMat3x3 {
public:
    xVec3 pos;
    unsigned int pad3;
};

class xTransform {
public:
    float lx;
    float ux;
    float fx;
    float px;
    float ly;
    float uy;
    float fy;
    float py;
    float lz;
    float uz;
    float fz;
    float pz;
};

void xMat4x3FromTransform(xMat4x3* m, const xTransform* t) {
    __ct__Q24Math6VectorFfff(&m->right, t->lx, t->ly, t->lz);
    __ct__Q24Math6VectorFfff(&m->up, t->ux, t->uy, t->uz);
    __ct__Q24Math6VectorFfff(&m->at, t->fx, t->fy, t->fz);
    __ct__Q24Math6VectorFfff(&m->pos, t->px, t->py, t->pz);
}
