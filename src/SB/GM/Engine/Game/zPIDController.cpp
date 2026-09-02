// zPIDController.cpp -- five functions, read from the image with
// tools/disasm.py. A PID controller over a ring of sixteen errors and
// time steps: Reset stores the three gains, clears both rings, sets
// the ring position to -1 and the estimated destination speed to
// zero; Update computes the error, the proportional term, the
// integral term over the ring (when kI is set and the summed time is
// above 1e-5), the derivative term against the last error (when kD is
// set, there is a last error, and the step is above 1e-5), adds the
// estimated speed, advances the ring and records the error and step.
// The vector controller does the same over xVec3s with the vector
// helpers, its estimated speed reset from xVec3::m_Null through the
// out-of-line assignment. SetEstimatedDestSpeed stores the speed.
//
// Layouts from the DWARF (tools/dwarf_types.py): zPIDController 0x94
// (errors, steps, position, kP, kI, kD, speed), zPIDController3 0x11C
// with xVec3 errors and speed. The three literals (1.0, 1e-5, 0.0) sit
// past 32 KB of the unity unit's .rodata, hence the generated header
// first; the unit has no data of its own.
//
// Three orderings the bytes fixed: the integral is declared and
// accumulated before the summed step (f4 before f6), the integral term
// is `(integral * kI) * (1 / sum)`, and the derivative quotient is a
// named local, or the fused multiply-subtract takes kD first.

#include "SB/GM/Engine/Game/zPIDController.pool.h"

extern "C" void* memset(void* dst, int c, unsigned long n);

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    xVec3& operator*=(float s);
    xVec3& operator+=(const xVec3& v);

    void Sub(const xVec3& a, const xVec3& b);
    void Scale(const xVec3& v, float s);
    void AddScale(const xVec3& v, float s);

    float x;
    float y;
    float z;

    static xVec3 m_Null;
};

class zPIDController {
public:
    void Reset(float p, float i, float d);
    void SetEstimatedDestSpeed(float speed);
    void Update(float& result, float current, float target, float dt);

    float errorsArray[16];
    float dtArray[16];
    int lastPos;
    float kP;
    float kI;
    float kD;
    float estimatedDestSpeed;
};

class zPIDController3 {
public:
    void Reset(float p, float i, float d);
    void Update(xVec3& result, const xVec3* current, const xVec3* target,
                float dt);

    xVec3 errorsArray[16];
    float dtArray[16];
    int lastPos;
    float kP;
    float kI;
    float kD;
    xVec3 estimatedDestSpeed;
};

void zPIDController::Reset(float p, float i, float d) {
    kP = p;
    kI = i;
    kD = d;

    memset(errorsArray, 0, sizeof(errorsArray));
    memset(dtArray, 0, sizeof(dtArray));

    lastPos = -1;
    estimatedDestSpeed = 0.0f;
}

void zPIDController::SetEstimatedDestSpeed(float speed) {
    estimatedDestSpeed = speed;
}

void zPIDController::Update(float& result, float current, float target,
                            float dt) {
    float error = target - current;

    result = error * -kP;

    if (kI > 0.0f) {
        float integral = error * dt;
        float dtSum = dt;

        for (int i = 0; i < 16; i++) {
            integral += errorsArray[i] * dtArray[i];
            dtSum += dtArray[i];
        }

        if (dtSum > 1e-5f) {
            result -= (integral * kI) * (1.0f / dtSum);
        }
    }

    if (kD > 0.0f && lastPos >= 0 && dt > 1e-5f) {
        float derivative = (error - errorsArray[lastPos]) / dt;

        result -= derivative * kD;
    }

    result += estimatedDestSpeed;

    lastPos++;

    if (lastPos == 16) {
        lastPos = 0;
    }

    errorsArray[lastPos] = error;
    dtArray[lastPos] = dt;
}

void zPIDController3::Reset(float p, float i, float d) {
    kP = p;
    kI = i;
    kD = d;

    memset(errorsArray, 0, sizeof(errorsArray));
    memset(dtArray, 0, sizeof(dtArray));

    lastPos = -1;
    estimatedDestSpeed = xVec3::m_Null;
}

void zPIDController3::Update(xVec3& result, const xVec3* current,
                             const xVec3* target, float dt) {
    xVec3 error;

    error.Sub(*target, *current);
    result.Scale(error, -kP);

    if (kI > 0.0f) {
        xVec3 integral;

        integral.Scale(error, dt);

        float dtSum = dt;

        for (unsigned int i = 0; i < 16; i++) {
            integral.AddScale(errorsArray[i], dtArray[i]);
            dtSum += dtArray[i];
        }

        if (dtSum > 1e-5f) {
            result.AddScale(integral, -kI * (1.0f / dtSum));
        }
    }

    if (kD > 0.0f && lastPos >= 0 && dt > 1e-5f) {
        xVec3 derivative;

        derivative.Sub(error, errorsArray[lastPos]);
        derivative *= 1.0f / dt;
        result.AddScale(derivative, -kD);
    }

    result += estimatedDestSpeed;

    lastPos++;

    if (lastPos == 16) {
        lastPos = 0;
    }

    errorsArray[lastPos] = error;
    dtArray[lastPos] = dt;
}
