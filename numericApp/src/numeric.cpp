#include "numeric.h"
#include <gsl/gsl_errno.h>
namespace numeric {
void initializeGsl() { gsl_set_error_handler_off(); }
bool Registry::add(const Metadata& metadata, Factory factory) {
    if (!metadata.name || !*metadata.name || !factory || find(metadata.name)) return false;
    entries_.push_back(Entry{metadata, factory});
    return true;
}
const Metadata* Registry::find(const std::string& name) const {
    for (const auto& entry : entries_) if (name == entry.metadata.name) return &entry.metadata;
    return nullptr;
}
std::unique_ptr<Operation> Registry::create(const std::string& name) const {
    for (const auto& entry : entries_) if (name == entry.metadata.name) return entry.factory();
    return std::unique_ptr<Operation>();
}
}
