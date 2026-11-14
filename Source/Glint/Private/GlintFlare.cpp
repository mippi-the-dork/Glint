#include "GlintFlare.h"
int32 UGlintFlare::GetEnabledElementCount() const
{
    int32 Count = 0;
    for (const FGlintElement& Element : Elements) { Count += Element.bEnabled ? 1 : 0; }
    return Count;
}
