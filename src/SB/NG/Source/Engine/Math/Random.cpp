// Random.cpp -- one function, read from the image with tools/disasm.py:
// Math::RandomTwist::MakeSeed is the Mersenne Twister's seeding, the
// reference 1812433253 recurrence over the 624-word state, with the
// index set to 624 at the end so the first draw regenerates. The seed
// type is the index followed by the state. Static: r3 is the seed and
// r4 the value, with no `this` in front of them.

namespace Math {

class RandomTwist {
public:
    class SeedType {
    public:
        int index;
        unsigned int state[624];
    };

    static void MakeSeed(SeedType& seed, unsigned int value);
};

}  // namespace Math

void Math::RandomTwist::MakeSeed(SeedType& seed, unsigned int value) {
    int i;

    seed.state[0] = value;

    for (i = 1; i < 624; i++) {
        seed.state[i] =
            1812433253 * (seed.state[i - 1] ^ (seed.state[i - 1] >> 30)) + i;
    }

    seed.index = 624;
}
