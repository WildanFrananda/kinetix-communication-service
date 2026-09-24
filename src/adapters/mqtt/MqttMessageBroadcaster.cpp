#include "adapters/mqtt/MqttMessageBroadcaster.hpp"

#include <utility>

#include <boost/asio/post.hpp>
#include <json/json.h>
#include <trantor/utils/Logger.h>

#include "domain/CommunicationError.hpp"

#include "adapters/wire/Json.hpp"

namespace communication::adapters::mqtt {

using domain::CommunicationError;

MqttMessageBroadcaster::MqttMessageBroadcaster(Settings settings)
    : settings_(std::move(settings)),
      keep_running_(boost::asio::make_work_guard(io_)),
      client_(::MQTT_NS::make_sync_client(io_, settings_.host, settings_.port)) {}

MqttMessageBroadcaster::~MqttMessageBroadcaster() {
    Stop();
}

bool MqttMessageBroadcaster::Start() {
    client_->set_client_id(settings_.client_id);

    client_->set_clean_session(true);

    client_->set_connack_handler([](bool session_present, ::MQTT_NS::connect_return_code code) {
        if (code != ::MQTT_NS::connect_return_code::accepted) {
            LOG_ERROR << "mqtt broker refused the connection: " << ::MQTT_NS::connect_return_code_to_str(code);
            return false;
        }
        LOG_INFO << "mqtt connected (session_present=" << session_present << ")";
        return true;
    });

    client_->set_close_handler([] { LOG_WARN << "mqtt connection closed"; });

    client_->set_error_handler([](const ::MQTT_NS::error_code& error) {
        LOG_ERROR << "mqtt connection failed: " << error.message();
    });

    client_->set_puback_handler([this](PacketId id) {
        if (const auto waiting = awaiting_.find(id); waiting != awaiting_.end()) {
            waiting->second->set_value(true);
            awaiting_.erase(waiting);
        }
        return true;
    });

    try {
        client_->connect();
    } catch (const std::exception& e) {
        LOG_ERROR << "could not reach the mqtt broker at " << settings_.host << ':' << settings_.port << ": " << e.what();
        return false;
    }

    network_ = std::thread([this] {
        io_.run();
    });

    return true;
}

void MqttMessageBroadcaster::Stop() {
    if (!network_.joinable()) {
        return;
    }

    boost::asio::post(io_, [this] {
        try {
            client_->disconnect();
        } catch (const std::exception& e) {
            LOG_WARN << "mqtt disconnect was not clean: " << e.what();
        }
        for (auto& [id, promise] : awaiting_) {
            promise->set_value(false);
        }
        awaiting_.clear();
    });
    keep_running_.reset();
    network_.join();
}

std::string MqttMessageBroadcaster::TopicFor(const domain::PrincipalId& principal) const {
    return settings_.topic_prefix + "/principals/" + principal.Value() + "/messages";
}

std::future<bool> MqttMessageBroadcaster::PublishTo(std::string topic, std::string payload) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto done = promise->get_future();

    boost::asio::post(
        io_,
        [this, topic = std::move(topic), payload = std::move(payload), promise = std::move(promise)]()
        mutable {
            try {
                const auto id = client_->publish(std::move(topic), std::move(payload), ::MQTT_NS::qos::at_least_once);
                awaiting_.emplace(id, std::move(promise));
            } catch (const std::exception& e) {
                LOG_ERROR << "mqtt publish was refused: " << e.what();
                promise->set_value(false);
            }
        }
    );

    return done;
}

std::expected<void, CommunicationError> MqttMessageBroadcaster::Publish(
    const domain::Conversation& conversation,
    const domain::Message& message
) {
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    const std::string payload = Json::writeString(writer, wire::ToJson(message));

    auto first = PublishTo(TopicFor(conversation.First()), payload);
    auto second = PublishTo(TopicFor(conversation.Second()), payload);

    const auto deadline = std::chrono::steady_clock::now() + settings_.acknowledgement_timeout;
    const bool first_ok = first.wait_until(deadline) == std::future_status::ready && first.get();
    const bool second_ok = second.wait_until(deadline) == std::future_status::ready && second.get();

    if (first_ok && second_ok) {
        return {};
    }

    LOG_WARN << "mqtt did not acknowledge message " << message.Id().Value()
        << " for both parties (first=" << first_ok << ", second=" << second_ok << ")";
    return std::unexpected(CommunicationError::kDependencyUnavailable);
}

}  // namespace communication::adapters::mqtt
