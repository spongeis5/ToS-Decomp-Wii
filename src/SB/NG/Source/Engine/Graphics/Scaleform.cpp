// C:/branches/SB09/main/NG/Source/Engine/Graphics/Scaleform.cpp
//
// Five one-line forwarders. `this` is never touched in any of them: the
// entity exists to give the rest of the engine a handle, and every call
// goes to the process-wide Coordinator.
//
// Scaleform::Coordinator::GetCoord is STATIC -- r3 is not set before the
// call and holds the result after it, so nothing is passed. StopAllMovies
// shows it cleanly: two `bl` in a row with no register moves between them.
//
// NOTE this is Engine/Graphics/Scaleform.cpp, NOT Game/zScaleform.cpp.
// The two live in different unity blobs and a suffix match on
// "Scaleform.cpp" returns both -- 26 functions and 14 KB instead of these
// five and 252 bytes.

namespace Scaleform {

class ViewNode;

class Coordinator {
public:
    static Coordinator* GetCoord();

    void InstantiateMovie(const char* filename);
    void UnloadMovie(const char* filename);
    void GraphicsPlayMovie(ViewNode* viewNode, const char* fn);
    void GraphicsStopMovie(ViewNode* viewNode);
    void GraphicsStopAllMovies();
};

}  // namespace Scaleform

namespace Graphics {

class ScaleformInternalEntity {
public:
    void LoadMovie(const char* filename);
    void UnloadMovie(const char* filename);
    void PlayMovie(Scaleform::ViewNode* viewNode, const char* fn);
    void StopMovie(Scaleform::ViewNode* viewNode);
    void StopAllMovies();
};

void ScaleformInternalEntity::LoadMovie(const char* filename) {
    Scaleform::Coordinator::GetCoord()->InstantiateMovie(filename);
}

void ScaleformInternalEntity::UnloadMovie(const char* filename) {
    Scaleform::Coordinator::GetCoord()->UnloadMovie(filename);
}

void ScaleformInternalEntity::PlayMovie(Scaleform::ViewNode* viewNode,
                                        const char* fn) {
    Scaleform::Coordinator::GetCoord()->GraphicsPlayMovie(viewNode, fn);
}

void ScaleformInternalEntity::StopMovie(Scaleform::ViewNode* viewNode) {
    Scaleform::Coordinator::GetCoord()->GraphicsStopMovie(viewNode);
}

void ScaleformInternalEntity::StopAllMovies() {
    Scaleform::Coordinator::GetCoord()->GraphicsStopAllMovies();
}

}  // namespace Graphics
