// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NUMERIC_ACK_H
#define NUMERIC_ACK_H
#include "numeric.h"
#include <array>
#include <cstdint>

namespace numeric {
enum class EventKind { Sample, Reset };
struct Event { std::uint64_t session; std::uint64_t sequence; EventKind kind; double value; };
struct Acknowledgment { Event event; std::array<double, 7> statistics; bool replayed; };

// One writer at a time; callers must serialize all access to an instance.
class NUMERIC_API AcknowledgedStream {
public:
    static std::unique_ptr<AcknowledgedStream> create();
    Result submit(const Event& event, Acknowledgment& acknowledgment);
private:
    explicit AcknowledgedStream(std::unique_ptr<Operation> operation);
    std::unique_ptr<Operation> operation_;
    Acknowledgment last_{};
    bool hasLast_ = false;
};
}
#endif
