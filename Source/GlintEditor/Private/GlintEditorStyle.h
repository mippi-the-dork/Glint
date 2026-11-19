#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

/** Small native Slate style for supplied Glint editor icons. */
class FGlintEditorStyle
{
public:
    static void Initialize();
    static void Shutdown();
    static const ISlateStyle& Get();
private:
    static TSharedPtr<FSlateStyleSet> Style;
};
