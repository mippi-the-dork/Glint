#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions_Base.h"
#include "GlintLensProfile.h"
#include "GlintFlare.h"

class FGlintAssetActions final : public FAssetTypeActions_Base
{
public:
    FGlintAssetActions(UClass* InClass, FText InName, EAssetTypeCategories::Type InCategory)
        : Class(InClass), Name(MoveTemp(InName)), Category(InCategory) {}
    FText GetName() const override { return Name; }
    FColor GetTypeColor() const override { return FColor(45, 148, 238); }
    UClass* GetSupportedClass() const override { return Class; }
    uint32 GetCategories() override { return Category; }
private:
    UClass* Class;
    FText Name;
    EAssetTypeCategories::Type Category;
};

class FGlintEditorModule final : public IModuleInterface
{
public:
    void StartupModule() override
    {
        IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
        const EAssetTypeCategories::Type Category = AssetTools.RegisterAdvancedAssetCategory(
            FName(TEXT("Glint")), FText::FromString(TEXT("Glint")));
        TSharedRef<IAssetTypeActions> Lens = MakeShared<FGlintAssetActions>(
            UGlintLensProfile::StaticClass(), FText::FromString(TEXT("Glint Lens")), Category);
        TSharedRef<IAssetTypeActions> Flare = MakeShared<FGlintAssetActions>(
            UGlintFlare::StaticClass(), FText::FromString(TEXT("Glint Flare")), Category);
        AssetTools.RegisterAssetTypeActions(Lens);
        AssetTools.RegisterAssetTypeActions(Flare);
        Registered.Add(Lens);
        Registered.Add(Flare);
    }
    void ShutdownModule() override
    {
        if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
        {
            IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
            for (const TSharedRef<IAssetTypeActions>& Action : Registered)
            {
                AssetTools.UnregisterAssetTypeActions(Action);
            }
        }
        Registered.Empty();
    }
private:
    TArray<TSharedRef<IAssetTypeActions>> Registered;
};
IMPLEMENT_MODULE(FGlintEditorModule, GlintEditor)
