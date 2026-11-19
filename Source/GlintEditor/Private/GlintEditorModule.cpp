#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions_Base.h"
#include "GlintLensProfile.h"
#include "GlintFlare.h"
#include "GlintFlareEditorToolkit.h"
#include "GlintEditorStyle.h"
#include "GlintLensDetailsCustomization.h"
#include "PropertyEditorModule.h"

class FGlintAssetActions : public FAssetTypeActions_Base
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

class FGlintFlareActions final : public FGlintAssetActions
{
public:
    explicit FGlintFlareActions(EAssetTypeCategories::Type Category)
        : FGlintAssetActions(UGlintFlare::StaticClass(), FText::FromString(TEXT("Glint Flare")), Category) {}

    void OpenAssetEditor(const TArray<UObject*>& Objects,
        TSharedPtr<IToolkitHost> Host) override
    {
        for (UObject* Object : Objects)
        {
            if (UGlintFlare* Flare = Cast<UGlintFlare>(Object))
            {
                TSharedRef<FGlintFlareEditorToolkit> Toolkit = MakeShared<FGlintFlareEditorToolkit>();
                Toolkit->Open(Flare, Host);
            }
        }
    }
};

class FGlintEditorModule final : public IModuleInterface
{
public:
    void StartupModule() override
    {
        FGlintEditorStyle::Initialize();
        FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
        PropertyEditor.RegisterCustomClassLayout(UGlintLensProfile::StaticClass()->GetFName(),
            FOnGetDetailCustomizationInstance::CreateStatic(&FGlintLensDetailsCustomization::MakeInstance));
        PropertyEditor.NotifyCustomizationModuleChanged();
        IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
        const EAssetTypeCategories::Type Category = AssetTools.RegisterAdvancedAssetCategory(
            FName(TEXT("Glint")), FText::FromString(TEXT("Glint")));
        TSharedRef<IAssetTypeActions> Lens = MakeShared<FGlintAssetActions>(
            UGlintLensProfile::StaticClass(), FText::FromString(TEXT("Glint Lens")), Category);
        TSharedRef<IAssetTypeActions> Flare = MakeShared<FGlintFlareActions>(Category);
        AssetTools.RegisterAssetTypeActions(Lens);
        AssetTools.RegisterAssetTypeActions(Flare);
        Registered.Add(Lens);
        Registered.Add(Flare);
    }
    void ShutdownModule() override
    {
        if (FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
        {
            FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
            PropertyEditor.UnregisterCustomClassLayout(UGlintLensProfile::StaticClass()->GetFName());
            PropertyEditor.NotifyCustomizationModuleChanged();
        }
        if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
        {
            IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
            for (const TSharedRef<IAssetTypeActions>& Action : Registered)
            {
                AssetTools.UnregisterAssetTypeActions(Action);
            }
        }
        Registered.Empty();
        FGlintEditorStyle::Shutdown();
    }
private:
    TArray<TSharedRef<IAssetTypeActions>> Registered;
};
IMPLEMENT_MODULE(FGlintEditorModule, GlintEditor)
