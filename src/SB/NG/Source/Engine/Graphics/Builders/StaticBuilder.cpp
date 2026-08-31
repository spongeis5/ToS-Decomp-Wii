// C:/branches/SB09/main/NG/Source/Engine/Graphics/Builders/StaticBuilder.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class StaticBuilder /* 0xE4 */ {
//       /* +0x0  */ Builder _base0;              // 0x10
//       /* +0x10 */ StaticGeometry geom;         // 0xB4
//       /* +0xC4 */ RenderAttrib viewAttrib;
//       /* +0xC8 */ FixedIndexBuffer* indexBuffers;
//       /* +0xCC */ FixedVertexBuffer* fixedVertexBuffers;
//       /* +0xD0 */ unsigned char* lightSectorIndices;
//       /* +0xD4 */ CollKDTree colltree;
//   };
//   class Geometry /* 0xB4 */ : Node { ... effect at +0x4C, indexLodCount
//       at +0x9C, vertexBufferCount at +0xA4 ... };
//   class Node /* 0xC */ { vptr; NodeTypeEnum type; bool attached; ... };
//
// geom is a MEMBER at +0x10, not a base, and that is what the addressing
// says. RenderAttach's `lwzu r12, 0x10(r3)` adjusts to &geom and loads its
// vptr in one instruction; RenderDetach reaches Node::Detach with an
// explicit `addi r3, r3, 0x10`. A base subobject would need neither.
//
// Only five of this unit's eight functions are here. Create takes eighteen
// parameters and CreateRenderable and SetBuffers reach further into the
// graphics types than has been recovered; the unit stays NonMatching for
// that, and dtk still counts the five.
//
// Everything is declared NON-VIRTUAL except the two Node slots the code
// actually dispatches through. CodeWarrior mangles virtual and non-virtual
// members identically, so the symbols are unaffected, and a virtual
// declaration whose key function landed here would emit a vtable this unit
// does not have.

extern "C" void* memset(void* dst, int c, unsigned long n);

namespace Graphics {

class Node {
public:
    // Attach is dispatched through the vtable at vptr+0xC, which is the
    // SECOND slot under CodeWarrior's two-word header. Slot one is declared
    // only to put Attach at the right index.
    virtual void Node_Slot0();
    virtual void Attach();

    void Detach();

    int type;
    bool attached;
    bool detachPending;
    bool updateThread;
};

class ParamFormatTable {
public:
    unsigned char _head[0x8];
    int size;
};

class Effect : public Node {
public:
    unsigned char _mid[0x28 - 0xC];
    ParamFormatTable params[3];
};

class Geometry : public Node {
public:
    unsigned char _mid[0x4C - 0xC];
    Effect* effect;
    unsigned char _mid2[0x9C - 0x50];
    int indexLodCount;
    unsigned char _mid3[0xA4 - 0xA0];
    int vertexBufferCount;
    unsigned char _tail[0xB4 - 0xA8];
};

class StaticGeometry : public Geometry {
};

class IndexBuffer {
public:
    // RenderDetach tests the FIRST WORD OF bufferObj, at +4 in the
    // FixedIndexBuffer -- not ownBufferSize at +0. One word apart, and the
    // only word that differed on the first attempt.
    int handle;
    unsigned char _tail[0x8];
};

class VertexBuffer {
public:
    unsigned char _head[0x8];
};

class FixedIndexBuffer {
public:
    void UnbindBuffer();

    int ownBufferSize;
    IndexBuffer bufferObj;
    unsigned short* buffer;
};

class FixedVertexBuffer {
public:
    void UnbindBuffer();

    int ownBufferSize;
    VertexBuffer bufferObj;
    void* buffer;
};

class Builder {
public:
    unsigned char _head[0x10];
};

class CollKDTree {
public:
    unsigned char _head[0x10];
};

class StaticBuilder : public Builder {
public:
    void RenderAttach();
    void RenderDetach();
    int GetCreateGeometrySize(int& count) const;
    int GetCreateRenderableSize(int& count) const;
    void SetBuffers();
    void UnbindBuffers();

    StaticGeometry geom;
    int viewAttrib;
    FixedIndexBuffer* indexBuffers;
    FixedVertexBuffer* fixedVertexBuffers;
    unsigned char* lightSectorIndices;
    CollKDTree colltree;
};

void StaticBuilder::RenderAttach() {
    geom.attached = true;

    geom.Attach();
}

void StaticBuilder::RenderDetach() {
    geom.Node::Detach();

    if (indexBuffers->bufferObj.handle != 0) {
        UnbindBuffers();
    }
}

int StaticBuilder::GetCreateGeometrySize(int& count) const {
    count = 1;

    return 0;
}

int StaticBuilder::GetCreateRenderableSize(int& count) const {
    count = 4;

    if (count < 16) {
        count = 16;
    }

    return ((geom.effect->params[2].size + 15) & ~15) + 256;
}

void StaticBuilder::UnbindBuffers() {
    // The counts are read ONCE, before each loop. Testing the member
    // directly reloads it every iteration -- the call in the body can alias
    // it -- which costs one instruction per loop and one fewer
    // callee-saved register.
    //
    // Two counts, not one: retail holds the first in r30 and the second in
    // r29, which one variable cannot do. And the LOOP INDEX is declared
    // LAST. Declared first it takes r31 and pushes the two counts down a
    // register each, which is the only thing four spellings of this
    // function differed by.
    int indexCount;
    int vertexCount;
    int i;

    indexCount = geom.indexLodCount;
    for (i = 0; i < indexCount; i++) {
        indexBuffers[i].UnbindBuffer();
    }

    vertexCount = geom.vertexBufferCount;
    for (i = 0; i < vertexCount; i++) {
        fixedVertexBuffers[i].UnbindBuffer();
    }
}

}  // namespace Graphics
