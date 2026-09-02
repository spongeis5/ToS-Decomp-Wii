// WAD02_9 -- one function, read from the image with tools/disasm.py:
// Memory::DeptLookup's copy assignment, four words loaded and then
// stored, the way the compiler copies a sixteen-byte object. The
// member names are not in these nine instructions.

namespace Memory {

class DeptLookup {
public:
    DeptLookup& operator=(const DeptLookup& o);

    int f0;
    int f4;
    int f8;
    int fC;
};

}  // namespace Memory

Memory::DeptLookup& Memory::DeptLookup::operator=(const DeptLookup& o) {
    f0 = o.f0;
    f4 = o.f4;
    f8 = o.f8;
    fC = o.fC;

    return *this;
}
