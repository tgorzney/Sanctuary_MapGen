// AppSettingsLocation_IO.h — the one new platform touchpoint AppSettings_IO needs: where the
// bootstrap settings file lives. Layer: IO / BRIDGE — this IS the platform seam (Constitution §5,
// ARCH §3.3), a direct structural twin of FileDialog_IO.h/FileDialog_Shell_IO.cpp's split
// (STEP19_AppSettings_IO Design item 6). Deliberately NOT named with the `MapExporter_<Domain>_IO`
// pairing — that convention is reserved for `.sanmap` top-level sections (IO_MIGRATION_SPEC.md
// §1's own definition of `Domain`); this file is outside that system entirely.
//
// A fixed, always-resolvable anchor: it solves the chicken-and-egg problem a fully user-chosen
// settings folder would create (you cannot ask the user where settings live before you have
// loaded settings). A larger, user-relocatable data folder — if ever wanted — becomes ONE FIELD
// inside the file this directory holds, not the file's own location.
#pragma once
#include <string>

namespace SanmapGen {
namespace Io {

// `%APPDATA%\SanGen\` on Windows (FOLDERID_RoamingAppData via SHGetKnownFolderPath). Never
// throws; an empty string means the platform seam is not compiled in or the shell call failed, so
// a caller degrades to AppSettings' compiled defaults rather than crashing (Constitution §6).
std::string DefaultAppSettingsDirectory();

} // namespace Io
} // namespace SanmapGen
