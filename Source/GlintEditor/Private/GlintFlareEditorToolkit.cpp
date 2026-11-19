#include "GlintFlareEditorToolkit.h"
#include "GlintElementDetailsProxy.h"
#include "GlintPreviewSelection.h"
#include "GlintEditorStyle.h"
#include "GlintLensProfile.h"
#include "GlintPreviewRenderer.h"
#include "PropertyEditorModule.h"
#include "PropertyCustomizationHelpers.h"
#include "AssetRegistry/AssetData.h"
#include "UObject/SoftObjectPath.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Images/SImage.h"
#include "Input/DragAndDrop.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateColor.h"
#include "Rendering/DrawElementTypes.h"
#include "Widgets/SLeafWidget.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "ScopedTransaction.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/Crc.h"
#include "Misc/ConfigCacheIni.h"
#include "RenderingThread.h"
#include "Editor.h"
#include "EditorUndoClient.h"

const FName FGlintFlareEditorToolkit::TabID(TEXT("Glint.Flare.VisualEditor"));

namespace
{
    static constexpr int32 ElementTypes = static_cast<int32>(EGlintElementType::DustCluster) + 1;
    static TArray<FVector2D> Circle(const FVector2D& Center, float Radius, int32 Segments = 48)
    {
        // Single closed polyline. Previous angular overdraw fixed a gap but
        // doubled the stroke near its seam. The last point equals the first.
        TArray<FVector2D> Points;
        Points.Reserve(Segments + 1);
        for (int32 i = 0; i <= Segments; ++i)
        {
            const double Angle = 2.0 * PI * double(i) / double(Segments);
            Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
        }
        return Points;
    }
    static TArray<FVector2D> Polygon(const FVector2D& Center,float Radius,int32 Sides,float Rotation)
    {
        TArray<FVector2D> Points;
        Sides=FMath::Clamp(Sides,3,16);
        Points.Reserve(Sides+1);
        for(int32 i=0;i<=Sides;++i)
        {
            const float Angle=Rotation+2.f*PI*float(i)/float(Sides);
            Points.Add(Center+FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*Radius);
        }
        return Points;
    }
    static TArray<FVector2D> PetalPath(const FVector2D& Center,float Radius,int32 Petals,float Rotation)
    {
        TArray<FVector2D> Points;
        Petals=FMath::Clamp(Petals,3,16);
        Points.Reserve(97);
        for(int32 i=0;i<=96;++i)
        {
            const float Angle=2.f*PI*float(i)/96.f;
            const float R=Radius*(1.f+0.16f*FMath::Cos(Angle*Petals));
            Points.Add(Center+FVector2D(FMath::Cos(Angle+Rotation),FMath::Sin(Angle+Rotation))*R);
        }
        return Points;
    }
}

