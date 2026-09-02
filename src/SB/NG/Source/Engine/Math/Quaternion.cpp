// Quaternion.cpp -- one function, read from the image with
// tools/disasm.py: Debug::QuaternionFormatter::Format prints the four
// components with snprintf, or does nothing without data. The format
// string is a POOLED string of the unity build this file is a fragment
// of, so the generated header puts the whole pool in front and the
// baked-in offset (+35) comes out as retail has it.
#include "SB/NG/Source/Engine/Math/Quaternion.pool.h"

extern "C" int snprintf(char* buffer, unsigned long size, const char* format,
                        ...);

namespace Debug {

class QuaternionFormatter {
public:
    void Format(char* buffer, int size, void* data, char* format, int arg);
};

}  // namespace Debug

void Debug::QuaternionFormatter::Format(char* buffer, int size, void* data,
                                        char* format, int arg) {
    float* q = (float*)data;

    if (data == 0) {
        return;
    }

    snprintf(buffer, size, "%f : %f : %f : %f", q[0], q[1], q[2], q[3]);
}
