// Material.cpp -- two functions, read from the image with
// tools/disasm.py: each hands `this` to the material depot's instance,
// as a tail call.

namespace Graphics {

class Material;

class MaterialDepot {
public:
    static MaterialDepot* g_instance;

    void AddMaterial(Material* material);
    void RemoveMaterial(Material* material);
};

class Material {
public:
    void RenderAttach();
    void RenderDetach();
};

}  // namespace Graphics

void Graphics::Material::RenderAttach() {
    MaterialDepot::g_instance->AddMaterial(this);
}

void Graphics::Material::RenderDetach() {
    MaterialDepot::g_instance->RemoveMaterial(this);
}
