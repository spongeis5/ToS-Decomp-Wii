// zSoundReverb -- ReverbManager's singleton accessor and its constructor.
//
// HAND-WRITTEN, and it replaces a generated file: gen_accessors owned this
// unit for `Instance` alone, and a unit cannot be half generated. The
// accessor keeps its bytes and is spelled the way the image reads rather
// than the way the generator had to -- a singleton accessor is static and
// hands back its own type. Neither changes a byte, because CodeWarrior
// mangles neither a return type nor `static`.
//
// Read from the image with tools/disasm.py:
//
//   lis r3,0x8076 ; addi r3,r3,-25200 ; blr    &reverbMgrInstance
//
//   stw r0(=0),  2356(r3)                      f934 = 0
//   stfs f0,     2360(r3)                      f938 = the pool literal at
//   stfs f0,     2364(r3)                      f93C =   8068D2B8, which is
//                                                       0x00000000 -- 0.0f
//   addi r3,r3,2232 ; bl operator=             this+0x8B8 = the static
//   mr r4,r3 ; addi r3,r31,2108 ; bl           this+0x83C = that
//   mr r4,r3 ; addi r3,r31,1984 ; bl           this+0x7C0 = that
//
// THE THREE ARE AN ARRAY, and the symbol table says so rather than the
// spacing alone: REVERB_PROPERTIES_OFF__13ReverbManager sits at 806C22A8
// and the next symbol at 806C2324, so the type is 0x7C bytes -- exactly
// the 0x8B8 - 0x83C and 0x83C - 0x7C0 stride. Three of them from 0x7C0
// end at 0x934, which is where the int is.
//
// Each call takes the PREVIOUS call's return value as its argument, which
// is a chained assignment and not three statements: `operator=` hands back
// the object it wrote and retail feeds that straight into the next call.

class FMOD_REVERB_PROPERTIES {
public:
    FMOD_REVERB_PROPERTIES& operator=(const FMOD_REVERB_PROPERTIES& rhs);

    unsigned char _pad[0x7C];
};

class ReverbManager {
public:
    ReverbManager();
    static ReverbManager* Instance();

    static FMOD_REVERB_PROPERTIES REVERB_PROPERTIES_OFF;

    unsigned char _pad0[0x7C0];
    FMOD_REVERB_PROPERTIES properties[3];
    int f934;
    float f938;
    float f93C;
};

extern ReverbManager reverbMgrInstance;

ReverbManager* ReverbManager::Instance() { return &reverbMgrInstance; }

ReverbManager::ReverbManager() {
    f934 = 0;
    f938 = 0.0f;
    f93C = 0.0f;

    properties[0] = properties[1] = properties[2] = REVERB_PROPERTIES_OFF;
}