class SGlintFlareCanvas final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SGlintFlareCanvas) {}
        SLATE_ARGUMENT(UGlintFlare*, Flare)
        SLATE_ARGUMENT(TSharedPtr<FGlintFlareSelection>, Selection)
        SLATE_ARGUMENT(const FSlateBrush*, PreviewBrush)
        SLATE_ARGUMENT(const FSlateBrush*, BackgroundBrush)
        SLATE_EVENT(FSimpleDelegate, OnSelectionChanged)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        Flare = Args._Flare;
        Selection = Args._Selection;
        PreviewBrush = Args._PreviewBrush;
        BackgroundBrush = Args._BackgroundBrush;
        OnSelectionChanged = Args._OnSelectionChanged;
    }
    FVector2D ComputeDesiredSize(float) const override { return FVector2D(768, 480); }

    int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& CullRect,
        FSlateWindowElementList& Draw, int32 Layer, const FWidgetStyle& Style, bool Enabled) const override
    {
        const FVector2D Extent = Geo.GetLocalSize();
        const FGlintPreviewFrame Frame=FGlintPreviewFrame::FromWorkspace(Extent, Selection->CameraAspect);
        const bool bSchematic = Selection.IsValid() && Selection->Mode == EGlintPreviewMode::Schematic;
        const bool bHybrid = Selection.IsValid() && Selection->Mode == EGlintPreviewMode::Hybrid;
        // Limit *drawing* to the editor canvas. Optical coordinates remain unbounded:
        // a remote element must not be destroyed/clamped just because its handle is offscreen.
        Draw.PushClip(FSlateClippingZone(Geo.GetLayoutBoundingRect()));
        FSlateDrawElement::MakeBox(Draw, Layer, Geo.ToPaintGeometry(),
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None,
            FLinearColor(0.026f, 0.026f, 0.026f, 1.f));
        const FPaintGeometry FrameGeometry=Geo.ToPaintGeometry(Frame.Size,FSlateLayoutTransform(Frame.Origin));
        // The background (including a user-provided screenshot) occupies only
        // the camera image, never the larger offscreen authoring workspace.
        // Keep Schematic distraction-free; Rendered/Hybrid share this image.
        FSlateDrawElement::MakeBox(Draw, Layer+1, FrameGeometry,
            FAppStyle::GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None,
            bSchematic ? FLinearColor(0.019f,0.019f,0.019f,1.f)
                       : FLinearColor(0.012f,0.012f,0.012f,1.f));
        if (!bSchematic && BackgroundBrush && BackgroundBrush->GetResourceObject())
        {
            FSlateDrawElement::MakeBox(Draw, Layer+2, FrameGeometry,
                BackgroundBrush, ESlateDrawEffect::None, FLinearColor::White);
        }
        if (!bSchematic && PreviewBrush)
        {
            // Premultiplied-looking additive flare contribution is encoded as
            // straight-alpha by the preview shader to layer over any screenshot.
            // Source UV is still independent of the background image.
            FSlateDrawElement::MakeBox(Draw, Layer+3, FrameGeometry,
                PreviewBrush, ESlateDrawEffect::None, FLinearColor::White);
        }
        auto Line = [&](const TArray<FVector2D>& Points, FLinearColor Color, float Width=1.f, int32 ExtraLayer=4)
        {
            TArray<FVector2f> Converted;
            Converted.Reserve(Points.Num());
            for (const FVector2D& P : Points) Converted.Add(FVector2f(float(P.X), float(P.Y)));
            FSlateDrawElement::MakeLines(Draw, Layer+ExtraLayer, Geo.ToPaintGeometry(),
                MoveTemp(Converted), ESlateDrawEffect::None, Color, true, Width);
        };
        const FVector2D TopLeft=Frame.Origin;
        const FVector2D BottomRight=Frame.Origin+Frame.Size;
        Line({TopLeft,FVector2D(BottomRight.X,TopLeft.Y),BottomRight,
            FVector2D(TopLeft.X,BottomRight.Y),TopLeft},
            FLinearColor(0.19f,0.19f,0.20f,0.85f),1.35f);
        // Light ticks emphasize exactly where the visible camera frame ends.
        const double Tick=9.0;
        for (const FVector2D& Corner : TArray<FVector2D>{TopLeft,
            FVector2D(BottomRight.X,TopLeft.Y),BottomRight,FVector2D(TopLeft.X,BottomRight.Y)})
        {
            const double InwardX=Corner.X==TopLeft.X?Tick:-Tick;
            const double InwardY=Corner.Y==TopLeft.Y?Tick:-Tick;
            Line({Corner,Corner+FVector2D(InwardX,0)},FLinearColor(0.29f,0.29f,0.30f),1.5f);
            Line({Corner,Corner+FVector2D(0,InwardY)},FLinearColor(0.29f,0.29f,0.30f),1.5f);
        }
        if (!bSchematic && !bHybrid)
        {
            Draw.PopClip();
            return Layer+6;
        }
        const FVector2D Center=Frame.CameraToWorkspace(FVector2D(0.5,0.5));
        Line({FVector2D(Center.X,0),FVector2D(Center.X,Extent.Y)},FLinearColor(0.10f,0.10f,0.11f,0.48f));
        Line({FVector2D(0,Center.Y),FVector2D(Extent.X,Center.Y)},FLinearColor(0.10f,0.10f,0.11f,0.48f));
        const FVector2D SourceRaw=Frame.CameraToWorkspace(Selection->SourceUV);
        const FVector2D Source=Frame.ConstrainHandle(SourceRaw);
        Line({Center,Source},FLinearColor(0.14f,0.16f,0.19f,0.6f),1.f);
        // The synthetic highlight is an editor-only anchor, not a flare disc.
        // Rendered mode exits above; in Schematic/Hybrid use a UE-blue dot.
        FSlateDrawElement::MakeBox(Draw,Layer+5,
            Geo.ToPaintGeometry(FVector2D(6,6),FSlateLayoutTransform(Source-FVector2D(3,3))),
            FAppStyle::GetBrush(TEXT("WhiteBrush")),ESlateDrawEffect::None,
            FLinearColor(0.015f,0.42f,0.94f,1.f));
        if (Flare)
        {
            for (int32 Index=0;Index<Flare->Elements.Num();++Index)
            {
                const FGlintElement& E=Flare->Elements[Index];
                if (!E.bEnabled) continue;
                const FVector2D UV=FGlintPreviewSelectionOps::ElementUV(E,Selection->SourceUV);
                const FVector2D RawP=Frame.CameraToWorkspace(UV);
                const FVector2D P=Frame.ConstrainHandle(RawP);
                const bool bOutsideWorkspace=FVector2D::DistSquared(RawP,P)>4.0;
                const float Radius=FMath::Clamp(E.Size*17.f,3.f,120.f);
                FLinearColor Color=E.Color.GetClamped();
                const bool bSelected=Selection->ElementIndex==Index;
                const bool bHovered=Selection->HoveredIndex==Index;
                const float BaseAlpha=FMath::Clamp(Selection->SchematicOpacity,0.05f,1.f);
                Color.A=(Selection->SoloIndex!=INDEX_NONE && Selection->SoloIndex!=Index)?BaseAlpha*0.20f
                    : ((bSelected || bHovered) ? 1.f : BaseAlpha);
                const float Rotation=FMath::DegreesToRadians(E.RotationDegrees);
                // An element outside the working canvas is represented by a small
                // directional edge handle, never by an unselectable off-window marker.
                if (bOutsideWorkspace)
                {
                    const FVector2D Dir=(RawP-P).GetSafeNormal();
                    const FVector2D Side(-Dir.Y,Dir.X);
                    Line({P-Dir*13.0,P},Color,2.f);
                    Line({P-Dir*7.0+Side*4.0,P,P-Dir*7.0-Side*4.0},Color,1.4f);
                }
                else switch (E.Type)
                {
                case EGlintElementType::Starburst:
                case EGlintElementType::SpikeBall:
                case EGlintElementType::Sparkle:
                case EGlintElementType::CrossStar:
                {
                    const int32 Rays=FMath::Clamp(E.RayCount,2,16);
                    for (int32 Ray=0;Ray<Rays;++Ray)
                    {
                        const float A=Rotation+Ray*PI/Rays;
                        const FVector2D Dir(FMath::Cos(A),FMath::Sin(A));
                        Line({P-Dir*Radius*2.2,P+Dir*Radius*2.2},Color,1.2f);
                    }
                    break;
                }
                case EGlintElementType::Streak:
                case EGlintElementType::Shimmer:
                case EGlintElementType::VerticalStreak:
                case EGlintElementType::DualStreak:
                case EGlintElementType::Smear:
                {
                    const FVector2D Dir(FMath::Cos(Rotation),FMath::Sin(Rotation));
                    Line({P-Dir*Radius*3.,P+Dir*Radius*3.},Color,1.6f);
                    if (E.Type==EGlintElementType::DualStreak)
                    {
                        const FVector2D Perp(-Dir.Y,Dir.X);
                        Line({P-Perp*Radius*1.5,P+Perp*Radius*1.5},Color,1.3f);
                    }
                    break;
                }
                case EGlintElementType::Halo:
                case EGlintElementType::Ring:
                case EGlintElementType::Hoop:
                case EGlintElementType::EdgeRing:
                    Line(Circle(P,Radius*(E.Type==EGlintElementType::Hoop?1.8f:1.f)),Color,1.7f);
                    break;
                case EGlintElementType::Iris:
                case EGlintElementType::MultiIris:
                    Line(Circle(P,Radius*0.65),Color,1.5f);
                    Line(Circle(P,Radius),Color*0.55f,1.f);
                    if (E.Type==EGlintElementType::MultiIris) Line(Circle(P,Radius*1.5f),Color*0.4f,1.f);
                    break;
                case EGlintElementType::PolygonIris:
                case EGlintElementType::ApertureBladeRing:
                case EGlintElementType::ApertureGhost:
                {
                    Line(Polygon(P,Radius,FMath::Max(3,E.RayCount),Rotation),Color,1.6f);
                    if (E.Type==EGlintElementType::ApertureGhost)
                        Line(Polygon(P,Radius*0.72f,FMath::Max(3,E.RayCount),Rotation),Color*0.5f,1.f);
                    break;
                }
                case EGlintElementType::PetalIris:
                    Line(PetalPath(P,Radius,E.RayCount,Rotation),Color,1.4f);
                    break;
                case EGlintElementType::ArcReflection:
                {
                    TArray<FVector2D> Arc;
                    for(int32 i=0;i<=18;++i)
                    {
                        const float A=-1.25f+2.5f*float(i)/18.f+Rotation;
                        Arc.Add(P+FVector2D(FMath::Cos(A),FMath::Sin(A))*Radius);
                    }
                    Line(Arc,Color,1.8f);
                    break;
                }
                case EGlintElementType::DustCluster:
                {
                    for(int32 i=0;i<9;++i)
                    {
                        const float A=float(i)*2.4f;
                        const FVector2D Dot=P+FVector2D(FMath::Cos(A),FMath::Sin(A))*(Radius*0.18f*(1+i%3));
                        Line(Circle(Dot,2.2f,16),Color,1.2f);
                    }
                    break;
                }
                default:
                    Line(Circle(P,Radius*0.6f),Color,1.5f);
                    break;
                }
                if (bSelected || bHovered)
                    Line(Circle(P,FMath::Max(8.f,Radius*0.4f)),
                        bSelected?FLinearColor(0.08f,0.58f,1.f):FLinearColor(0.85f,0.72f,0.34f),2.0f);
                Line(Circle(P,2.5f),Color,2.f);
            }
        }
        Draw.PopClip();
        return Layer+6;
    }

    FReply OnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Mouse) override
    {
        if (!Flare || !Selection.IsValid()) return FReply::Unhandled();
        const FVector2D Local=Geo.AbsoluteToLocal(Mouse.GetScreenSpacePosition());
        const FVector2D Size=Geo.GetLocalSize();
        if (Mouse.GetEffectingButton()==EKeys::RightMouseButton)
        {
            bDraggingSource=true;
            SourceDragStartLocal=Local;
            SourceDragStartUV=Selection->SourceUV;
            return FReply::Handled().CaptureMouse(AsShared());
        }
        if (Mouse.GetEffectingButton()!=EKeys::LeftMouseButton) return FReply::Unhandled();
        // Rendered mode intentionally has no hidden clickable handles. Hybrid
        // and Schematic provide the direct manipulation surfaces.
        if (Selection->Mode==EGlintPreviewMode::Rendered) return FReply::Unhandled();
        const TArray<int32> Hits=FGlintPreviewSelectionOps::FindHits(*Flare,Selection->SourceUV,Local,Size,18.0,Selection->CameraAspect);
        const int32 Next=FGlintPreviewSelectionOps::CycleHit(Hits,Selection->ElementIndex);
        Selection->ElementIndex=Next;
        OnSelectionChanged.ExecuteIfBound();
        if (Next==INDEX_NONE) return FReply::Handled();
        // Selection alone must NOT open an undo transaction or dirty the asset.
        DragBeginLocal=Local;
        bDidModify=false;
        bDraggingElement=true;
        return FReply::Handled().CaptureMouse(AsShared());
    }
    FReply OnMouseMove(const FGeometry& Geo, const FPointerEvent& Mouse) override
    {
        if (!Flare || !Selection.IsValid()) return FReply::Unhandled();
        const FVector2D Local=Geo.AbsoluteToLocal(Mouse.GetScreenSpacePosition());
        const FVector2D Extent=Geo.GetLocalSize();
        if (Extent.X<1 || Extent.Y<1) return FReply::Unhandled();
        if (HasMouseCapture() && bDraggingSource)
        {
            const FGlintPreviewFrame Frame=FGlintPreviewFrame::FromWorkspace(Extent, Selection->CameraAspect);
            const FVector2D Delta=Local-SourceDragStartLocal;
            Selection->SourceUV=Frame.WorkspaceToCamera(Frame.ConstrainHandle(
                Frame.CameraToWorkspace(SourceDragStartUV)+Delta));
            Invalidate(EInvalidateWidgetReason::Paint);
            return FReply::Handled();
        }
        if (HasMouseCapture() && bDraggingElement && Flare->Elements.IsValidIndex(Selection->ElementIndex))
        {
            if (!bDidModify)
            {
                if (FVector2D::DistSquared(Local,DragBeginLocal)<FMath::Square(3.0)) return FReply::Handled();
                DragTransaction=MakeUnique<FScopedTransaction>(FText::FromString(TEXT("Move Glint Flare Element")));
                Flare->Modify();
                bDidModify=true;
            }
            FGlintElement& E=Flare->Elements[Selection->ElementIndex];
            const FGlintPreviewFrame Frame=FGlintPreviewFrame::FromWorkspace(Extent, Selection->CameraAspect);
            const FVector2D Desired=Frame.WorkspaceToCamera(Frame.ConstrainHandle(Local));
            const FVector2D AxisPosition(0.5+(Selection->SourceUV.X-0.5)*E.AxisPosition,
                                          0.5+(Selection->SourceUV.Y-0.5)*E.AxisPosition);
            E.ScreenOffset.X=FMath::Clamp(Desired.X-AxisPosition.X,-4.0,4.0);
            E.ScreenOffset.Y=FMath::Clamp(Desired.Y-AxisPosition.Y,-4.0,4.0);
            Invalidate(EInvalidateWidgetReason::Paint);
            return FReply::Handled();
        }
        if (Selection->Mode==EGlintPreviewMode::Rendered) return FReply::Unhandled();
        const FGlintPreviewFrame Frame=FGlintPreviewFrame::FromWorkspace(Extent, Selection->CameraAspect);
        int32 Hover=INDEX_NONE;
        float Best=FMath::Square(18.f);
        for (int32 i=0; i<Flare->Elements.Num(); ++i)
        {
            if (!Flare->Elements[i].bEnabled) continue;
            const FVector2D UV=FGlintPreviewSelectionOps::ElementUV(Flare->Elements[i],Selection->SourceUV);
            const float Dist=FVector2D::DistSquared(Frame.ConstrainHandle(Frame.CameraToWorkspace(UV)),Local);
            if (Dist<Best) { Best=Dist;Hover=i; }
        }
        if (Selection->HoveredIndex!=Hover) { Selection->HoveredIndex=Hover;Invalidate(EInvalidateWidgetReason::Paint); }
        return FReply::Unhandled();
    }
    FReply OnMouseButtonUp(const FGeometry& Geo, const FPointerEvent& Mouse) override
    {
        if (!HasMouseCapture() || (bDraggingSource && Mouse.GetEffectingButton()!=EKeys::RightMouseButton)
            || (bDraggingElement && Mouse.GetEffectingButton()!=EKeys::LeftMouseButton)) return FReply::Unhandled();
        FinishDrag();
        return FReply::Handled().ReleaseMouseCapture();
    }
    void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override { FinishDrag(); }
