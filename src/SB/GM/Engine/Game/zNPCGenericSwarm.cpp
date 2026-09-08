// The accessor part below was written by tools/gen_accessors.py and
// is kept unchanged; the constructor at the foot was added by hand,
// so the generator's banner is gone -- gen_units.py overwrites any
// file that still carries it, and this one must not be overwritten.
//
// Every function here touches nothing but its own members or a
// constant: one load, one store, the address of a member, a
// constant return, or members set to one constant. Each body was
// decoded from the image and re-encoded back to the same bytes,
// and each parameter list re-mangled back to the same symbol,
// before being written. GENERATED, not read: real matched
// functions whose offsets are recovered fact, but a count of them
// is not a count of decompiled code.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.


namespace World { class EntityHandleBase; }

// The base carries the vptr the constructor stores at +0; how much
// of the 0x240 belongs to it and how much to the swarm changes no
// code, so all of it rides here.
class zNPCBase {
public:
    zNPCBase(World::EntityHandleBase* handle);

    virtual void _v0();
};

class zNPCGenericSwarm : public zNPCBase {
public:
    zNPCGenericSwarm(World::EntityHandleBase* handle);

    int GetNumberOfChildren();

    unsigned char _pad0[0x240 - 0x4];
    int f240;
};


int zNPCGenericSwarm::GetNumberOfChildren() { return f240; }

zNPCGenericSwarm::zNPCGenericSwarm(World::EntityHandleBase* handle)
    : zNPCBase(handle) {
    f240 = 0;
}
