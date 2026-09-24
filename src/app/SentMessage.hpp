#pragma once

#include "domain/Message.hpp"

namespace communication::app {

struct SentMessage {
    domain::Message message;
    bool broadcast;
};

}  // namespace communication::app
