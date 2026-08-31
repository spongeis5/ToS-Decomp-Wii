// C:/branches/SB09/main/GM/Engine/Core/x/xTextAsset.cpp
//
// Two overloads, one word each: a bare tail call to the NGStrings overload
// of the same shape.
//
//   80046E00  b  NGStrings::FindString(unsigned int, unsigned int*)
//   80046E10  b  NGStrings::FindString(const char*, unsigned int*)

class NGStrings {
public:
    static bool FindString(unsigned int id, unsigned int* out);
    static bool FindString(const char* name, unsigned int* out);
};

bool xTextFindString(unsigned int id, unsigned int* out) {
    return NGStrings::FindString(id, out);
}

bool xTextFindString(const char* name, unsigned int* out) {
    return NGStrings::FindString(name, out);
}
