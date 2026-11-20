#include "GlintFlare.h"
int32 UGlintFlare::GetEnabledElementCount() const
{
    int32 Count = 0;
    for (const FGlintElement& Element : Elements) { Count += Element.bEnabled ? 1 : 0; }
    return Count;
}

const TCHAR* UGlintFlare::GetTypeName(EGlintElementType Type)
{
    static const TCHAR* Names[] = { TEXT("Glow"), TEXT("Halo"), TEXT("Ghost"), TEXT("Starburst"),
        TEXT("Streak"), TEXT("Iris"), TEXT("Ring"), TEXT("Hoop"), TEXT("Multi Iris"),
        TEXT("Spike Ball"), TEXT("Shimmer"), TEXT("Sparkle"), TEXT("Caustic"),
        TEXT("Polygon Iris"), TEXT("Aperture Blade Ring"), TEXT("Aperture Ghost"),
        TEXT("Cross Star"), TEXT("Vertical Streak"), TEXT("Dual Streak"),
        TEXT("Smear"), TEXT("Arc Reflection"), TEXT("Edge Ring"),
        TEXT("Soft Disc"), TEXT("Veiling Glow"), TEXT("Petal Iris"), TEXT("Dust Cluster") };
    const int32 Index = static_cast<int32>(Type);
    return Index >= 0 && Index < UE_ARRAY_COUNT(Names) ? Names[Index] : TEXT("Unknown");
}

