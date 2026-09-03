// WAD03_28 -- one function, read from the image with tools/disasm.py:
// FMOD_REVERB_PROPERTIES' copy assignment, thirty-one members loaded into
// thirty-one registers and then stored back. It is the COMPILER'S, not the
// original author's: the symbol is STB_WEAK in the retail image, which is
// what an implicitly declared copy assignment is and what an out-of-line
// member the source wrote would not be. So this file declares the layout
// and makes the compiler generate the operator, rather than spelling the
// body out.
//
// The LAYOUT is read out of the eighty-five instructions rather than taken
// from a header, because the type is FMOD's and not the game's, so the
// DWARF does not describe it. Every member's offset is the displacement of
// its load, and its TYPE is which register file the value moved through: a
// float member goes by `lfs`/`stfs`, an int by `lwz`/`stw` -- and a float
// ARRAY goes through GPRs too, one word an element with no loop, which is
// what says +0x30 and +0x44 are three-element pan vectors and not six
// separate floats. In order: two ints, two floats, three ints, three
// floats, one int, one float, a float[3], one int, one float, a float[3],
// ten floats and one unsigned int -- 124 bytes, and the names are the
// EAX-4 reverb set those offsets fit.
//
// Two shapes the bytes fixed.
//
// The copy is IMPLICIT. Written out member by member instead -- one
// assignment a line, the arrays element by element -- the object is 404
// bytes against retail's 340 and 101 words against 85: the six pan words
// move through f29-f31 and f23-f28 rather than through GPRs, which costs
// six more callee-saved FPRs to save and restore, and the function stops
// needing `_savegpr_27` at all.
//
// And an implicit operator is only emitted where something USES it. This
// fragment holds no statement that assigns the type -- retail's file did,
// elsewhere -- and an added function would be a function retail does not
// have, so the use here is a file-scope pointer to the member. That costs
// twelve bytes of `.data` the split does not own, which is why this unit
// matches and could not be linked as it stands.

class FMOD_REVERB_PROPERTIES {
public:
    /* +0x00 */ int Instance;
    /* +0x04 */ int Environment;
    /* +0x08 */ float EnvSize;
    /* +0x0C */ float EnvDiffusion;
    /* +0x10 */ int Room;
    /* +0x14 */ int RoomHF;
    /* +0x18 */ int RoomLF;
    /* +0x1C */ float DecayTime;
    /* +0x20 */ float DecayHFRatio;
    /* +0x24 */ float DecayLFRatio;
    /* +0x28 */ int Reflections;
    /* +0x2C */ float ReflectionsDelay;
    /* +0x30 */ float ReflectionsPan[3];
    /* +0x3C */ int Reverb;
    /* +0x40 */ float ReverbDelay;
    /* +0x44 */ float ReverbPan[3];
    /* +0x50 */ float EchoTime;
    /* +0x54 */ float EchoDepth;
    /* +0x58 */ float ModulationTime;
    /* +0x5C */ float ModulationDepth;
    /* +0x60 */ float AirAbsorptionHF;
    /* +0x64 */ float HFReference;
    /* +0x68 */ float LFReference;
    /* +0x6C */ float RoomRolloffFactor;
    /* +0x70 */ float Diffusion;
    /* +0x74 */ float Density;
    /* +0x78 */ unsigned int Flags;
};

// The use that makes the compiler emit the implicit operator.
typedef FMOD_REVERB_PROPERTIES& (FMOD_REVERB_PROPERTIES::*FMOD_REVERB_AsFn)(
    const FMOD_REVERB_PROPERTIES&);

static FMOD_REVERB_AsFn sReverbPropertiesAssign =
    &FMOD_REVERB_PROPERTIES::operator=;
