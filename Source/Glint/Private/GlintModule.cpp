#include "Modules/ModuleManager.h"
#include "GlintSceneViewExtension.h"
#include "ShaderCore.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "SceneViewExtension.h"
#include "Engine/Engine.h"
#include "Misc/CoreDelegates.h"

class FGlintModule final : public IModuleInterface
{
public:
    void StartupModule() override
    {
        const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Glint"));
        if (Plugin.IsValid())
        {
            AddShaderSourceDirectoryMapping(TEXT("/Plugin/Glint"),
                FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders")));
        }
        // Shader registration must happen early; the view extension needs GEngine.
        if (GEngine) RegisterViewExtension();
        else EngineInitHandle = FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &FGlintModule::RegisterViewExtension);
    }
    void ShutdownModule() override
    {
        if (EngineInitHandle.IsValid())
        {
            FCoreDelegates::GetOnPostEngineInit().Remove(EngineInitHandle);
            EngineInitHandle.Reset();
        }
        Extension.Reset();
    }

    void RegisterViewExtension()
    {
        if (!Extension.IsValid() && GEngine)
        {
            Extension = FSceneViewExtensions::NewExtension<FGlintSceneViewExtension>();
        }
    }
private:
    TSharedPtr<FGlintSceneViewExtension, ESPMode::ThreadSafe> Extension;
    FDelegateHandle EngineInitHandle;
};
IMPLEMENT_MODULE(FGlintModule, Glint)