private:
    void FinishDrag()
    {
        if (bDraggingElement && bDidModify && Flare)
        {
            Flare->MarkPackageDirty();Flare->PostEditChange();OnSelectionChanged.ExecuteIfBound();
        }
        DragTransaction.Reset(); bDraggingElement=false;bDraggingSource=false;bDidModify=false;
    }
    UGlintFlare* Flare=nullptr;
    TSharedPtr<FGlintFlareSelection> Selection;
    const FSlateBrush* PreviewBrush=nullptr;
    const FSlateBrush* BackgroundBrush=nullptr;
    FSimpleDelegate OnSelectionChanged;
    TUniquePtr<FScopedTransaction> DragTransaction;
    bool bDraggingSource=false;
    bool bDraggingElement=false;
    bool bDidModify=false;
    FVector2D DragBeginLocal=FVector2D::ZeroVector;
    FVector2D SourceDragStartLocal=FVector2D::ZeroVector;
    FVector2D SourceDragStartUV=FVector2D::ZeroVector;
};

// Reordering is a native Slate drag operation. Only the grip starts a drag,
// so clicking the row still selects without moving or dirtying the asset.
class FGlintElementDragOp final : public FDecoratedDragDropOp
{
public:
    DRAG_DROP_OPERATOR_TYPE(FGlintElementDragOp,FDecoratedDragDropOp)
    int32 FromIndex=INDEX_NONE;
    static TSharedRef<FGlintElementDragOp> New(int32 Index)
    {
        TSharedRef<FGlintElementDragOp> Op=MakeShared<FGlintElementDragOp>();
        Op->FromIndex=Index;
        Op->DefaultHoverText=FText::FromString(TEXT("Move Glint element (release over row)"));
        Op->Construct();
        return Op;
    }
};
DECLARE_DELEGATE_TwoParams(FOnGlintReorder,int32,int32);
class SGlintElementGrip final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGlintElementGrip) {} SLATE_ARGUMENT(int32,Index) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Index=Args._Index;
        ChildSlot[
            SNew(SBox).WidthOverride(18.f).HeightOverride(24.f)
            [SNew(SBorder).Padding(FMargin(2,2))
                .BorderImage(FAppStyle::GetBrush(TEXT("NoBorder")))
                .ToolTipText(FText::FromString(TEXT("Drag to reorder element")))
                [SNew(SImage).Image(FGlintEditorStyle::Get().GetBrush(TEXT("Glint.Icon.Drag")))
                    .ColorAndOpacity(FLinearColor::White)]]];
    }
    FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent& Mouse) override
    {
        return Mouse.GetEffectingButton()==EKeys::LeftMouseButton
            ? FReply::Handled().DetectDrag(AsShared(),EKeys::LeftMouseButton) : FReply::Unhandled();
    }
    FReply OnDragDetected(const FGeometry&,const FPointerEvent&) override
    { return FReply::Handled().BeginDragDrop(FGlintElementDragOp::New(Index)); }
