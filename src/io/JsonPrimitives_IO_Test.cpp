// JsonPrimitives_IO_Test.cpp — acceptance test for the five transform primitives
// (`RenameKey`/`MoveKey`/`WrapScalarAsVector`/`DefaultIfMissing`/`DeleteKeyIfPresent`), including
// the idempotency check IO_MIGRATION_SPEC.md §5 requires (a second call is always safe) for at
// least `DeleteKeyIfPresent`/`RenameKey`. Does not re-test the relocated `ReadJson*` typed
// accessors — those already have live coverage through every existing `MapImporter_*_IO.cpp`
// round-trip test; this file's job is the NEW transform primitives.
#include "JsonPrimitives_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

void CheckRenameKey() {
    nlohmann::json object = { {"oldName", 5} };
    Io::RenameKey(object, "oldName", "newName");
    Check(!object.contains("oldName"), "RenameKey removes the old key");
    Check(object.contains("newName") && object["newName"] == 5, "RenameKey moves the value");

    // No-op when the old key is absent.
    nlohmann::json missing = { {"other", 1} };
    Io::RenameKey(missing, "absent", "target");
    Check(!missing.contains("target"), "RenameKey is a no-op when oldKey is absent");

    // Idempotency: calling it a second time (oldKey is now absent) must leave the object unchanged.
    nlohmann::json again = object;
    Io::RenameKey(again, "oldName", "newName");
    Check(again == object, "RenameKey is idempotent: a second call changes nothing");
}

void CheckMoveKey() {
    nlohmann::json source      = { {"field", 42} };
    nlohmann::json destination = nlohmann::json::object();
    Io::MoveKey(source, "field", destination, "movedField");
    Check(!source.contains("field"), "MoveKey removes the value from the source");
    Check(destination.contains("movedField") && destination["movedField"] == 42,
          "MoveKey places the value at the destination key");

    // No-op when the source key is absent.
    nlohmann::json emptySource = nlohmann::json::object();
    nlohmann::json untouched   = nlohmann::json::object();
    Io::MoveKey(emptySource, "absent", untouched, "target");
    Check(!untouched.contains("target"), "MoveKey is a no-op when sourceKey is absent");
}

void CheckWrapScalarAsVector() {
    nlohmann::json object = { {"value", 7} };
    Io::WrapScalarAsVector(object, "value");
    Check(object["value"].is_array() && object["value"].size() == 1 && object["value"][0] == 7,
          "WrapScalarAsVector replaces the scalar with a single-element array containing it");

    // Idempotency: a second call on an already-wrapped key must not double-wrap into [[x]].
    Io::WrapScalarAsVector(object, "value");
    Check(object["value"].is_array() && object["value"].size() == 1 && object["value"][0] == 7,
          "WrapScalarAsVector is idempotent: a second call does not double-wrap");

    // No-op when the key is absent.
    nlohmann::json empty = nlohmann::json::object();
    Io::WrapScalarAsVector(empty, "absent");
    Check(!empty.contains("absent"), "WrapScalarAsVector is a no-op when the key is absent");
}

void CheckDefaultIfMissing() {
    nlohmann::json object = nlohmann::json::object();
    Io::DefaultIfMissing(object, "key", 3);
    Check(object.contains("key") && object["key"] == 3, "DefaultIfMissing sets an absent key");

    Io::DefaultIfMissing(object, "key", 99);
    Check(object["key"] == 3, "DefaultIfMissing never overwrites a value already present");
}

void CheckDeleteKeyIfPresent() {
    nlohmann::json object = { {"toDelete", 1}, {"keep", 2} };
    Io::DeleteKeyIfPresent(object, "toDelete");
    Check(!object.contains("toDelete"), "DeleteKeyIfPresent erases the key");
    Check(object.contains("keep"), "DeleteKeyIfPresent leaves other keys untouched");

    // Idempotency: calling it again (already absent) must be a safe no-op.
    nlohmann::json before = object;
    Io::DeleteKeyIfPresent(object, "toDelete");
    Check(object == before, "DeleteKeyIfPresent is idempotent: a second call changes nothing");
}

} // namespace

int main() {
    CheckRenameKey();
    CheckMoveKey();
    CheckWrapScalarAsVector();
    CheckDefaultIfMissing();
    CheckDeleteKeyIfPresent();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
