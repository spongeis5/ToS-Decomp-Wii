// MathUtil.cpp -- three functions in the image, two written here.
// StartupMathUtil fills the binomial triangle, 32 rows laid end to
// end, each row's ends 1 and its middle the sum of the two above,
// walked with two pointers over the previous row. DampSpring is a
// critically damped spring step on a position and velocity passed by
// reference. ConvertOBBToAABB is written in paired-single intrinsics
// (psq_l, ps_madds0, ps_sum0) and is not attempted here.

namespace Math {

extern unsigned int binomTriangle[];

void StartupMathUtil();
void DampSpring(float& x, float& v, float a, float b, float c);

}  // namespace Math

void Math::StartupMathUtil() {
    unsigned int* row = binomTriangle;
    unsigned int* next = binomTriangle + 1;

    row[0] = 1;

    for (int n = 1; n < 32; n++) {
        next[n] = 1;
        next[0] = 1;

        const unsigned int* a = row;
        const unsigned int* b = row + 1;

        for (unsigned int* p = next + 1; p < next + n; p++) {
            *p = *a++ + *b++;
        }

        row = next;
        next += n + 1;
    }
}

void Math::DampSpring(float& x, float& v, float a, float b, float c) {
    float t = b * x + v;

    x = c * (t * a + x);
    v = t * c - b * x;
}
