#include "windowCreationUtils.h"

#include <atomic>

namespace utils {
static std::atomic<uint64_t> sNextHandleId{1};

VeraWindowHandle generateUniqueHandle() {
    uint64_t id = sNextHandleId.fetch_add(1, std::memory_order_relaxed);
    return VeraWindowHandle{id};
}
}  // namespace utils