private:
    int32 Index=INDEX_NONE;
};
class SGlintElementDropRow final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGlintElementDropRow) {}
        SLATE_ARGUMENT(int32,Index)
        SLATE_EVENT(FOnGlintReorder,OnReorder)
        SLATE_DEFAULT_SLOT(FArguments,Content)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Index=Args._Index; OnReorder=Args._OnReorder;
        ChildSlot[
            SAssignNew(Border,SBorder)
            .Padding(1)
            .BorderImage(FAppStyle::GetBrush(TEXT("WhiteBrush")))
            .BorderBackgroundColor(this,&SGlintElementDropRow::GetBorderColor)
            [Args._Content.Widget]
        ];
    }
    void OnDragEnter(const FGeometry& Geo,const FDragDropEvent& Event) override
    {
        if (Event.GetOperationAs<FGlintElementDragOp>().IsValid())
        {
            bDragHover=true;
            bDropAfter=Geo.AbsoluteToLocal(Event.GetScreenSpacePosition()).Y>=Geo.GetLocalSize().Y*0.5;
            Invalidate(EInvalidateWidgetReason::Paint);
        }
    }
    void OnDragLeave(const FDragDropEvent&) override
    {
        bDragHover=false;
        Invalidate(EInvalidateWidgetReason::Paint);
    }
    FReply OnDragOver(const FGeometry& Geo,const FDragDropEvent& Event) override
    {
        if (!Event.GetOperationAs<FGlintElementDragOp>().IsValid()) return FReply::Unhandled();
        const bool bAfter=Geo.AbsoluteToLocal(Event.GetScreenSpacePosition()).Y>=Geo.GetLocalSize().Y*0.5;
        if (bAfter!=bDropAfter) { bDropAfter=bAfter; Invalidate(EInvalidateWidgetReason::Paint); }
        return FReply::Handled();
    }
    FReply OnDrop(const FGeometry&,const FDragDropEvent& Event) override
    {
        bDragHover=false;
        Invalidate(EInvalidateWidgetReason::Paint);
        const TSharedPtr<FGlintElementDragOp> Op=Event.GetOperationAs<FGlintElementDragOp>();
        if (!Op.IsValid()) return FReply::Unhandled();
        OnReorder.ExecuteIfBound(Op->FromIndex,Index+(bDropAfter?1:0));
        return FReply::Handled();
    }
    int32 OnPaint(const FPaintArgs& Args,const FGeometry& Geo,const FSlateRect& CullRect,
        FSlateWindowElementList& Draw,int32 Layer,const FWidgetStyle& Style,bool Enabled) const override
    {
        const int32 OutLayer=SCompoundWidget::OnPaint(Args,Geo,CullRect,Draw,Layer,Style,Enabled);
        if (bDragHover)
        {
            const float Y=bDropAfter?float(Geo.GetLocalSize().Y)-1.f:1.f;
            TArray<FVector2f> Points={FVector2f(2.f,Y),FVector2f(float(Geo.GetLocalSize().X)-2.f,Y)};
            FSlateDrawElement::MakeLines(Draw,OutLayer+1,Geo.ToPaintGeometry(),
                Points,ESlateDrawEffect::None,FLinearColor(0.07f,0.56f,1.f,1.f),true,2.5f);
        }
        return OutLayer+1;
    }
private:
    FSlateColor GetBorderColor() const
    {
        return bDragHover ? FLinearColor(0.07f,0.21f,0.39f,0.25f) : FLinearColor(0.f,0.f,0.f,0.f);
    }
    int32 Index=INDEX_NONE;
    bool bDragHover=false;
    bool bDropAfter=false;
    TSharedPtr<SBorder> Border;
    FOnGlintReorder OnReorder;
};

