// WAD04_6_1 -- one function, read from the image with tools/disasm.py.
//
// FoundPath: a member at +0x28 tested for non-zero and returned as a
// bool (addic/subfe). What the member is, beyond a word, is not in
// these four instructions; the name is the DWARF's for the class.

class zSearchStrategyAStar {
public:
    bool FoundPath() const;

    unsigned char _pad0[0x28];
    int f28;
};

bool zSearchStrategyAStar::FoundPath() const { return f28 != 0; }
