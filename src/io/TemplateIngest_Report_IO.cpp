// TemplateIngest_Report_IO.cpp — private ARCH §1.5 aspect split off TemplateIngest_IO.cpp:
// TemplateIngestReport's own read-only accessors and SummaryText. Every method here is already
// declared on the class in TemplateIngest_IO.h -- this file needs no header of its own.
#include "TemplateIngest_IO.h"

namespace SanmapGen {
namespace Io {

const TemplateFootprintRecord*
TemplateIngestReport::FindByTemplateIdentifier(const std::string& templateIdentifier) const {
    const auto found = footprintByTemplateIdentifier.find(templateIdentifier);
    return found != footprintByTemplateIdentifier.end() ? &found->second : nullptr;
}

const std::vector<std::string>*
TemplateIngestReport::FindTagsByTemplateIdentifier(const std::string& templateIdentifier) const {
    const auto found = tagsByTemplateIdentifier.find(templateIdentifier);
    return found != tagsByTemplateIdentifier.end() ? &found->second : nullptr;
}

// House warning shape (STEP73 §0 / STEP82 precedent): one aggregate block, loud, non-blocking,
// names every skip/collision category, never auto-fixes.
std::string TemplateIngestReport::SummaryText() const {
    std::string text = "Ingested " + std::to_string(ingestedFootprintRecordCount)
        + " template footprint(s), " + std::to_string(tagsByTemplateIdentifier.size())
        + " tags record(s), from " + std::to_string(totalSourceFileCount) + " source file(s)"
        + (bLoadedFromDiskCache ? " (loaded from disk cache)." : " (cold ingest, disk cache written).");

    if (skippedProjectileOrMarkerCount > 0)
        text += " " + std::to_string(skippedProjectileOrMarkerCount)
              + " projectile/marker template(s) skipped by design.";
    if (skippedUnrecognizedCount > 0)
        text += " " + std::to_string(skippedUnrecognizedCount)
              + " file(s) had no recognized root table (see log).";
    if (skippedMissingFootprintCount > 0)
        text += " " + std::to_string(skippedMissingFootprintCount)
              + " record(s) recognized but missing a footprint field (see log).";
    if (skippedOversizeFileCount > 0)
        text += " " + std::to_string(skippedOversizeFileCount) + " oversize file(s) never opened.";
    if (skippedUnreadableFileCount > 0)
        text += " " + std::to_string(skippedUnreadableFileCount) + " unreadable file(s) skipped.";

    if (tpIdCollisions.AnyCollisions()) {
        text += " " + std::to_string(tpIdCollisions.collisions.size()) + " tpId collision(s) found:";
        for (const TpIdCollision& collision : tpIdCollisions.collisions) {
            text += "\n  \"" + collision.templateIdentifier + "\": ";
            for (std::size_t index = 0; index < collision.conflictingSourcePaths.size(); ++index) {
                if (index > 0) text += ", ";
                text += collision.conflictingSourcePaths[index];
            }
            text += " (first source wins)";
        }
    }
    return text;
}

} // namespace Io
} // namespace SanmapGen
