#pragma once
#include "CoreMinimal.h"
#include "GlintFlare.h"
#include "GlintLensProfile.h"

/** A camera image inset into a larger authoring workspace.  Pure geometry, so
 *  automated tests can validate mapping without constructing a Slate widget. */
struct FGlintPreviewFrame
{
    FVector2D Origin = FVector2D::ZeroVector;
    FVector2D Size = FVector2D(1.0, 1.0);
    FVector2D WorkspaceSize = FVector2D(1.0, 1.0);

    static FIntPoint TargetResolutionForAspect(double CameraAspect)
    {
        // 768 pixels on the longest dimension: respect portrait and widescreen
        // reference textures without exceeding the preview's small GPU budget.
        CameraAspect=FMath::Clamp(CameraAspect,0.25,4.0);
        return CameraAspect>=1.0
            ? FIntPoint(768,FMath::Max(1,FMath::RoundToInt(768.0/CameraAspect)))
            : FIntPoint(FMath::Max(1,FMath::RoundToInt(768.0*CameraAspect)),768);
    }
    static FGlintPreviewFrame FromWorkspace(const FVector2D& Workspace, double CameraAspect = 768.0 / 480.0)
    {
        FGlintPreviewFrame Frame;
        Frame.WorkspaceSize = FVector2D(FMath::Max(1.0, Workspace.X), FMath::Max(1.0, Workspace.Y));
        // Match the GPU render target's current aspect, including a reference
        // screenshot when one is selected. Keep editing margins on all sides.
        CameraAspect = FMath::Clamp(CameraAspect, 0.25, 4.0);
        Frame.Size.X = FMath::Max(1.0, FMath::Min(Frame.WorkspaceSize.X * 0.76,
            Frame.WorkspaceSize.Y * 0.76 * CameraAspect));
        Frame.Size.Y = Frame.Size.X / CameraAspect;
        Frame.Origin = (Frame.WorkspaceSize - Frame.Size) * 0.5;
        return Frame;
    }
    FVector2D CameraToWorkspace(const FVector2D& UV) const
    {
        return Origin + FVector2D(UV.X * Size.X, UV.Y * Size.Y);
    }
    FVector2D WorkspaceToCamera(const FVector2D& Point) const
    {
        return FVector2D((Point.X-Origin.X)/Size.X, (Point.Y-Origin.Y)/Size.Y);
    }
    FVector2D ConstrainHandle(const FVector2D& Point, double Margin=12.0) const
    {
        const FVector2D MaxPoint(FMath::Max(Margin,WorkspaceSize.X-Margin),
                                 FMath::Max(Margin,WorkspaceSize.Y-Margin));
        return FVector2D(FMath::Clamp(Point.X, Margin, MaxPoint.X),
                         FMath::Clamp(Point.Y, Margin, MaxPoint.Y));
    }
    bool IsInCamera(const FVector2D& UV) const
    {
        return UV.X>=0.0 && UV.X<=1.0 && UV.Y>=0.0 && UV.Y<=1.0;
    }
};

/** No Slate dependencies: exercised by native automation and used by the actual canvas. */
struct FGlintPreviewSelectionOps
{
    static int32 RepairSelectionIndex(int32 Index,int32 Count)
    {
        return Count<=0 ? INDEX_NONE : (Index==INDEX_NONE ? 0 : FMath::Clamp(Index,0,Count-1));
    }
    static int32 RepairOptionalIndex(int32 Index,int32 Count)
    {
        return Index>=0 && Index<Count ? Index : INDEX_NONE;
    }
    static FVector2D ElementUV(const FGlintElement& Element, const FVector2D& Source)
    {
        return FVector2D(0.5+(Source.X-0.5)*Element.AxisPosition+Element.ScreenOffset.X,
                         0.5+(Source.Y-0.5)*Element.AxisPosition+Element.ScreenOffset.Y);
    }
    static int32 DestinationAfterDrop(int32 Source,int32 InsertionIndex,int32 Count)
    {
        if (Source<0 || Source>=Count || InsertionIndex<0 || InsertionIndex>Count)
            return INDEX_NONE;
        return FMath::Clamp(InsertionIndex-(Source<InsertionIndex?1:0),0,Count-1);
    }
    static int32 RemapIndexAfterMove(int32 Index,int32 Source,int32 Target)
    {
        if (Index==INDEX_NONE) return INDEX_NONE;
        if (Index==Source) return Target;
        if (Source<Target && Index>Source && Index<=Target) return Index-1;
        if (Source>Target && Index>=Target && Index<Source) return Index+1;
        return Index;
    }
    static TArray<int32> FindHits(const UGlintFlare& Flare, const FVector2D& SourceUV,
                                 const FVector2D& Local, const FVector2D& Extent,
                                 double HitRadius=18.0, double CameraAspect=768.0/480.0)
    {
        TArray<int32> Hits;
        const FGlintPreviewFrame Frame=FGlintPreviewFrame::FromWorkspace(Extent,CameraAspect);
        for (int32 Index=0;Index<Flare.Elements.Num();++Index)
        {
            if (!Flare.Elements[Index].bEnabled) continue;
            const FVector2D UV=ElementUV(Flare.Elements[Index],SourceUV);
            if (FVector2D::DistSquared(Frame.ConstrainHandle(Frame.CameraToWorkspace(UV)),Local)<=FMath::Square(HitRadius))
                Hits.Add(Index);
        }
        return Hits;
    }
    static int32 CycleHit(const TArray<int32>& Hits, int32 CurrentlySelected)
    {
        if (Hits.IsEmpty()) return INDEX_NONE;
        const int32 Position=Hits.Find(CurrentlySelected);
        return Position==INDEX_NONE?Hits[0]:Hits[(Position+1)%Hits.Num()];
    }
    static void PackVisible(const UGlintFlare& Flare, int32 SoloIndex,
                            TArray<FGlintRuntimeElement>& Output)
    {
        Output.Reset();
        for (int32 Index=0;Index<Flare.Elements.Num() && Output.Num()<FGlintLensBlendData::MaxElementsPerLens;++Index)
        {
            const FGlintElement& E=Flare.Elements[Index];
            if (!E.bEnabled || E.Intensity<=0.f || (SoloIndex!=INDEX_NONE && SoloIndex!=Index)) continue;
            Output.Add(UGlintLensProfile::PackElement(E));
        }
    }
};
