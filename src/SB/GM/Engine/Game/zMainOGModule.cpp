// The accessor part below was written by tools/gen_accessors.py and is
// kept unchanged apart from one thing; Graphics::Scene::AmendAddView at
// the foot was added by hand, so the generator's banner is gone --
// gen_units.py overwrites any file that still carries it.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.
//
// THE ONE THING: the generator wrote `class Graphics`, and this is a
// NAMESPACE. Both mangle the two accessors the same way, because
// CodeWarrior writes a single qualifier bare and only uses `Q<n>` for
// two or more -- `HackGetScreenView__8GraphicsFv` says nothing about
// which. Two qualifiers do say: AmendAddView is
// `Q28Graphics5Scene`, and Scene is a class in namespace Graphics
// everywhere else in the image, so a namespace is what this is. The two
// accessors still match after the change; that is the check that says
// the rename cost nothing.
//
// AmendAddView calls its own AddView through a POINTER TO MEMBER: the
// twelve-byte constant onto the stack, r12 pointed at it, __ptmf_scall.
// The constant is at 806BD8B4 and names the target -- delta 0, vtable
// offset -1, and a third word that is AddView's address at 801CF700.
// The View* passes through untouched in r4, which is why the constant is
// copied through r7/r6 here and through r6/r5 in the ones whose only
// arguments are floats.

namespace EngineOG { extern int MainScreenView; }
namespace EngineOG { extern float MonitorAspectRatio; }


namespace Graphics {

class View;

class Scene {
public:
    void AddView(View* view);
    void AmendAddView(View* view);
};

float HackGetMonitorAspectRatio();
int* HackGetScreenView();

}  // namespace Graphics


int* Graphics::HackGetScreenView() { return &EngineOG::MainScreenView; }
float Graphics::HackGetMonitorAspectRatio() { return EngineOG::MonitorAspectRatio; }

void Graphics::Scene::AmendAddView(View* view) {
    void (Scene::*add)(View*) = &Scene::AddView;

    (this->*add)(view);
}
