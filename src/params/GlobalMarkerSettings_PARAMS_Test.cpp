// GlobalMarkerSettings_PARAMS_Test.cpp — acceptance test for STEP116's
// ResolveMarkerGroupTypeTintColor: a pure, additive helper that maps a manual marker group's
// free-form name to the matching GlobalMarkerSettings default color, mirroring
// ResolveMarkerIconTemplateIdentifier's own reserved-literal-plus-singular/plural vocabulary.
// Header must compile standalone: this file includes nothing but the header + STL.
#include "GlobalMarkerSettings_PARAMS.h"
#include <cstdio>

using namespace SanmapGen::Params;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

GlobalMarkerSettings MakeNonDefaultSettings() {
    GlobalMarkerSettings settings;
    settings.colorSpawn[0] = 0.8f; settings.colorSpawn[1] = 0.2f; settings.colorSpawn[2] = 0.2f;
    settings.colorAlloy[0] = 0.8f; settings.colorAlloy[1] = 0.8f; settings.colorAlloy[2] = 0.2f;
    settings.colorPlasma[0] = 0.2f; settings.colorPlasma[1] = 0.8f; settings.colorPlasma[2] = 0.8f;
    settings.selectColorSpawn[0] = 1.0f; settings.selectColorSpawn[1] = 0.4f; settings.selectColorSpawn[2] = 0.4f;
    settings.selectColorAlloy[0] = 1.0f; settings.selectColorAlloy[1] = 1.0f; settings.selectColorAlloy[2] = 0.4f;
    settings.selectColorPlasma[0] = 0.4f; settings.selectColorPlasma[1] = 1.0f; settings.selectColorPlasma[2] = 1.0f;
    settings.selectColorDefault[0] = 1.0f; settings.selectColorDefault[1] = 0.6f; settings.selectColorDefault[2] = 0.0f;
    return settings;
}

} // namespace

int main() {
    // STEP127 item 3 — scaleAlloy/Plasma/Spawn's default initializer, was 0.17f, now 0.50f.
    const GlobalMarkerSettings defaultSettings;
    Check(defaultSettings.scaleAlloy == 0.50f && defaultSettings.scalePlasma == 0.50f
          && defaultSettings.scaleSpawn == 0.50f,
          "GlobalMarkerSettings::scaleAlloy/Plasma/Spawn default to 0.50f (STEP127, was 0.17f)");

    const GlobalMarkerSettings settings = MakeNonDefaultSettings();
    float red = -1.0f, green = -1.0f, blue = -1.0f;

    // Spawn/Spawns resolve colorSpawn.
    ResolveMarkerGroupTypeTintColor(kSpawnMarkerGroupName, settings, red, green, blue);
    Check(red == settings.colorSpawn[0] && green == settings.colorSpawn[1] && blue == settings.colorSpawn[2],
          "\"Spawn\" resolves colorSpawn");
    ResolveMarkerGroupTypeTintColor("Spawns", settings, red, green, blue);
    Check(red == settings.colorSpawn[0] && green == settings.colorSpawn[1] && blue == settings.colorSpawn[2],
          "\"Spawns\" (plural) resolves colorSpawn");

    // Alloy/Alloys resolve colorAlloy.
    ResolveMarkerGroupTypeTintColor("Alloy", settings, red, green, blue);
    Check(red == settings.colorAlloy[0] && green == settings.colorAlloy[1] && blue == settings.colorAlloy[2],
          "\"Alloy\" resolves colorAlloy");
    ResolveMarkerGroupTypeTintColor("Alloys", settings, red, green, blue);
    Check(red == settings.colorAlloy[0] && green == settings.colorAlloy[1] && blue == settings.colorAlloy[2],
          "\"Alloys\" (plural) resolves colorAlloy");

    // Plasma/Plasmas resolve colorPlasma.
    ResolveMarkerGroupTypeTintColor("Plasma", settings, red, green, blue);
    Check(red == settings.colorPlasma[0] && green == settings.colorPlasma[1] && blue == settings.colorPlasma[2],
          "\"Plasma\" resolves colorPlasma");
    ResolveMarkerGroupTypeTintColor("Plasmas", settings, red, green, blue);
    Check(red == settings.colorPlasma[0] && green == settings.colorPlasma[1] && blue == settings.colorPlasma[2],
          "\"Plasmas\" (plural) resolves colorPlasma");

    // Generic/Expansion/an arbitrary freeform name all resolve white, regardless of the fixture's
    // non-default settings values — proves no accidental match.
    ResolveMarkerGroupTypeTintColor("Generic", settings, red, green, blue);
    Check(red == 1.0f && green == 1.0f && blue == 1.0f, "\"Generic\" resolves white");
    ResolveMarkerGroupTypeTintColor("Expansion", settings, red, green, blue);
    Check(red == 1.0f && green == 1.0f && blue == 1.0f, "\"Expansion\" resolves white");
    ResolveMarkerGroupTypeTintColor("SomeFreeformGroupName", settings, red, green, blue);
    Check(red == 1.0f && green == 1.0f && blue == 1.0f, "an arbitrary freeform name resolves white");

    // ARCH §19.17: ResolveMarkerGroupSelectTintColor mirrors the same name-matching vocabulary, but
    // an unmatched name resolves selectColorDefault, NOT white.
    ResolveMarkerGroupSelectTintColor(kSpawnMarkerGroupName, settings, red, green, blue);
    Check(red == settings.selectColorSpawn[0] && green == settings.selectColorSpawn[1]
          && blue == settings.selectColorSpawn[2], "\"Spawn\" resolves selectColorSpawn");
    ResolveMarkerGroupSelectTintColor("Alloys", settings, red, green, blue);
    Check(red == settings.selectColorAlloy[0] && green == settings.selectColorAlloy[1]
          && blue == settings.selectColorAlloy[2], "\"Alloys\" (plural) resolves selectColorAlloy");
    ResolveMarkerGroupSelectTintColor("Plasma", settings, red, green, blue);
    Check(red == settings.selectColorPlasma[0] && green == settings.selectColorPlasma[1]
          && blue == settings.selectColorPlasma[2], "\"Plasma\" resolves selectColorPlasma");
    // The deviation this ticket exists to prove: unmatched names resolve selectColorDefault, NOT
    // white — unlike ResolveMarkerGroupTypeTintColor's own fallback, exercised just above in this
    // same file for the identical group names.
    ResolveMarkerGroupSelectTintColor("Generic", settings, red, green, blue);
    Check(red == settings.selectColorDefault[0] && green == settings.selectColorDefault[1]
          && blue == settings.selectColorDefault[2],
          "\"Generic\" resolves selectColorDefault, NOT white (ARCH §19.17's signed-off deviation)");
    ResolveMarkerGroupSelectTintColor("SomeFreeformGroupName", settings, red, green, blue);
    Check(red == settings.selectColorDefault[0] && green == settings.selectColorDefault[1]
          && blue == settings.selectColorDefault[2],
          "an arbitrary freeform name also resolves selectColorDefault");

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
