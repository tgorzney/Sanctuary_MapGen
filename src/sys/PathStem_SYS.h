// PathStem_SYS.h — Sys::FileStemFromPath, header-only, mirroring LuaTableValue_SYS.h's existing
// small-primitive shape. Layer: SYS.
//
// Promoted here (STEP115_MarkerPropDecalLayerReconciliationOnImport_IO) because this exact
// algorithm was independently written twice already (TemplateIdentifierFromBlueprintPath,
// MapCanvas_IconLayer_CullManual_UI.cpp:47-53; FileStemOfEntryName, Application_Assets_UI.cpp:23-30)
// and this ticket needed it a third/fourth time — crossing this codebase's own "promote at the third
// occurrence" threshold. Both IO and UI may depend on SYS (ARCH §3.1) — no boundary violation.
// Migrating the two existing UI-layer copies onto this primitive is a recommended, low-risk
// follow-up, NOT part of this ticket's scope — both are left exactly as they are.
#pragma once
#include <string>

namespace SanmapGen {
namespace Sys {

// "Props/Rock/Rock01.santp" -> "Rock01". Strips up to the last '/' or '\\' and the trailing
// extension.
inline std::string FileStemFromPath(const std::string& path) {
    const std::size_t lastSeparator = path.find_last_of("/\\");
    const std::size_t stemBegin = lastSeparator == std::string::npos ? 0 : lastSeparator + 1;
    const std::size_t lastDot = path.find_last_of('.');
    const std::size_t stemEnd =
        (lastDot == std::string::npos || lastDot < stemBegin) ? path.size() : lastDot;
    return path.substr(stemBegin, stemEnd - stemBegin);
}

} // namespace Sys
} // namespace SanmapGen
