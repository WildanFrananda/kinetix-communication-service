#pragma once

#include <chrono>
#include <cstdint>
#include <expected>
#include <string>

namespace communication::adapters::system {

struct Settings {
    std::string database_url;
    std::string identity_endpoint;
    std::chrono::milliseconds identity_deadline{1500};
    std::string mqtt_host;
    std::uint16_t mqtt_port{1883};
    std::string mqtt_client_id{"kinetix-communication"};
    std::string mqtt_topic_prefix{"kinetix/communication"};
    std::chrono::milliseconds mqtt_acknowledgement_timeout{2000};
    std::uint16_t http_port{8087};
    std::size_t worker_threads{16};
    std::string pki_directory;

    [[nodiscard]] static std::expected<Settings, std::string> FromEnvironment();
};

}  // namespace communication::adapters::system
