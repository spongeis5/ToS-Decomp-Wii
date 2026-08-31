// C:/branches/SB09/main/GM/Engine/Game/zCameraCurveAsset.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class zCameraCurveAsset /* 0x78 */ { CurveCamera _base0; };
//   class CurveCamera       /* 0x78 */ {
//       ... /* +0x28 */ unsigned char NumCurvesData[0x28];  // anonymous
//   };
//   enum eCamCurve { eCamCurve_Player = 0, eCamCurve_Camera = 1,
//                    eCamCurve_NumCurve = 2 };
//
// The two ids live inside that anonymous 0x28-byte block, at +0x28 and
// +0x30. DWARF does not describe it, so the block is spelled as padding and
// the two ids as what the loads say they are: adjacent word pairs, i.e. a
// 64-bit value returned in r3:r4.
//
// It is a BRANCH, not an indexed load -- `CurveIDs[which]` would emit a
// shift and an `lwzx`. And `mr r4, r3` has to come before the first load
// because the id's high word lands in r4, which is still holding the
// argument.

typedef unsigned long long uid;

enum eCamCurve {
    eCamCurve_Player = 0,
    eCamCurve_Camera = 1,
    eCamCurve_NumCurve = 2
};

class zCameraCurveAsset {
public:
    uid GetCurveID(eCamCurve which);

    unsigned char _head[0x28];
    uid PlayerCurveID;
    uid CameraCurveID;
};

uid zCameraCurveAsset::GetCurveID(eCamCurve which) {
    if (which == eCamCurve_Player) {
        return PlayerCurveID;
    }

    return CameraCurveID;
}
