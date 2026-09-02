// WAD00_5_2 -- one function, read from the image with tools/disasm.py:
// World::RawBlobEntity::RemoveOwner unlinks an owner from the entity's
// doubly linked owner list: the head at +0x20 moves on when it is the
// one removed, then the neighbours are joined. Owner's links are at
// +4 (previous) and +8 (next); nothing else of either type is in
// these sixteen instructions.

namespace World {

class RawBlobEntity {
public:
    class Owner {
    public:
        unsigned char _pad0[0x4];
        Owner* prev;
        Owner* next;
    };

    void RemoveOwner(Owner* owner);

    unsigned char _pad0[0x20];
    Owner* owners;
};

}  // namespace World

void World::RawBlobEntity::RemoveOwner(Owner* owner) {
    if (owners == owner) {
        owners = owner->next;
    }

    if (owner->prev) {
        owner->prev->next = owner->next;
    }

    if (owner->next) {
        owner->next->prev = owner->prev;
    }
}
