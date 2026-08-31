// C:/branches/SB09/main/GM/Engine/Core/x/xOGEntity.cpp
//
// Six one-line forwarders at the head of WAD04. Names, signatures and
// declaration lines are the Wii build's own DWARF; the bodies are
// reconstructed from 88 bytes of .text.
//
//   80189AF0  DriveDetach       line 18   b    zNeoDrivenLink::RemoveChild
//   80189B00  DriveOn           line 19   li r4,1 ; b  ...::DriveStatus
//   80189B10  DriveOff          line 20   li r4,0 ; b  ...::DriveStatus
//   80189B20  DriveReset        line 21   b    zNeoDrivenLink::DriveReset
//   80189B30  DriveMoved        line 23   li r4,1 ; b  ...::Moved
//   80189B40  DriveMovedNoPass  line 24   li r4,0 ; b  ...::Moved
//
// `this` reaches every target in r3 UNCHANGED and the bool arrives in r4,
// which is what says the zNeoDrivenLink members are STATIC -- a non-static
// one would take its own `this` in r3 and push the entity to r4.

namespace World {
class xOGEntity;
}

class zNeoDrivenLink {
public:
    static void RemoveChild(World::xOGEntity* ent);
    static void DriveStatus(World::xOGEntity* ent, bool on);
    static void DriveReset(World::xOGEntity* ent);
    static void Moved(World::xOGEntity* ent, bool pass);
};

namespace World {

// NOT declared `virtual` here, though they are overrides in the real
// header. A class with virtual functions makes mwcc emit `xOGEntity::__vt`
// in whichever object defines its key function, and this file is a SPLIT
// out of a unity build whose remainder already emits it:
//
//   multiply-defined: 'World::xOGEntity::__vt' in WAD04.o
//   Previously defined in xOGEntity.o
//
// The bodies are unaffected -- every one of them uses `this` unadjusted --
// so the six functions still assemble to the retail bytes. When the rest of
// WAD04 is split out, this becomes a real header include and the vtable
// goes wherever the key function does.
class xOGEntity {
public:
    void DriveDetach();
    void DriveOn();
    void DriveOff();
    void DriveReset();
    void DriveMoved();
    void DriveMovedNoPass();
};

void xOGEntity::DriveDetach() {
    zNeoDrivenLink::RemoveChild(this);
}

void xOGEntity::DriveOn() {
    zNeoDrivenLink::DriveStatus(this, true);
}

void xOGEntity::DriveOff() {
    zNeoDrivenLink::DriveStatus(this, false);
}

void xOGEntity::DriveReset() {
    zNeoDrivenLink::DriveReset(this);
}

void xOGEntity::DriveMoved() {
    zNeoDrivenLink::Moved(this, true);
}

void xOGEntity::DriveMovedNoPass() {
    zNeoDrivenLink::Moved(this, false);
}

}  // namespace World
