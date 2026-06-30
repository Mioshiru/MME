#include "main.h"
#include "items.h"

namespace {
struct ItemRegistryStub {
    ItemRegistryStub() = default;
};
}

// This translation unit intentionally stays empty because the actual item registry
// implementation lives in items.cpp and the current headers expose ItemDatabase.
// Keeping it as a minimal stub avoids the broken standalone implementations that
// were previously introduced here.