FGlintFlareEditorToolkit::~FGlintFlareEditorToolkit()
{
    if (GEditor) GEditor->UnregisterForUndo(this);
    if (PreviewTicker.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(PreviewTicker);
    SavePreviewSettings();
}
void FGlintFlareEditorToolkit::RefreshAfterUndoRedo()
{
    if (!EditingFlare.IsValid() || !Selection.IsValid()) return;
    // Transactions restore the DATA ASSET, not the transient Details proxy,
    // outliner row widgets or cached GPU render target. Rebind all three.
    const int32 Count=EditingFlare->Elements.Num();
    Selection->ElementIndex=FGlintPreviewSelectionOps::RepairSelectionIndex(Selection->ElementIndex,Count);
    Selection->SoloIndex=FGlintPreviewSelectionOps::RepairOptionalIndex(Selection->SoloIndex,Count);
    Selection->HoveredIndex=INDEX_NONE;
    LastElementCount=-1;
    bHasPreviewHash=false;
    RefreshSelectedDetails();
    RebuildElementList();
    if (Canvas.IsValid()) Canvas->Invalidate(EInvalidateWidgetReason::Paint);
    TickPreview(0.f);
}
void FGlintFlareEditorToolkit::PostUndo(bool bSuccess)
{
    if (bSuccess) RefreshAfterUndoRedo();
}
void FGlintFlareEditorToolkit::PostRedo(bool bSuccess)
{
    if (bSuccess) RefreshAfterUndoRedo();
}
void FGlintFlareEditorToolkit::Open(UGlintFlare* InFlare, const TSharedPtr<IToolkitHost>& Host)
{
    check(InFlare);
    EditingFlare.Reset(InFlare);
    Selection=MakeShared<FGlintFlareSelection>();
    if (!InFlare->Elements.IsEmpty()) Selection->ElementIndex=0;
    LoadPreviewSettings();
    DetailsProxy.Reset(NewObject<UGlintElementDetailsProxy>(GetTransientPackage(),NAME_None,RF_Transient));
    UTextureRenderTarget2D* Texture=NewObject<UTextureRenderTarget2D>(GetTransientPackage(),NAME_None,RF_Transient);
    Texture->bSupportsUAV=true;
    const FIntPoint PreviewResolution=FGlintPreviewFrame::TargetResolutionForAspect(Selection->CameraAspect);
    Texture->InitCustomFormat(PreviewResolution.X,PreviewResolution.Y,PF_FloatRGBA,false);
    Texture->UpdateResourceImmediate(true);
    PreviewTarget.Reset(Texture);
    PreviewBrush.DrawAs=ESlateBrushDrawType::Image;
    PreviewBrush.ImageSize=FVector2D(PreviewResolution.X,PreviewResolution.Y);
    PreviewBrush.SetResourceObject(Texture);
    BackgroundBrush.DrawAs=ESlateBrushDrawType::Image;
    const TSharedRef<FTabManager::FLayout> Layout=FTabManager::NewLayout(TEXT("GlintFlareEditorLayout_v2"))
        ->AddArea(FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
            ->Split(FTabManager::NewStack()->AddTab(TabID,ETabState::OpenedTab)->SetHideTabWell(true)));
    InitAssetEditor(EToolkitMode::Standalone,Host,FName(TEXT("GlintFlareEditorApp")),Layout,true,true,InFlare);
    if (GEditor) GEditor->RegisterForUndo(this);
    PreviewTicker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(this,&FGlintFlareEditorToolkit::TickPreview),0.08f);
    TickPreview(0.f);
}
void FGlintFlareEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
    FAssetEditorToolkit::RegisterTabSpawners(Manager);
    Manager->RegisterTabSpawner(TabID,FOnSpawnTab::CreateSP(this,&FGlintFlareEditorToolkit::SpawnEditorTab))
        .SetDisplayName(FText::FromString(TEXT("Flare Elements")));
}
void FGlintFlareEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& Manager)
{
    Manager->UnregisterTabSpawner(TabID);
    FAssetEditorToolkit::UnregisterTabSpawners(Manager);
}
TSharedRef<SDockTab> FGlintFlareEditorToolkit::SpawnEditorTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab).Label(FText::FromString(TEXT("Glint Flare")))[BuildEditorContent()];
}
TSharedRef<SWidget> FGlintFlareEditorToolkit::BuildPreviewSettingsMenu()
{
    auto ColorButton=[this](const TCHAR* Name,FLinearColor Color)->TSharedRef<SWidget>
    {
        return SNew(SButton).Text(FText::FromString(Name))
            .ToolTipText(FText::FromString(TEXT("Preview source only; not saved to the flare")))
            .OnClicked_Lambda([this,Color](){Selection->SourceColor=Color;SavePreviewSettings();return FReply::Handled();});
    };
    // Bounding the WHOLE popup is essential: an auto-wrapped text block or
    // SObjectPropertyEntryBox can otherwise inflate a ComboButton menu to 800px.
    return SNew(SBox).WidthOverride(304.f)
      [SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Menu.Background"))).Padding(6)
      [SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4,3)
          [SNew(STextBlock).Text(FText::FromString(TEXT("Preview Settings")))]
        + SVerticalBox::Slot().AutoHeight().Padding(4,3)
          [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(FString::Printf(TEXT("Exposure  %.2fx"),Selection->Exposure));})]
        + SVerticalBox::Slot().AutoHeight().Padding(4,2)
          [SNew(SSlider).Value_Lambda([this](){return FMath::Clamp(Selection->Exposure/4.f,0.f,1.f);})
            .OnValueChanged_Lambda([this](float Value){Selection->Exposure=FMath::Max(0.05f,Value*4.f);})]
        + SVerticalBox::Slot().AutoHeight().Padding(4,3)
          [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(FString::Printf(TEXT("Schematic Opacity  %.0f%%"),100.f*Selection->SchematicOpacity));})]
        + SVerticalBox::Slot().AutoHeight().Padding(4,2)
          [SNew(SSlider).Value_Lambda([this](){return Selection->SchematicOpacity;})
             .OnValueChanged_Lambda([this](float Value){
                 Selection->SchematicOpacity=FMath::Clamp(Value,0.05f,1.f);
                 if (Canvas.IsValid()) Canvas->Invalidate(EInvalidateWidgetReason::Paint);
                 SavePreviewSettings();})]
        + SVerticalBox::Slot().AutoHeight().Padding(4,4)
          [SNew(STextBlock).Text(FText::FromString(TEXT("Preview Source Color")))]
        + SVerticalBox::Slot().AutoHeight().Padding(4,2)
          [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.f)[ColorButton(TEXT("W"),FLinearColor::White)]
            + SHorizontalBox::Slot().FillWidth(1.f)[ColorButton(TEXT("R"),FLinearColor(1.f,0.17f,0.1f))]
            + SHorizontalBox::Slot().FillWidth(1.f)[ColorButton(TEXT("G"),FLinearColor(0.1f,1.f,0.16f))]
            + SHorizontalBox::Slot().FillWidth(1.f)[ColorButton(TEXT("B"),FLinearColor(0.14f,0.35f,1.f))]]
        + SVerticalBox::Slot().AutoHeight().Padding(4,5)
          [SNew(STextBlock).Text(FText::FromString(TEXT("Reference Texture")))]
        + SVerticalBox::Slot().AutoHeight().Padding(4,2)
          [SNew(SObjectPropertyEntryBox)
            .AllowedClass(UTexture2D::StaticClass()).AllowClear(true).DisplayThumbnail(false)
            .ObjectPath_Lambda([this]() -> FString {
                return BackgroundTexture.IsValid()?BackgroundTexture->GetPathName():FString();})
            .OnObjectChanged_Lambda([this](const FAssetData& Data){
                SetBackgroundTexture(Cast<UTexture2D>(Data.GetAsset()));})]
        + SVerticalBox::Slot().AutoHeight().Padding(4,5)
          [SNew(STextBlock).AutoWrapText(true).WrapTextAt(278.f)
            .ColorAndOpacity(FLinearColor(0.65f,0.65f,0.65f,1.f))
            .Text(FText::FromString(TEXT("Editor-only backdrop. Not used for HDR detection. Right-drag on the canvas to move the source.")))]]];
}
TSharedRef<SWidget> FGlintFlareEditorToolkit::BuildPreviewToolbar()
{
    auto ModeButton=[this](EGlintPreviewMode Mode,const TCHAR* Text)->TSharedRef<SWidget>
    {
        return SNew(SButton).Text(FText::FromString(Text))
            .ToolTipText(FText::FromString(Mode==EGlintPreviewMode::Schematic ? TEXT("Diagram-only element editing") :
              Mode==EGlintPreviewMode::Hybrid ? TEXT("GPU rendered preview with selectable schematic overlay") :
              TEXT("GPU rendered preview without editing overlays")))
            .ButtonColorAndOpacity_Lambda([this,Mode]() -> FSlateColor {
                return FSlateColor(Selection->Mode==Mode ? FLinearColor(0.08f,0.40f,0.69f) : FLinearColor(0.21f,0.23f,0.28f));
            }).OnClicked_Lambda([this,Mode](){return SetPreviewMode(Mode);});
    };
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(3,0)[ModeButton(EGlintPreviewMode::Schematic,TEXT("Schematic"))]
        + SHorizontalBox::Slot().AutoWidth().Padding(3,0)[ModeButton(EGlintPreviewMode::Rendered,TEXT("Rendered"))]
        + SHorizontalBox::Slot().AutoWidth().Padding(3,0)[ModeButton(EGlintPreviewMode::Hybrid,TEXT("Hybrid"))]
        + SHorizontalBox::Slot().AutoWidth().Padding(6,0)
          [SNew(SComboButton).ToolTipText(FText::FromString(TEXT("Preview exposure, color and light controls")))
            .ButtonContent()[SNew(SImage).Image(FAppStyle::GetBrush(TEXT("Icons.Settings")))]
            .OnGetMenuContent(this,&FGlintFlareEditorToolkit::BuildPreviewSettingsMenu)];
}
TSharedRef<SWidget> FGlintFlareEditorToolkit::BuildPreviewStrip()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
        .Padding(FMargin(8,4))
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.f)
              [SNew(SBox)]
            + SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)
              [BuildPreviewToolbar()]];
}

