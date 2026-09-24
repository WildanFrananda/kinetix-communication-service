#pragma once

#include <chrono>
#include <cstdint>
#include <expected>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <mqtt_client_cpp.hpp>

#include "ports/IMessageBroadcaster.hpp"

namespace communication::adapters::mqtt {

class MqttMessageBroadcaster final : public ports::IMessageBroadcaster {
  public:
    struct Settings {
        std::string host;
        std::uint16_t port;
        std::string client_id;
        std::string topic_prefix;
        std::chrono::milliseconds acknowledgement_timeout;
    };

    explicit MqttMessageBroadcaster(Settings settings);
    ~MqttMessageBroadcaster() override;

    [[nodiscard]] bool Start();

    void Stop();

    [[nodiscard]] std::expected<void, domain::CommunicationError> Publish(
        const domain::Conversation& conversation,
        const domain::Message& message
    ) override;

  private:
    using Client = decltype(::MQTT_NS::make_sync_client(
        std::declval<boost::asio::io_context&>(),
        std::string(),
        std::uint16_t()
    ));

    using PacketId = Client::element_type::packet_id_t;

    [[nodiscard]] std::string TopicFor(const domain::PrincipalId& principal) const;

    [[nodiscard]] std::future<bool> PublishTo(std::string topic, std::string payload);

    Settings settings_;
    boost::asio::io_context io_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> keep_running_;
    Client client_;
    std::thread network_;

    std::map<PacketId, std::shared_ptr<std::promise<bool>>> awaiting_;
};

}  // namespace communication::adapters::mqtt