FGlintElement UGlintFlare::MakeDefaultElement(EGlintElementType Type)
{
    FGlintElement E;
    E.Type = Type;
    E.Label = FName(GetTypeName(Type));
    E.bEnabled = true;
    E.Color = FLinearColor::White;
    E.ScreenOffset = FVector2D::ZeroVector;
    switch (Type)
    {
    case EGlintElementType::Glow:
        E.Label=TEXT("Source Glow"); E.AxisPosition=1.f; E.Size=1.65f; E.Intensity=0.32f; E.Falloff=3.3f; break;
    case EGlintElementType::Halo:
        E.Label=TEXT("Soft Halo"); E.AxisPosition=0.9f; E.Size=1.65f; E.Intensity=0.26f;
        E.RingThickness=0.38f; E.Color=FLinearColor(0.9f,0.95f,1.f); break;
    case EGlintElementType::Ghost:
        E.Label=TEXT("Opposite Ghost"); E.AxisPosition=-0.65f; E.Size=0.85f; E.Intensity=0.36f;
        E.Color=FLinearColor(0.72f,0.9f,1.f); E.Falloff=4.2f; break;
    case EGlintElementType::Starburst:
        E.Label=TEXT("Diffraction Star"); E.AxisPosition=1.f; E.Size=1.4f; E.Intensity=0.2f;
        E.RayCount=4; E.RaySharpness=65.f; break;
    case EGlintElementType::Streak:
        E.Label=TEXT("Anamorphic Streak"); E.AxisPosition=1.f; E.Size=2.0f; E.Intensity=0.22f;
        E.AspectRatio=2.2f; E.RaySharpness=85.f; E.Color=FLinearColor(0.72f,0.86f,1.f); break;
    case EGlintElementType::Iris:
        E.Label=TEXT("Iris Reflection"); E.AxisPosition=-1.25f; E.Size=1.15f; E.Intensity=0.23f;
        E.RingThickness=0.12f; E.Color=FLinearColor(1.f,0.8f,0.62f); break;
    case EGlintElementType::Ring:
        E.Label=TEXT("Inner Ring"); E.AxisPosition=-1.45f; E.Size=0.95f; E.Intensity=0.19f;
        E.RingThickness=0.1f; E.Color=FLinearColor(0.65f,0.8f,1.f); break;
    case EGlintElementType::Hoop:
        E.Label=TEXT("Wide Hoop"); E.AxisPosition=-1.75f; E.Size=1.1f; E.Intensity=0.15f;
        E.RingThickness=0.08f; E.Color=FLinearColor(0.75f,0.87f,1.f); break;
    case EGlintElementType::MultiIris:
        E.Label=TEXT("Layered Iris"); E.AxisPosition=-1.2f; E.Size=1.2f; E.Intensity=0.18f;
        E.RingThickness=0.09f; E.SecondaryStrength=0.5f; E.Color=FLinearColor(0.9f,0.8f,1.f); break;
    case EGlintElementType::SpikeBall:
        E.Label=TEXT("Fine Spikes"); E.AxisPosition=1.f; E.Size=1.25f; E.Intensity=0.15f;
        E.RayCount=8; E.RaySharpness=95.f; break;
    case EGlintElementType::Shimmer:
        E.Label=TEXT("Anamorphic Shimmer"); E.AxisPosition=0.85f; E.Size=1.4f; E.Intensity=0.15f;
        E.AspectRatio=2.f; E.SecondaryStrength=0.55f; E.DetailFrequency=10.f;
        E.Color=FLinearColor(0.7f,0.85f,1.f); break;
    case EGlintElementType::Sparkle:
        E.Label=TEXT("Fine Sparkle"); E.AxisPosition=1.f; E.Size=0.85f; E.Intensity=0.18f;
        E.RayCount=6; E.RaySharpness=100.f; E.SecondaryStrength=0.35f;
        E.DetailFrequency=12.f; E.Falloff=5.5f; break;
    case EGlintElementType::Caustic:
        E.Label=TEXT("Caustic Reflection"); E.AxisPosition=-0.4f; E.Size=1.5f; E.Intensity=0.12f;
        E.AspectRatio=1.4f; E.SecondaryStrength=0.8f; E.DetailFrequency=6.f;
        E.Color=FLinearColor(0.75f,0.88f,1.f); break;
    case EGlintElementType::PolygonIris:
        E.Label=TEXT("Polygon Iris"); E.AxisPosition=-0.9f; E.Size=1.1f; E.Intensity=0.22f;
        E.RingThickness=0.11f; E.RayCount=6; E.Color=FLinearColor(0.8f,0.88f,1.f); break;
    case EGlintElementType::ApertureBladeRing:
        E.Label=TEXT("Aperture Blades"); E.AxisPosition=-1.35f; E.Size=1.0f; E.Intensity=0.2f;
        E.RingThickness=0.07f; E.RayCount=8; E.Color=FLinearColor(1.f,0.84f,0.7f); break;
    case EGlintElementType::ApertureGhost:
        E.Label=TEXT("Polygon Ghost"); E.AxisPosition=-0.6f; E.Size=0.95f; E.Intensity=0.22f;
        E.AspectRatio=1.05f; E.RayCount=6; E.Color=FLinearColor(0.74f,0.94f,1.f); break;
    case EGlintElementType::CrossStar:
        E.Label=TEXT("Cross Star"); E.AxisPosition=1.f; E.Size=1.35f; E.Intensity=0.16f;
        E.RayCount=2; E.RaySharpness=105.f; break;
    case EGlintElementType::VerticalStreak:
        E.Label=TEXT("Vertical Streak"); E.AxisPosition=1.f; E.RotationDegrees=90.f; E.Size=1.8f;
        E.Intensity=0.18f; E.RaySharpness=90.f; E.Color=FLinearColor(0.85f,0.9f,1.f); break;
    case EGlintElementType::DualStreak:
        E.Label=TEXT("Dual Streak"); E.AxisPosition=1.f; E.Size=1.65f; E.Intensity=0.16f;
        E.RaySharpness=85.f; E.SecondaryStrength=0.5f; E.Color=FLinearColor(0.7f,0.82f,1.f); break;
    case EGlintElementType::Smear:
        E.Label=TEXT("Comet Smear"); E.AxisPosition=-0.45f; E.Size=1.2f; E.Intensity=0.13f;
        E.AspectRatio=2.1f; E.RotationDegrees=15.f; E.Falloff=2.3f; break;
    case EGlintElementType::ArcReflection:
        E.Label=TEXT("Arc Reflection"); E.AxisPosition=-1.65f; E.Size=1.15f; E.Intensity=0.14f;
        E.RingThickness=0.09f; E.Color=FLinearColor(0.8f,0.87f,1.f); break;
    case EGlintElementType::EdgeRing:
        E.Label=TEXT("Outer Edge Ring"); E.AxisPosition=-1.9f; E.Size=1.4f; E.Intensity=0.12f;
        E.RingThickness=0.04f; E.Color=FLinearColor(0.78f,0.9f,1.f); break;
    case EGlintElementType::SoftDisc:
        E.Label=TEXT("Soft Disc"); E.AxisPosition=-0.8f; E.Size=1.0f; E.Intensity=0.15f;
        E.Falloff=2.8f; E.Color=FLinearColor(0.95f,0.79f,0.62f); break;
    case EGlintElementType::VeilingGlow:
        E.Label=TEXT("Veiling Glow"); E.AxisPosition=0.9f; E.Size=2.5f; E.Intensity=0.08f;
        E.Falloff=1.0f; E.Color=FLinearColor(0.86f,0.92f,1.f); break;
    case EGlintElementType::PetalIris:
        E.Label=TEXT("Petal Iris"); E.AxisPosition=-1.2f; E.Size=1.1f; E.Intensity=0.19f;
        E.RayCount=7; E.RingThickness=0.14f; E.DetailFrequency=7.f; break;
    case EGlintElementType::DustCluster:
        E.Label=TEXT("Dust Cluster"); E.AxisPosition=-0.3f; E.Size=1.5f; E.Intensity=0.08f;
        E.DetailFrequency=12.f; E.SecondaryStrength=0.65f; E.Falloff=2.f; break;
    default: break;
    }
    return E;
}

