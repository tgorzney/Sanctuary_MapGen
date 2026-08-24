// TemplateIngestCache_Record_IO.cpp — see TemplateIngestCache_Record_IO.h's own header comment.
#include "TemplateIngestCache_Record_IO.h"
#include "JsonPrimitives_IO.h"
#include <utility>

namespace SanmapGen {
namespace Io {

nlohmann::json RecordToJson(const TemplateRecord& record) {
    nlohmann::json object;
    object["DialectKind"]         = static_cast<int>(record.dialectKind);
    object["TemplateIdentifier"]  = record.templateIdentifier;
    object["TpIdWasDeclared"]     = record.bTpIdWasDeclared;
    object["HasFootprint"]        = record.bHasFootprint;
    object["BaseFootprintWidth"]  = record.baseFootprintWidth;
    object["BaseFootprintDepth"]  = record.baseFootprintDepth;
    object["Tags"]                = record.tags;
    object["SourceLogicalPath"]   = record.sourceLogicalPath;
    object["SourceByteSize"]      = record.sourceByteSize;
    object["SourceModifiedTime"]  = record.sourceModifiedTime;
    object["SourceContentHash"]   = record.sourceContentHash;
    object["SourcePriorityRank"]  = record.sourcePriorityRank;
    return object;
}

bool RecordFromJson(const nlohmann::json& object, TemplateRecord& outRecord) {
    if (!object.is_object()) return false;

    int dialectKindValue = 0;
    if (!ReadJsonEnumeration(object, "DialectKind", 6, dialectKindValue)) return false;   // 6 enumerators
    std::string templateIdentifier;
    if (!ReadJsonText(object, "TemplateIdentifier", templateIdentifier)) return false;
    bool bTpIdWasDeclared = false;
    if (!ReadJsonBoolean(object, "TpIdWasDeclared", bTpIdWasDeclared)) return false;
    bool bHasFootprint = false;
    if (!ReadJsonBoolean(object, "HasFootprint", bHasFootprint)) return false;
    float baseFootprintWidth = 0.0f;
    if (!ReadJsonFloat(object, "BaseFootprintWidth", baseFootprintWidth)) return false;
    float baseFootprintDepth = 0.0f;
    if (!ReadJsonFloat(object, "BaseFootprintDepth", baseFootprintDepth)) return false;
    if (!object.contains("Tags") || !object["Tags"].is_array()) return false;
    std::vector<std::string> tags;
    tags.reserve(object["Tags"].size());
    for (const nlohmann::json& tagValue : object["Tags"]) {
        if (!tagValue.is_string()) return false;
        tags.push_back(tagValue.get<std::string>());
    }
    std::string sourceLogicalPath;
    if (!ReadJsonText(object, "SourceLogicalPath", sourceLogicalPath)) return false;
    std::uint64_t sourceByteSize = 0;
    if (!ReadJsonUnsignedInteger64(object, "SourceByteSize", sourceByteSize)) return false;
    std::uint64_t sourceModifiedTime = 0;
    if (!ReadJsonUnsignedInteger64(object, "SourceModifiedTime", sourceModifiedTime)) return false;
    std::uint64_t sourceContentHash = 0;
    if (!ReadJsonUnsignedInteger64(object, "SourceContentHash", sourceContentHash)) return false;
    int sourcePriorityRank = 0;
    if (!ReadJsonInteger(object, "SourcePriorityRank", sourcePriorityRank)) return false;

    outRecord.dialectKind         = static_cast<TemplateDialectKind>(dialectKindValue);
    outRecord.templateIdentifier  = std::move(templateIdentifier);
    outRecord.bTpIdWasDeclared    = bTpIdWasDeclared;
    outRecord.bHasFootprint       = bHasFootprint;
    outRecord.baseFootprintWidth  = baseFootprintWidth;
    outRecord.baseFootprintDepth  = baseFootprintDepth;
    outRecord.tags                = std::move(tags);
    outRecord.sourceLogicalPath   = std::move(sourceLogicalPath);
    outRecord.sourceByteSize      = sourceByteSize;
    outRecord.sourceModifiedTime  = sourceModifiedTime;
    outRecord.sourceContentHash   = sourceContentHash;
    outRecord.sourcePriorityRank  = sourcePriorityRank;
    return true;
}

} // namespace Io
} // namespace SanmapGen
