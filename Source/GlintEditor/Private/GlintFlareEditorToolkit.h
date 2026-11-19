#pragma once
#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/StrongObjectPtr.h"
#include "Styling/SlateBrush.h"
#include "Containers/Ticker.h"
#include "GlintFlare.h"
#include "GlintElementDetailsProxy.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "EditorUndoClient.h"

class IDetailsView;
class SGlintFlareCanvas;
class SVerticalBox;
class UTextureRenderTarget2D;
class UGlintElementDetailsProxy;

enum class EGlintPreviewMode : uint8 { Schematic, Rendered, Hybrid };

struct FGlintFlareSelection
{
    int32 ElementIndex = INDEX_NONE;
    int32 HoveredIndex = INDEX_NONE;
    int32 SoloIndex = INDEX_NONE; // editor preview isolation; does not change saved asset
    EGlintPreviewMode Mode = EGlintPreviewMode::Hybrid;
    FVector2D SourceUV = FVector2D(0.74, 0.39);
    float Exposure = 1.f;
    /** Only schematic handles use this; rendered optical energy is unaffected. */
    float SchematicOpacity = 0.38f;
    double CameraAspect = 768.0 / 480.0;
    FLinearColor SourceColor = FLinearColor::White;
};

/** Native three-mode editor: shared schematic selection + same HLSL optics GPU preview. */
class FGlintFlareEditorToolkit final : public FAssetEditorToolkit, public FEditorUndoClient
{
public:
    virtual ~FGlintFlareEditorToolkit() override;
    void Open(UGlintFlare* InFlare, const TSharedPtr<IToolkitHost>& InHost);
    virtual FName GetToolkitFName() const override { return FName(TEXT("GlintFlareEditor")); }
    virtual FText GetBaseToolkitName() const override { return FText::FromString(TEXT("Glint Flare")); }
    virtual FString GetWorldCentricTabPrefix() const override { return TEXT("Glint "); }
    virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.13f, 0.48f, 0.85f); }
    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void PostUndo(bool bSuccess) override;
    virtual void PostRedo(bool bSuccess) override;
private:
    TSharedRef<SWidget> BuildPreviewStrip();
    TSharedRef<SDockTab> SpawnEditorTab(const FSpawnTabArgs& Args);
    TSharedRef<SWidget> BuildEditorContent();
    TSharedRef<SWidget> BuildPreviewToolbar();
    void RebuildElementList();
    void RefreshAfterUndoRedo();
    void RefreshSelectedDetails();
    void OnSelectedDetailsEdited(const FPropertyChangedEvent& Event);
    FReply SelectElement(int32 ElementIndex);
    FReply DuplicateElement();
    FReply RemoveElement(int32 Index);
    void ReorderElement(int32 SourceIndex, int32 TargetIndex);
    FReply ToggleVisibility(int32 Index);
    FReply SoloElement(int32 Index);
    FReply SetPreviewMode(EGlintPreviewMode Mode);
    TSharedRef<SWidget> BuildAddMenu();
    TSharedRef<SWidget> BuildPresetMenu(EGlintElementType Type);
    FReply AddPresetElement(EGlintElementType Type,int32 Preset);
    TSharedRef<SWidget> BuildPreviewSettingsMenu();
    FReply AddElement(EGlintElementType Type);
    bool TickPreview(float DeltaTime);
    void SetBackgroundTexture(UTexture2D* NewTexture);
    void UpdateBackgroundAspect(bool bResizeGPU);
    void SavePreviewSettings() const;
    void LoadPreviewSettings();
    void OnCanvasSelectionChanged();
    void RequestListRefresh();

    TStrongObjectPtr<UGlintFlare> EditingFlare;
    TStrongObjectPtr<UGlintElementDetailsProxy> DetailsProxy;
    TStrongObjectPtr<UTextureRenderTarget2D> PreviewTarget;
    TStrongObjectPtr<UTexture2D> BackgroundTexture; // editor-only, never serialized into UGlintFlare
    FSlateBrush PreviewBrush;
    FSlateBrush BackgroundBrush;
    TSharedPtr<FGlintFlareSelection> Selection;
    TSharedPtr<IDetailsView> Details;
    TSharedPtr<SVerticalBox> ElementList;
    TSharedPtr<SGlintFlareCanvas> Canvas;
    FString Filter;
    uint32 LastPreviewHash = 0u;
    bool bHasPreviewHash = false;
    int32 LastElementCount = -1;
    FTSTicker::FDelegateHandle PreviewTicker;
    static const FName TabID;
};
