// LinkFastSqrt.cpp -- two functions, read from the image with
// tools/disasm.py: the game's own sqrtf and sqrt, a reciprocal square
// root estimate refined by Newton-Raphson -- once for the float, twice
// for the double -- guarded with a select so a negative estimate falls
// back to the argument, and multiplied back up by the argument. The
// 0.5 constants are the ones the image holds (@20 float, @22 double).
//
// Hand-fused: the estimate, the Newton step and the select are asm
// statements on register variables, because the intrinsics return
// double and the cast to float is an frsp retail does not have, and
// because the C spelling of the step is forwarded into a temporary
// that lands in the lowest free register where retail writes it back
// into t. Register variables take FPRs in declaration order, so the
// half is declared first and lands in f0, then e in f2, t in f3, h in
// f4 -- the assignment retail has. NOTES.md has the seventeen probes.

extern "C" {

float sqrtf(register float x) {
    register float half = 0.5f;
    register float e;
    register float t;
    register float h;

    asm { frsqrte e, x }
    h = half * x;
    t = e * e;
    asm { fnmsubs t, t, h, half }
    e = e * t + e;
    asm { fsel e, e, e, x }
    e = e * x;

    return e;
}

double sqrt(register double x) {
    register double half = 0.5;
    register double e;
    register double t;
    register double h;

    asm { frsqrte e, x }
    h = half * x;
    t = e * e;
    asm { fnmsub t, t, h, half }
    e = e * t + e;
    t = e * e;
    asm { fnmsub t, t, h, half }
    e = e * t + e;
    asm { fsel e, e, e, x }
    e = e * x;

    return e;
}

}
