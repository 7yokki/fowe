#include "third_party/The-Forge/Common_3/Application/Interfaces/IApp.h"
#include "third_party/The-Forge/Common_3/Application/Interfaces/IUI.h"
#include <cstdint>

enum class QualityTier { Low, Medium, High, Ultra };

class FOWEApp : public IApp {
public:
    bool Init() override { mTier = QualityTier::High; return true; }
    void Exit() override {}
    bool Load(ReloadDesc*) override { return true; }
    void Unload(ReloadDesc*) override {}
    void Update(float) override {}
    void Draw() override {
        // ImGui editor window + quality tier selector
        // (Forge UI calls omitted for skeleton; wired in build)
    }
    const char* GetName() override { return "Furious Open World Engine"; }
private:
    QualityTier mTier = QualityTier::High;
};

DEFINE_APPLICATION_MAIN(FOWEApp)
