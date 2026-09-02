// WAD00_8 -- one function, read from the image with tools/disasm.py:
// xResponseCurve::find_active_node walks the active-node cursor along a
// strided array of keys until the key at the cursor is at or below the
// value and the next key is at or above it, stopping at either end.
// The cursor is a member written from a const method, so it is
// mutable; the members are the DWARF's. The last index is declared
// before the node pointer and the product reads the stride first: the
// bytes give the index the lower register and the multiply that order.

class xResponseCurve {
public:
    void find_active_node(float value, unsigned long stride) const;

    unsigned int _values;
    float* curve;
    unsigned int _nodes;
    mutable unsigned int active_node;
};

void xResponseCurve::find_active_node(float value,
                                      unsigned long stride) const {
    unsigned int last = _nodes - 2;
    const char* node = (const char*)curve + stride * active_node;

    for (;;) {
        bool below = value < *(const float*)node;

        if (below) {
            if (active_node == 0) {
                return;
            }

            active_node--;
            node -= stride;
        } else {
            if (!(value > *(const float*)(node + stride))) {
                return;
            }

            if (active_node >= last) {
                return;
            }

            active_node++;
            node += stride;
        }
    }
}
