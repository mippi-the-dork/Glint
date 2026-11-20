#include "GlintEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Brushes/SlateImageBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

TSharedPtr<FSlateStyleSet> FGlintEditorStyle::Style;
void FGlintEditorStyle::Initialize()
{
    if (Style.IsValid()) return;
    Style=MakeShared<FSlateStyleSet>(TEXT("GlintEditorStyle"));
    const TSharedPtr<IPlugin> Plugin=IPluginManager::Get().FindPlugin(TEXT("Glint"));
    const FString Resources=Plugin.IsValid()
        ? FPaths::Combine(Plugin->GetBaseDir(),TEXT("Resources"))
        : FPaths::ProjectPluginsDir()/TEXT("Glint/Resources");
    Style->SetContentRoot(Resources);
    Style->Set(TEXT("Glint.Solo.Solid"),new FSlateImageBrush(Style->RootToContentDir(TEXT("solo-solid"), TEXT(".png")),FVector2D(16,16)));
    Style->Set(TEXT("Glint.Solo.Outline"),new FSlateImageBrush(Style->RootToContentDir(TEXT("solo-outline"), TEXT(".png")),FVector2D(16,16)));
    Style->Set(TEXT("Glint.Icon.Drag"),new FSlateImageBrush(Style->RootToContentDir(TEXT("drag-icon"), TEXT(".png")),FVector2D(16,16)));
    Style->Set(TEXT("Glint.Icon.Trash"),new FSlateImageBrush(Style->RootToContentDir(TEXT("trash-icon"), TEXT(".png")),FVector2D(16,16)));
    FSlateStyleRegistry::RegisterSlateStyle(*Style);
}
void FGlintEditorStyle::Shutdown()
{
    if (!Style.IsValid()) return;
    FSlateStyleRegistry::UnRegisterSlateStyle(*Style);
    Style.Reset();
}
const ISlateStyle& FGlintEditorStyle::Get()
{
    check(Style.IsValid());
    return *Style;
}