TSharedRef<SWidget> FGlintFlareEditorToolkit::BuildEditorContent()
{
    FPropertyEditorModule& PropertyEditor=FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
    FDetailsViewArgs DetailArgs;
    DetailArgs.bHideSelectionTip=true;
    DetailArgs.bAllowSearch=true;
    DetailArgs.NameAreaSettings=FDetailsViewArgs::HideNameArea;
    Details=PropertyEditor.CreateDetailView(DetailArgs);
    Details->OnFinishedChangingProperties().AddRaw(this,&FGlintFlareEditorToolkit::OnSelectedDetailsEdited);
    RefreshSelectedDetails();
    TSharedRef<SWidget> CanvasWidget=SAssignNew(Canvas,SGlintFlareCanvas)
        .Flare(EditingFlare.Get()).Selection(Selection).PreviewBrush(&PreviewBrush)
        .BackgroundBrush(&BackgroundBrush)
        .OnSelectionChanged(FSimpleDelegate::CreateSP(this,&FGlintFlareEditorToolkit::OnCanvasSelectionChanged));
    // Native outliner above selected Details in one resizable LEFT column.
    // GPU canvas fills the RIGHT column, with an explicit in-panel preview strip.
    TSharedRef<SWidget> Editor=SNew(SSplitter).Orientation(Orient_Horizontal)
        + SSplitter::Slot().Value(0.28f)
        [
            SNew(SSplitter).Orientation(Orient_Vertical)
            + SSplitter::Slot().Value(0.56f)
            [SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
              [SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(6,5)
                  [SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                      [SNew(STextBlock).Text(FText::FromString(TEXT("Elements")))]
                    + SHorizontalBox::Slot().AutoWidth().Padding(2,0)
                      [SNew(SComboButton).ButtonContent()[SNew(STextBlock).Text(FText::FromString(TEXT("+ Add")))]
                       .OnGetMenuContent(this,&FGlintFlareEditorToolkit::BuildAddMenu)]
                    + SHorizontalBox::Slot().AutoWidth().Padding(2,0)
                      [SNew(SButton).Text(FText::FromString(TEXT("Duplicate")))
                        .ToolTipText(FText::FromString(TEXT("Duplicate selected element")))
                        .OnClicked(this,&FGlintFlareEditorToolkit::DuplicateElement)]]
                + SVerticalBox::Slot().AutoHeight().Padding(5,1)
                  [SNew(SSearchBox).HintText(FText::FromString(TEXT("Search elements")))
                   .OnTextChanged_Lambda([this](const FText& Search){Filter=Search.ToString();RebuildElementList();})]
                + SVerticalBox::Slot().FillHeight(1.f).Padding(4,4)
                  [SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(ElementList,SVerticalBox)]]]]
            + SSplitter::Slot().Value(0.44f)
            [SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
              [SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(6,5)
                  [SNew(STextBlock).Text(FText::FromString(TEXT("Selected Element Details")))]
                + SVerticalBox::Slot().FillHeight(1.f)[Details.ToSharedRef()]]]
        ]
        + SSplitter::Slot().Value(0.72f)
          [SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,4)
              [BuildPreviewStrip()]
            + SVerticalBox::Slot().FillHeight(1.f)
              [SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))[CanvasWidget]]];
    RebuildElementList();
    return Editor;
}
void FGlintFlareEditorToolkit::RebuildElementList()
{
    if (!ElementList.IsValid() || !EditingFlare.IsValid()) return;
    ElementList->ClearChildren();
    LastElementCount=EditingFlare->Elements.Num();
    for (int32 Index=0; Index<EditingFlare->Elements.Num(); ++Index)
    {
        const FGlintElement& E=EditingFlare->Elements[Index];
        const FString Type=UGlintFlare::GetTypeName(E.Type);
        if (!Filter.IsEmpty() && !E.Label.ToString().Contains(Filter,ESearchCase::IgnoreCase)
            && !Type.Contains(Filter,ESearchCase::IgnoreCase)) continue;
        const FString Row=FString::Printf(TEXT("%02d  %s [%s]"),Index+1,*E.Label.ToString(),*Type);
        ElementList->AddSlot().AutoHeight().Padding(2,1)
        [SNew(SGlintElementDropRow).Index(Index)
          .OnReorder(FOnGlintReorder::CreateSP(this,&FGlintFlareEditorToolkit::ReorderElement))
          [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
              [SNew(SGlintElementGrip).Index(Index)]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(1)
              [SNew(SBox).WidthOverride(24.f).HeightOverride(24.f)
                [SNew(SButton).ContentPadding(1)
                  .ToolTipText(FText::FromString(TEXT("Solo element in preview (does not alter asset)")))
                  .ButtonColorAndOpacity(FLinearColor::White)
                  .OnClicked_Lambda([this,Index](){return SoloElement(Index);})
                  [SNew(SImage).ColorAndOpacity(FLinearColor::White)
                    .Image(Selection->SoloIndex==Index?
                        FGlintEditorStyle::Get().GetBrush(TEXT("Glint.Solo.Solid")):
                        FGlintEditorStyle::Get().GetBrush(TEXT("Glint.Solo.Outline")))]]]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
              [SNew(SCheckBox)
                .ToolTipText(FText::FromString(TEXT("Toggle visibility in the asset and at runtime")))
                .IsChecked(E.bEnabled?ECheckBoxState::Checked:ECheckBoxState::Unchecked)
                .OnCheckStateChanged_Lambda([this,Index](ECheckBoxState){ToggleVisibility(Index);})]
            + SHorizontalBox::Slot().FillWidth(1.f).Padding(2,0)
              [SNew(SButton).Text(FText::FromString(Row))
                .ToolTipText(FText::FromString(TEXT("Select this element and edit its properties below")))
                .ButtonColorAndOpacity(Selection->ElementIndex==Index?
                    FLinearColor(0.1f,0.39f,0.69f):FLinearColor(0.22f,0.24f,0.29f))
                .OnClicked_Lambda([this,Index](){return SelectElement(Index);})]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(1)
              [SNew(SBox).WidthOverride(24.f).HeightOverride(24.f)
                [SNew(SButton).ContentPadding(1)
                  .ToolTipText(FText::FromString(TEXT("Delete this element (Undo supported)")))
                  .OnClicked_Lambda([this,Index](){return RemoveElement(Index);})
                  [SNew(SImage).ColorAndOpacity(FLinearColor::White)
                    .Image(FGlintEditorStyle::Get().GetBrush(TEXT("Glint.Icon.Trash")))]]]]];
    }
    if (Canvas.IsValid()) Canvas->Invalidate(EInvalidateWidgetReason::Paint);
}
void FGlintFlareEditorToolkit::RefreshSelectedDetails()
{
    if (!Details.IsValid() || !DetailsProxy.IsValid() || !EditingFlare.IsValid()) return;
    const int32 Index=Selection.IsValid()?Selection->ElementIndex:INDEX_NONE;
    if (EditingFlare->Elements.IsValidIndex(Index))
    {
        DetailsProxy->Element=EditingFlare->Elements[Index];
        Details->SetObject(DetailsProxy.Get(),true);
    }
    else Details->SetObject(nullptr,true);
}
void FGlintFlareEditorToolkit::OnSelectedDetailsEdited(const FPropertyChangedEvent& Event)
{
    if (!DetailsProxy.IsValid() || !EditingFlare.IsValid() || !Selection.IsValid()
        || !EditingFlare->Elements.IsValidIndex(Selection->ElementIndex)) return;
    const FScopedTransaction Transaction(FText::FromString(TEXT("Edit Glint Flare Element")));
    EditingFlare->Modify();
    FGlintElement& Existing=EditingFlare->Elements[Selection->ElementIndex];
    if (Event.Property && Event.Property->GetFName()==GET_MEMBER_NAME_CHECKED(FGlintElement,Type)
        && DetailsProxy->Element.Type != Existing.Type)
    {
        const FGlintElement OldDefaults=UGlintFlare::MakeDefaultElement(Existing.Type);
        FGlintElement NewDefaults=UGlintFlare::MakeDefaultElement(DetailsProxy->Element.Type);
        // Keep deliberate custom labels when changing type; auto-labels follow the new type.
        if (Existing.Label != OldDefaults.Label) NewDefaults.Label=Existing.Label;
        DetailsProxy->Element=NewDefaults;
        Details->ForceRefresh();
    }
    Existing=DetailsProxy->Element;
    EditingFlare->PostEditChange();
    RebuildElementList();
}
FReply FGlintFlareEditorToolkit::SelectElement(int32 Index)
{
    if (!Selection.IsValid()) return FReply::Handled();
    Selection->ElementIndex=Index;
    RefreshSelectedDetails();
    RebuildElementList();
    return FReply::Handled();
}
void FGlintFlareEditorToolkit::OnCanvasSelectionChanged()
{
    RefreshSelectedDetails();
    RebuildElementList();
}
TSharedRef<SWidget> FGlintFlareEditorToolkit::BuildPresetMenu(EGlintElementType Type)
{
    TSharedRef<SVerticalBox> Choices=SNew(SVerticalBox);
    for(int32 Preset=0;Preset<3;++Preset)
    {
        const int32 Choice=Preset;
        const TCHAR* Name=Preset==0?TEXT("Default"):Preset==1?TEXT("Subtle"):TEXT("Cinematic");
        Choices->AddSlot().AutoHeight().Padding(2,1)
          [SNew(SButton).Text(FText::FromString(Name))
            .OnClicked_Lambda([this,Type,Choice](){return AddPresetElement(Type,Choice);})];
    }
    return Choices;
}
TSharedRef<SWidget> FGlintFlareEditorToolkit::BuildAddMenu()
{
    TSharedRef<SVerticalBox> Menu=SNew(SVerticalBox);
    for (int32 Index=0;Index<ElementTypes;++Index)
    {
        const EGlintElementType Type=EGlintElementType(Index);
        Menu->AddSlot().AutoHeight().Padding(1)
          [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.f)
              [SNew(SButton).HAlign(HAlign_Left)
                .Text(FText::FromString(UGlintFlare::GetTypeName(Type)))
                .ToolTipText(FText::FromString(TEXT("Add with type-specific defaults")))
                .OnClicked_Lambda([this,Type](){return AddPresetElement(Type,0);})]
            + SHorizontalBox::Slot().AutoWidth()
              [SNew(SComboButton).ToolTipText(FText::FromString(TEXT("Choose Default, Subtle, or Cinematic")))
                .ButtonContent()[SNew(STextBlock).Text(FText::FromString(TEXT("Presets")))]
                .OnGetMenuContent_Lambda([this,Type](){return BuildPresetMenu(Type);})]];
    }
    return SNew(SBox).WidthOverride(290.f).HeightOverride(470.f)
      [SNew(SScrollBox)+SScrollBox::Slot()[Menu]];
}
FReply FGlintFlareEditorToolkit::AddPresetElement(EGlintElementType Type,int32 Preset)
{
    if (!EditingFlare.IsValid()) return FReply::Handled();
    const FScopedTransaction Transaction(FText::FromString(TEXT("Add Glint Flare Element")));
    EditingFlare->Modify();
    EditingFlare->Elements.Add(UGlintFlare::MakePresetElement(Type,Preset));
    Selection->ElementIndex=EditingFlare->Elements.Num()-1;
    EditingFlare->PostEditChange();
    bHasPreviewHash=false;
    RefreshSelectedDetails();RebuildElementList();
    return FReply::Handled();
}
FReply FGlintFlareEditorToolkit::AddElement(EGlintElementType Type)
{
    return AddPresetElement(Type,0);
}
FReply FGlintFlareEditorToolkit::DuplicateElement()
{
    if (!EditingFlare.IsValid() || !EditingFlare->Elements.IsValidIndex(Selection->ElementIndex)) return FReply::Handled();
    const FScopedTransaction Transaction(FText::FromString(TEXT("Duplicate Glint Flare Element")));
    EditingFlare->Modify();
    EditingFlare->Elements.Insert(EditingFlare->Elements[Selection->ElementIndex],Selection->ElementIndex+1);
    ++Selection->ElementIndex;
    EditingFlare->PostEditChange();RefreshSelectedDetails();RebuildElementList();
    return FReply::Handled();
}
FReply FGlintFlareEditorToolkit::RemoveElement(int32 Index)
{
    if (!EditingFlare.IsValid() || !EditingFlare->Elements.IsValidIndex(Index)) return FReply::Handled();
    const FScopedTransaction Transaction(FText::FromString(TEXT("Remove Glint Flare Element")));
    EditingFlare->Modify();
    EditingFlare->Elements.RemoveAt(Index);
    if (Selection->SoloIndex==Index) Selection->SoloIndex=INDEX_NONE;
    else if (Selection->SoloIndex>Index) --Selection->SoloIndex;
    if (Selection->ElementIndex==Index)
        Selection->ElementIndex=EditingFlare->Elements.IsEmpty()?INDEX_NONE:
            FMath::Min(Index,EditingFlare->Elements.Num()-1);
    else if (Selection->ElementIndex>Index) --Selection->ElementIndex;
    EditingFlare->PostEditChange();RefreshSelectedDetails();RebuildElementList();
    return FReply::Handled();
}
void FGlintFlareEditorToolkit::ReorderElement(int32 SourceIndex,int32 InsertionIndex)
{
    if (!EditingFlare.IsValid() || !Selection.IsValid()) return;
    const int32 Target=FGlintPreviewSelectionOps::DestinationAfterDrop(
        SourceIndex,InsertionIndex,EditingFlare->Elements.Num());
    if (Target==INDEX_NONE || SourceIndex==Target) return;
    const FScopedTransaction Transaction(FText::FromString(TEXT("Reorder Glint Flare Element")));
    EditingFlare->Modify();
    const FGlintElement Element=EditingFlare->Elements[SourceIndex];
    EditingFlare->Elements.RemoveAt(SourceIndex);
    EditingFlare->Elements.Insert(Element,Target);
    Selection->ElementIndex=FGlintPreviewSelectionOps::RemapIndexAfterMove(
        Selection->ElementIndex,SourceIndex,Target);
    Selection->SoloIndex=FGlintPreviewSelectionOps::RemapIndexAfterMove(
        Selection->SoloIndex,SourceIndex,Target);
    EditingFlare->PostEditChange();
    RefreshSelectedDetails();RebuildElementList();
}
FReply FGlintFlareEditorToolkit::ToggleVisibility(int32 Index)
{
    if (!EditingFlare.IsValid() || !EditingFlare->Elements.IsValidIndex(Index)) return FReply::Handled();
    const FScopedTransaction Transaction(FText::FromString(TEXT("Toggle Glint Element Visibility")));
    EditingFlare->Modify();
    EditingFlare->Elements[Index].bEnabled=!EditingFlare->Elements[Index].bEnabled;
    EditingFlare->PostEditChange();RefreshSelectedDetails();RebuildElementList();
    return FReply::Handled();
}
FReply FGlintFlareEditorToolkit::SoloElement(int32 Index)
{
    if (!Selection.IsValid()) return FReply::Handled();
    Selection->SoloIndex=Selection->SoloIndex==Index?INDEX_NONE:Index;
    RebuildElementList();
    return FReply::Handled();
}
FReply FGlintFlareEditorToolkit::SetPreviewMode(EGlintPreviewMode Mode)
{
    if (Selection.IsValid()) Selection->Mode=Mode;
    if (Canvas.IsValid()) Canvas->Invalidate(EInvalidateWidgetReason::Paint);
    SavePreviewSettings();
    return FReply::Handled();
}
bool FGlintFlareEditorToolkit::TickPreview(float DeltaTime)
{
    if (!EditingFlare.IsValid() || !Selection.IsValid() || !PreviewTarget.IsValid()) return true;
    if (LastElementCount!=EditingFlare->Elements.Num())
    {
        if (!EditingFlare->Elements.IsValidIndex(Selection->ElementIndex))
            Selection->ElementIndex=EditingFlare->Elements.IsEmpty()?INDEX_NONE:0;
        RefreshSelectedDetails();RebuildElementList();
    }
    FGlintPreviewRequest Request;
    Request.SourceUV=Selection->SourceUV;
    Request.SourceColor=Selection->SourceColor;
    Request.Exposure=Selection->Exposure;
    FGlintPreviewSelectionOps::PackVisible(*EditingFlare,Selection->SoloIndex,Request.Elements);
    uint32 Hash=FCrc::MemCrc32(Request.Elements.GetData(),Request.Elements.Num()*sizeof(FGlintRuntimeElement));
    Hash=FCrc::MemCrc32(&Request.SourceUV,sizeof(Request.SourceUV),Hash);
    Hash=FCrc::MemCrc32(&Request.SourceColor,sizeof(Request.SourceColor),Hash);
    Hash=FCrc::MemCrc32(&Request.Exposure,sizeof(Request.Exposure),Hash);
    if (!bHasPreviewHash || Hash!=LastPreviewHash)
    {
        LastPreviewHash=Hash;bHasPreviewHash=true;
        QueueGlintPreviewRender(PreviewTarget.Get(),Request);
        if (Canvas.IsValid()) Canvas->Invalidate(EInvalidateWidgetReason::Paint);
    }
    return true;
}
void FGlintFlareEditorToolkit::SetBackgroundTexture(UTexture2D* NewTexture)
{
    BackgroundTexture.Reset(NewTexture);
    BackgroundBrush.SetResourceObject(NewTexture);
    if (NewTexture)
        BackgroundBrush.ImageSize=FVector2D(NewTexture->GetSizeX(),NewTexture->GetSizeY());
    else BackgroundBrush.ImageSize=FVector2D::ZeroVector;
    UpdateBackgroundAspect(true);
    SavePreviewSettings();
    if (Canvas.IsValid()) Canvas->Invalidate(EInvalidateWidgetReason::Paint);
}
void FGlintFlareEditorToolkit::UpdateBackgroundAspect(bool bResizeGPU)
{
    if (!Selection.IsValid()) return;
    const double DesiredAspect = BackgroundTexture.IsValid()
        ? double(FMath::Max(1,BackgroundTexture->GetSizeX())) / FMath::Max(1,BackgroundTexture->GetSizeY())
        : 768.0/480.0;
    const FIntPoint Resolution=FGlintPreviewFrame::TargetResolutionForAspect(DesiredAspect);
    const int32 Width=Resolution.X,Height=Resolution.Y;
    Selection->CameraAspect = double(Width)/Height;
    if (bResizeGPU && PreviewTarget.IsValid() &&
        (PreviewTarget->SizeX != Width || PreviewTarget->SizeY != Height))
    {
        // The previous GPU preview may still be queued against this resource.
        // Synchronize only on this infrequent user action, never each frame.
        FlushRenderingCommands();
        PreviewTarget->InitCustomFormat(Width,Height,PF_FloatRGBA,false);
        PreviewTarget->UpdateResourceImmediate(true);
        PreviewBrush.ImageSize=FVector2D(Width,Height);
        bHasPreviewHash=false;
    }
}
void FGlintFlareEditorToolkit::SavePreviewSettings() const
{
    if (!GConfig || !EditingFlare.IsValid() || !Selection.IsValid()) return;
    const FString Section=FString::Printf(TEXT("Glint.FlareEditor.%s"),*EditingFlare->GetPathName());
    GConfig->SetInt(*Section,TEXT("Mode"),int32(Selection->Mode),GEditorPerProjectIni);
    GConfig->SetFloat(*Section,TEXT("SourceX"),float(Selection->SourceUV.X),GEditorPerProjectIni);
    GConfig->SetFloat(*Section,TEXT("SourceY"),float(Selection->SourceUV.Y),GEditorPerProjectIni);
    GConfig->SetFloat(*Section,TEXT("Exposure"),Selection->Exposure,GEditorPerProjectIni);
    GConfig->SetFloat(*Section,TEXT("SchematicOpacity"),Selection->SchematicOpacity,GEditorPerProjectIni);
    // Editor-only per-asset preference. Never stores the screenshot in the runtime asset.
    GConfig->SetString(*Section,TEXT("BackgroundTexture"),
        BackgroundTexture.IsValid()?*BackgroundTexture->GetPathName():TEXT(""),GEditorPerProjectIni);
}
void FGlintFlareEditorToolkit::LoadPreviewSettings()
{
    if (!GConfig || !EditingFlare.IsValid() || !Selection.IsValid()) return;
    const FString Section=FString::Printf(TEXT("Glint.FlareEditor.%s"),*EditingFlare->GetPathName());
    int32 Mode=int32(Selection->Mode);
    GConfig->GetInt(*Section,TEXT("Mode"),Mode,GEditorPerProjectIni);
    Selection->Mode=EGlintPreviewMode(FMath::Clamp(Mode,0,2));
    float X=float(Selection->SourceUV.X),Y=float(Selection->SourceUV.Y),Exposure=Selection->Exposure;
    GConfig->GetFloat(*Section,TEXT("SourceX"),X,GEditorPerProjectIni);
    GConfig->GetFloat(*Section,TEXT("SourceY"),Y,GEditorPerProjectIni);
    GConfig->GetFloat(*Section,TEXT("Exposure"),Exposure,GEditorPerProjectIni);
    // A preview light may be outside the camera image; do not collapse it
    // back onto the picture boundary when reopening the editor.
    Selection->SourceUV=FVector2D(FMath::Clamp(X,-2.f,3.f),FMath::Clamp(Y,-2.f,3.f));
    Selection->Exposure=FMath::Clamp(Exposure,0.05f,4.f);
    float Opacity=Selection->SchematicOpacity;
    GConfig->GetFloat(*Section,TEXT("SchematicOpacity"),Opacity,GEditorPerProjectIni);
    Selection->SchematicOpacity=FMath::Clamp(Opacity,0.05f,1.f);
    FString BackgroundPath;
    if (GConfig->GetString(*Section,TEXT("BackgroundTexture"),BackgroundPath,GEditorPerProjectIni)
        && !BackgroundPath.IsEmpty())
    {
        UTexture2D* Loaded=Cast<UTexture2D>(FSoftObjectPath(BackgroundPath).TryLoad());
        BackgroundTexture.Reset(Loaded);
        BackgroundBrush.SetResourceObject(Loaded);
        if (Loaded) BackgroundBrush.ImageSize=FVector2D(Loaded->GetSizeX(),Loaded->GetSizeY());
    }
    UpdateBackgroundAspect(false);
}
void FGlintFlareEditorToolkit::RequestListRefresh() { RebuildElementList(); }
