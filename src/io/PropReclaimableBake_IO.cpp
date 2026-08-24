#include "PropReclaimableBake_IO.h"
#include "TemplateDialect_IO.h"                 // Io::DeriveTemplateIdentifierFromPath
#include "../params/ScatterRule_PARAMS.h"       // Params::PropRule
#include "../params/PropInstance_PARAMS.h"      // Params::PropInstanceGroup

namespace SanmapGen {
namespace Io {

bool TemplateHasHarvestableTag(const TemplateIngestReport& report, const std::string& templateIdentifier) {
    const std::vector<std::string>* tags = report.FindTagsByTemplateIdentifier(templateIdentifier);
    if (tags == nullptr) return false;
    for (const std::string& tag : *tags) if (tag == "HARVESTABLE") return true;
    return false;
}

bool BakeReclaimableForPropRule(const TemplateIngestReport& report, Params::PropRule& rule) {
    const std::string templateIdentifier(rule.transform.templateIdentifier);
    if (report.FindTagsByTemplateIdentifier(templateIdentifier) == nullptr) return false;
    rule.bReclaimable = TemplateHasHarvestableTag(report, templateIdentifier);
    return true;
}

bool BakeReclaimableForPropInstanceGroup(const TemplateIngestReport& report, Params::PropInstanceGroup& group) {
    const std::string templateIdentifier = DeriveTemplateIdentifierFromPath(group.blueprintPath);
    if (report.FindTagsByTemplateIdentifier(templateIdentifier) == nullptr) return false;
    group.bReclaimable = TemplateHasHarvestableTag(report, templateIdentifier);
    return true;
}

} // namespace Io
} // namespace SanmapGen
