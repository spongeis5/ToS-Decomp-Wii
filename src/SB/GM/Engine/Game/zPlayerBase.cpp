// zPlayerBase.cpp -- five functions, read from the image with
// tools/disasm.py. They are the entity-callback shims the player entity
// installs: zPlayerUpdate keeps the scene on the player and runs its three
// update virtuals in order; zPlayerReset and zPlayerMove are bare tail
// calls into Reset and Move; zPlayer_StoreCheckPoint remembers the
// checkpoint's camera uid globally and hands every player in the container
// its own checkpoint; zPlayerSceneExit tells every player the scene is
// leaving.
//
// Layouts from the DWARF (tools/dwarf_types.py): zPlayer 0x480 with the
// scene pointer `sc` at +0x10C; xGlobals 0x4E8 with the zPlayerContainer
// at +0x428; zPlayerContainer 0x14, four zPlayer* then the count at +0x10
// -- which is the +0x428 and +0x438 the two loops read; zGlobals 0x5A0
// with zCheckPoint at +0x500, whose first member is the 8-byte camera uid
// the store writes; zPlayerCheckPoint 0x10, an xVec3 and a float, which is
// the 16-byte stride the checkpoint loop walks.
//
// The virtual SLOTS are recovered fact, not a guess: __vt__7zPlayer at
// 806C63F4 is 508 bytes, two leading zero words then the virtuals, so the
// offsets the five functions call -- 156, 164, 172, 180, 196, 204 and 264
// -- are indices 37, 39, 41, 43, 47, 49 and 64, and the table names them
// BeginUpdate, Update, EndUpdate, Move, Reset, Exit and StoreCheckPoint.
// The slots between them are declared and never defined, which is also
// what keeps zPlayer's vtable out of this object.
//
// Four shapes the bytes fixed.
//
// zPlayerUpdate keeps dt in f31 across the three calls and does NOT
// restore f1 for the first of them, because f1 still holds the incoming
// argument there -- so BeginUpdate takes dt like the other two, and a
// no-argument first call would leave the `fmr` out of the wrong place.
// `player` is a local (the DWARF puts it in r31, declared one line above
// the store), and the store of the scene reuses r3 rather than r31.
//
// zPlayerMove drops its last parameter: Move takes the scene, dt and the
// frame, which arrive in the registers the shim was called with, so
// nothing moves before the branch.
//
// Both loops read `xglobals` in the CONDITION and the body uses that same
// register -- the container is loaded once an iteration, at the top, and
// the call at the end of the body is what makes the next iteration reload
// it. Written with the count in a local, or the container hoisted above
// the loop, the load leaves the condition and the body grows one.
//
// The checkpoint loop carries two strides against one counter: four bytes
// for the player pointer and sixteen for the checkpoint, which is
// `checkPoints[i]` passed BY REFERENCE -- StoreCheckPoint's parameter is
// `const zPlayerCheckPoint&`, so the address is the argument and no copy
// is made.

class xEnt;
class zEnt;
class xScene;
class xEntFrame;
class xMat4x3;
class zScene;

class xVec3 {
public:
    float x;
    float y;
    float z;
};

class zPlayerCheckPoint {
public:
    xVec3 position;
    float rotation;
};

class zCheckPoint {
public:
    unsigned long long initCamID;
    bool* jsp_active;
};

class zPlayer {
public:
    virtual void _v0();   virtual void _v1();   virtual void _v2();
    virtual void _v3();   virtual void _v4();   virtual void _v5();
    virtual void _v6();   virtual void _v7();   virtual void _v8();
    virtual void _v9();   virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();
    virtual void _v15();  virtual void _v16();  virtual void _v17();
    virtual void _v18();  virtual void _v19();  virtual void _v20();
    virtual void _v21();  virtual void _v22();  virtual void _v23();
    virtual void _v24();  virtual void _v25();  virtual void _v26();
    virtual void _v27();  virtual void _v28();  virtual void _v29();
    virtual void _v30();  virtual void _v31();  virtual void _v32();
    virtual void _v33();  virtual void _v34();  virtual void _v35();
    virtual void _v36();
    virtual void BeginUpdate(float dt);                       // slot 37
    virtual void _v38();
    virtual void Update(float dt);                            // slot 39
    virtual void _v40();
    virtual void EndUpdate(float dt);                         // slot 41
    virtual void _v42();
    virtual void Move(xScene* scene, float dt, xEntFrame* frame);  // 43
    virtual void _v44();  virtual void _v45();  virtual void _v46();
    virtual void Reset();                                     // slot 47
    virtual void _v48();
    virtual void Exit();                                      // slot 49
    virtual void _v50();  virtual void _v51();  virtual void _v52();
    virtual void _v53();  virtual void _v54();  virtual void _v55();
    virtual void _v56();  virtual void _v57();  virtual void _v58();
    virtual void _v59();  virtual void _v60();  virtual void _v61();
    virtual void _v62();  virtual void _v63();
    virtual void StoreCheckPoint(const zPlayerCheckPoint& checkPoint);  // 64

    unsigned char _pad0[0x10C - 0x4];
    xScene* sc;
};

class zPlayerContainer {
public:
    zPlayer* playerArray[4];
    int numPlayers;
};

class xGlobals {
public:
    unsigned char _pad0[0x428];
    zPlayerContainer players;
};

class zGlobals {
public:
    unsigned char _pad0[0x500];
    zCheckPoint checkPoint;
};

extern xGlobals* xglobals;
extern zGlobals globals;

void zPlayerUpdate(xEnt* ent, xScene* scene, float dt) {
    zPlayer* player = (zPlayer*)ent;

    player->sc = scene;

    player->BeginUpdate(dt);
    player->Update(dt);
    player->EndUpdate(dt);
}

void zPlayerReset(zEnt* ent) {
    ((zPlayer*)ent)->Reset();
}

void zPlayerMove(xEnt* ent, xScene* scene, float dt, xEntFrame* frame,
                 xMat4x3* mat) {
    ((zPlayer*)ent)->Move(scene, dt, frame);
}

void zPlayer_StoreCheckPoint(const zPlayerCheckPoint* checkPoints,
                             unsigned long long initCamID) {
    globals.checkPoint.initCamID = initCamID;

    for (int i = 0; i < xglobals->players.numPlayers; i++) {
        xglobals->players.playerArray[i]->StoreCheckPoint(checkPoints[i]);
    }
}

void zPlayerSceneExit() {
    for (int i = 0; i < xglobals->players.numPlayers; i++) {
        xglobals->players.playerArray[i]->Exit();
    }
}