FGlintElement UGlintFlare::MakePresetElement(EGlintElementType Type, int32 Preset)
{
    FGlintElement E = MakeDefaultElement(Type);
    if (Preset == 1) // Subtle: useful for a physically restrained composition
    {
        E.Label = FName(*FString::Printf(TEXT("Subtle %s"), GetTypeName(Type)));
        E.Intensity *= 0.4f;
        E.Size *= 0.8f;
        E.SecondaryStrength *= 0.65f;
    }
    else if (Preset == 2) // Cinematic: authored PER OPTICAL FAMILY, not one universal multiplier.
    {
        E.Label = FName(*FString::Printf(TEXT("Cinematic %s"), GetTypeName(Type)));
        switch (Type)
        {
        case EGlintElementType::Glow:
        case EGlintElementType::Halo:
            E.Intensity *= 1.12f; E.Size *= 1.10f; E.Falloff *= 0.85f;
            E.RingThickness *= 1.25f; break;
        case EGlintElementType::VeilingGlow:
            // A veiling glare must remain behind the subject, not white-out the frame.
            E.Intensity *= 1.04f; E.Size *= 0.88f; E.Falloff *= 1.2f; break;
        case EGlintElementType::Starburst:
        case EGlintElementType::SpikeBall:
        case EGlintElementType::Sparkle:
        case EGlintElementType::CrossStar:
            E.Intensity *= 1.22f; E.Size *= 1.14f; E.RaySharpness *= 0.87f;
            E.SecondaryStrength *= 1.05f; break;
        case EGlintElementType::Streak:
        case EGlintElementType::VerticalStreak:
        case EGlintElementType::DualStreak:
            E.Intensity *= 1.16f; E.Size *= 1.16f; E.RaySharpness *= 0.85f; break;
        case EGlintElementType::Ring:
        case EGlintElementType::Hoop:
        case EGlintElementType::MultiIris:
        case EGlintElementType::EdgeRing:
        case EGlintElementType::PolygonIris:
        case EGlintElementType::ApertureBladeRing:
        case EGlintElementType::ArcReflection:
        case EGlintElementType::PetalIris:
            E.Intensity *= 1.12f; E.Size *= 1.08f;
            E.RingThickness = FMath::Min(1.f, E.RingThickness * 1.60f);
            E.SecondaryStrength *= 0.90f; break;
        case EGlintElementType::Ghost:
        case EGlintElementType::Iris:
        case EGlintElementType::ApertureGhost:
        case EGlintElementType::SoftDisc:
            E.Intensity *= 1.14f; E.Size *= 1.1f; E.Falloff *= 0.83f; break;
        case EGlintElementType::DustCluster:
            E.Intensity *= 1.08f; E.Size *= 0.95f;
            E.SecondaryStrength *= 0.75f; E.DetailFrequency *= 0.85f; break;
        case EGlintElementType::Shimmer:
        case EGlintElementType::Caustic:
            E.Intensity *= 1.08f; E.Size *= 1.1f; E.SecondaryStrength *= 0.72f;
            E.DetailFrequency *= 0.8f; break;
        case EGlintElementType::Smear:
            E.Intensity *= 1.16f; E.Size *= 1.13f; E.AspectRatio *= 1.12f; break;
        default:
            E.Intensity *= 1.1f; break;
        }
        E.Intensity = FMath::Min(3.f, E.Intensity);
        E.Size = FMath::Clamp(E.Size, 0.05f, 8.f);
    }
    return E;
}
