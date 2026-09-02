// zEmbeddedStartupIcon.cpp -- two functions, read from the image with
// tools/disasm.py, the members from the DWARF: the constructor clears
// the five members, and StopCallback, when the icon is started, takes
// one off the class's active count and marks it stopped.

class zEmbeddedStartupIcon {
public:
    static int activeCount;

    zEmbeddedStartupIcon();
    void StopCallback();

    bool started;
    unsigned int xPos;
    unsigned int yPos;
    unsigned int width;
    unsigned int height;
};

zEmbeddedStartupIcon::zEmbeddedStartupIcon()
    : started(false), xPos(0), yPos(0), width(0), height(0) {}

void zEmbeddedStartupIcon::StopCallback() {
    if (!started) {
        return;
    }

    activeCount--;
    started = false;
}
