// zNPCStatus.cpp -- one function, read from the image with
// tools/disasm.py: ResetToNPCAsset rebuilds the last position from the
// asset's three floats at +0x3C and the last orientation from the three
// at +0x30, each by calling Math::Vector's three-float constructor on
// the member with no null test. Placement new adds the test and this
// compiler rejects the explicit constructor-call syntax (error 10409),
// so the constructor is called by its retail symbol. Layout from the
// DWARF (zNPCStatus 0x1C: two xVec3 and the status).

extern "C" void __ct__Q24Math6VectorFfff(void* vector, float x, float y,
                                         float z);

enum eNPCStatus { eNPCStatus_ = 0x7FFFFFFF };

class xVec3 {
public:
    float x;
    float y;
    float z;
};

namespace Sext {

class NPCAsset {
public:
    unsigned char _pad0[0x30];
    float orientation[3];
    float position[3];
};

}  // namespace Sext

class zNPCStatus {
public:
    void ResetToNPCAsset(const Sext::NPCAsset* asset);

    xVec3 lastPos;
    xVec3 lastOrientation;
    eNPCStatus lastStatus;
};

void zNPCStatus::ResetToNPCAsset(const Sext::NPCAsset* asset) {
    __ct__Q24Math6VectorFfff(&lastPos, asset->position[0], asset->position[1],
                             asset->position[2]);
    __ct__Q24Math6VectorFfff(&lastOrientation, asset->orientation[0],
                             asset->orientation[1], asset->orientation[2]);
}
