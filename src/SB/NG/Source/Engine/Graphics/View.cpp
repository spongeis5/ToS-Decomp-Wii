// View.cpp -- four functions, read from the image with tools/disasm.py.
// A view is a render-target client: Create constructs its two lists in
// place; SetViewport hands the viewport to the target's own virtual and
// records it as the display's active one; the screen view's Start stores
// the target and tail-calls the target's begin slot; Finish calls the
// target's end slot, and then -- when a movie is queued and it is not
// full screen -- walks the movie queue, marking the last entry, giving
// each its surface index and running its end callback, before clearing
// the active viewport and the target.
//
// Layouts from the DWARF (tools/dwarf_types.py): Graphics::Node 0xC with
// the vptr at 0 and the type at +4; View 0x28 on Node, hackRenderOrder
// at +0xC, the two VoidLists at +0x10 and +0x18, ready at +0x20 and the
// target at +0x24; ScreenView 0x28, adding nothing; RenderTarget 0x14 on
// Node; Util::VoidList 0x8; MovieData 0x50 with the callback at +0x10,
// the surface index at +0x34 and the `last` flag at +0x4A; the movie
// queue is the DWARF's CircularQueue<MovieData*> -- start, size, mask,
// pool -- and only its size is read here.
//
// The VTABLE SLOTS are read off the retail tables, not guessed.
// __vt__Q28Graphics4Node is 20 bytes, so Node has three virtuals (0..2)
// and every derived slot starts at 3. __vt__Q28Graphics12RenderTarget is
// 36 bytes and holds SetViewport in slot 5, which is the +0x1C this file
// calls; slots 4 and 6 are zero there -- pure in the base -- and they are
// the +0x18 Start tail-calls and the +0x20 Finish calls.
// __vt__Q28Graphics4View is 32 bytes: slot 3 zero, slot 4 View::
// SetViewport, slot 5 zero, so View declares Start, SetViewport and
// Finish in that order and ScreenView's table fills 3 and 5 with the two
// functions here. Start is declared ahead of SetViewport and left
// undefined so View's vtable home is not this unit, and ScreenView's
// first virtual is declared and undefined for the same reason.
//
// Four shapes the bytes fixed. Create's two constructor calls carry no
// null test, so they are calls by the MANGLED NAME rather than placement
// new, the way zNPCStatus and BeginUpdate reach a constructor. The movie
// count is read once into a SIGNED local -- the queue's own size is
// unsigned in the DWARF and both loop compares are `cmpw`. The callback
// is a data member at +0x10, one load and a `bctrl`, not a virtual call,
// which would take two. And the `last` flag is an IF/ELSE, not an
// assignment of the comparison: retail hoists 1 and 0 into r30 and r31
// ahead of the loop and stores one of them from either arm, five words,
// where `movie->last = i == count - 1;`, the same in a ternary, and the
// same through a local bool all fold to a branchless `subf`/`cntlzw`/
// `srwi` -- three words, and 33 of 44 wrong once the frame shrinks with
// them. Writing the arms the other way round (`!=` first) costs three
// words; the two spellings that put `true` in the first arm are exact.

extern "C" {
// Util::VoidList::VoidList(). Called by name: retail constructs both
// lists in place with no null test, and placement new adds one.
void __ct__Q24Util8VoidListFv(void* list);
}

namespace Util {

class NodeHeader {
public:
    NodeHeader* prev;
    NodeHeader* next;
};

class NodeListBase {
public:
    NodeHeader tail;
};

class VoidList : public NodeListBase {};

}  // namespace Util

enum movieEndCode { movieEndCode_ = 0x7FFFFFFF };

class D3DRECT {
public:
    long x1;
    long y1;
    long x2;
    long y2;
};

class MovieData {
public:
    char movie[12];
    void* hBink;
    movieEndCode (*cbFn)();
    D3DRECT rect;
    D3DRECT dstRect;
    int surfaceIndex;
    int startFrameSmall;
    int startCountdown;
    unsigned long long subtitlesID;
    bool fullscreen;
    bool skippable;
    bool last;
    bool (*delaycbFn)();
};

// The DWARF's CircularQueue<MovieData*>.
class MovieQueue {
public:
    unsigned int start;
    unsigned int size;
    unsigned int mask;
    MovieData** pool;
};

namespace Graphics {

class Scene;
class Viewport;

class Node {
public:
    enum NodeTypeEnum { NodeTypeEnum_ = 0x7FFFFFFF };

    virtual void _v0();
    virtual void _v1();
    virtual void _v2();

    NodeTypeEnum type;
    bool attached;
    bool detachPending;
    bool updateThread;
};

class RenderTarget : public Node {
public:
    virtual void _v3();
    virtual void Begin();
    virtual void SetViewport(Viewport* viewport);
    virtual void End();

    int width;
    int height;
};

namespace Display {
extern Viewport* activeViewport;
}  // namespace Display

bool HasMovie();
bool IsMovieFullScreen();
MovieData* GetMovie(int index);

extern MovieQueue movieQueue;

class View : public Node {
public:
    void Create();

    // Declared ahead of SetViewport and undefined: View's vtable home is
    // not this unit, and the three fill slots 3, 4 and 5.
    virtual void Start(Scene& scene, RenderTarget* target);
    virtual void SetViewport(Viewport* viewport);
    virtual void Finish();

    int hackRenderOrder;
    Util::VoidList spaces;
    Util::VoidList viewports;
    bool ready;
    RenderTarget* target;
};

class ScreenView : public View {
public:
    // Declared first and undefined, so ScreenView's vtable stays where
    // retail has it.
    virtual void _v0();

    virtual void Start(Scene& scene, RenderTarget* target);
    virtual void Finish();
};

}  // namespace Graphics

void Graphics::View::Create() {
    __ct__Q24Util8VoidListFv(&spaces);
    __ct__Q24Util8VoidListFv(&viewports);
}

void Graphics::View::SetViewport(Viewport* viewport) {
    target->SetViewport(viewport);

    Display::activeViewport = viewport;
}

void Graphics::ScreenView::Start(Scene& scene, RenderTarget* target) {
    this->target = target;

    target->Begin();
}

void Graphics::ScreenView::Finish() {
    target->End();

    if (HasMovie() && !IsMovieFullScreen()) {
        int count = movieQueue.size;

        for (int i = 0; i < count; i++) {
            MovieData* movie = GetMovie(i);

            if (i == count - 1) {
                movie->last = true;
            } else {
                movie->last = false;
            }

            movie->surfaceIndex = i;

            movie->cbFn();
        }
    }

    Display::activeViewport = 0;
    target = 0;
}
