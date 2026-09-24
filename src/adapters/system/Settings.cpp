#include "adapters/system/Settings.hpp"

#include <cstdlib>
#include <string_view>
#include <vector>

namespace communication::adapters::system {
namespace {

[[nodiscard]] std::string Read(const char* name) {
    const char* value = std::getenv(name);
    return value == nullptr ? std::string() : std::string(value);
}

[[nodiscard]] std::string ReadOr(const char* name, std::string fallback) {
    auto value = Read(name);
    return value.empty() ? std::move(fallback) : value;
}

[[nodiscard]] long long ReadNumberOr(const char* name, long long fallback) {
    const auto raw = Read(name);
    if (raw.empty()) {
        return fallback;
    }
    try {
        return std::stoll(raw);
    } catch (const std::exception&) {
        return fallback;
    }
}

}  // namespace

std::expected<Settings, std::string> Settings::FromEnvironment() {
    Settings settings;
    std::vector<std::string_view> missing;

    settings.database_url = Read("COMMUNICATION_DATABASE_URL");
    if (settings.database_url.empty()) {
        missing.emplace_back("COMMUNICATION_DATABASE_URL");
    }

    settings.identity_endpoint = Read("COMMUNICATION_IDENTITY_ENDPOINT");
    if (settings.identity_endpoint.empty()) {
        missing.emplace_back("COMMUNICATION_IDENTITY_ENDPOINT");
    }

    settings.mqtt_host = Read("COMMUNICATION_MQTT_HOST");
    if (settings.mqtt_host.empty()) {
        missing.emplace_back("COMMUNICATION_MQTT_HOST");
    }

    settings.pki_directory = Read("KINETIX_PKI_DIR");
    if (settings.pki_directory.empty()) {
        missing.emplace_back("KINETIX_PKI_DIR");
    }

    if (!missing.empty()) {
        std::string complaint = "these environment variables must be set:";
        for (const auto& name : missing) {
            complaint += "\n  ";
            complaint += name;
        }
        return std::unexpected(complaint);
    }

    settings.identity_deadline =
        std::chrono::milliseconds(ReadNumberOr("COMMUNICATION_IDENTITY_DEADLINE_MS", 1500));
    settings.mqtt_port = static_cast<std::uint16_t>(ReadNumberOr("COMMUNICATION_MQTT_PORT", 1883));
    settings.mqtt_client_id = ReadOr("COMMUNICATION_MQTT_CLIENT_ID", "kinetix-communication");
    settings.mqtt_topic_prefix = ReadOr("COMMUNICATION_MQTT_TOPIC_PREFIX", "kinetix/communication");
    settings.mqtt_acknowledgement_timeout =
        std::chrono::milliseconds(ReadNumberOr("COMMUNICATION_MQTT_ACK_TIMEOUT_MS", 2000));
    settings.http_port = static_cast<std::uint16_t>(ReadNumberOr("COMMUNICATION_HTTP_PORT", 8087));
    settings.worker_threads = static_cast<std::size_t>(ReadNumberOr("COMMUNICATION_WORKER_THREADS", 16));

    return settings;
}

}  // namespace communication::adapters::system
