// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NUMERIC_ACK_CLIENT_H
#define NUMERIC_ACK_CLIENT_H
#include "numericAck.h"
#include <array>
#include <string>

namespace numeric {
enum class SendStatus { Acknowledged, Rejected, Uncertain, InvalidArgument };
struct SendResult { SendStatus status; std::array<double, 7> statistics{}; };
// Call from a thread without a CA context. The caller must serialize producers.
SendResult sendAcknowledged(const std::string& eventPv, const std::string& record,
                            const Event& event, unsigned attempts, double timeout);
}
#endif
