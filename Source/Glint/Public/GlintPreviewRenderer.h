#pragma once
#include "CoreMinimal.h"
#include "GlintLensProfile.h"
class UTextureRenderTarget2D;

/** GPU editor preview of the runtime's authored source-relative optics.
 *  Synthetic single source; scene-dependent detection is tested separately. */
struct GLINT_API FGlintPreviewRequest
{
    TArray<FGlintRuntimeElement> Elements;
    FVector2D SourceUV = FVector2D(0.74, 0.39);
    FLinearColor SourceColor = FLinearColor::White;
    float Exposure = 1.f;
};
GLINT_API void QueueGlintPreviewRender(UTextureRenderTarget2D* Target, const FGlintPreviewRequest& Request);
