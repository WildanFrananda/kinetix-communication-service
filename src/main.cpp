#include <QCoreApplication>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include <drogon/drogon.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <orm/db.hpp>

#include "adapters/grpc/GrpcPrincipalAuthenticator.hpp"
#include "adapters/grpc/GrpcPrincipalDirectory.hpp"
#include "adapters/http/CommunicationController.hpp"
#include "adapters/http/RequestWorkers.hpp"
#include "adapters/mqtt/MqttMessageBroadcaster.hpp"
#include "adapters/persistence/PostgresConversationRepository.hpp"
#include "adapters/persistence/PostgresMessageRepository.hpp"
#include "adapters/system/Settings.hpp"
#include "adapters/system/SystemClock.hpp"
#include "adapters/system/UuidIdentifierSource.hpp"
#include "app/ListConversations.hpp"
#include "app/ListMessages.hpp"
#include "app/OpenConversation.hpp"
#include "app/SendMessage.hpp"

namespace {

constexpr auto kConnection = "communication";

[[nodiscard]] std::string ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

[[nodiscard]] std::shared_ptr<grpc::Channel> IdentityChannel(
    const communication::adapters::system::Settings& settings
) {
    grpc::SslCredentialsOptions tls;
    tls.pem_root_certs = ReadFile(settings.pki_directory + "/ca.pem");
    tls.pem_cert_chain = ReadFile(settings.pki_directory + "/tls.crt");
    tls.pem_private_key = ReadFile(settings.pki_directory + "/tls.key");

    if (tls.pem_root_certs.empty() || tls.pem_cert_chain.empty() || tls.pem_private_key.empty()) {
        LOG_FATAL << "the PKI directory " << settings.pki_directory << " does not hold ca.pem, tls.crt and tls.key";
        return nullptr;
    }

    return grpc::CreateChannel(settings.identity_endpoint, grpc::SslCredentials(tls));
}

[[nodiscard]] bool OpenDatabase(const std::string& url) {
    const QUrl parsed(QString::fromStdString(url));
    if (!parsed.isValid() || parsed.host().isEmpty()) {
        LOG_FATAL << "COMMUNICATION_DATABASE_URL is not a usable URL";
        return false;
    }

    Orm::DB::create(
        {
            {"driver", QStringLiteral("QPSQL")},
            {"host", parsed.host()},
            {"port", parsed.port(5432)},
            {"database", parsed.path().mid(1)},
            {"username", parsed.userName()},
            {"password", parsed.password()},
            {"charset", QStringLiteral("utf8")},
            {"prefix", QString()},
        },
        QString::fromLatin1(kConnection)
    );

    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    const QCoreApplication qt(argc, argv);

    const auto settings = communication::adapters::system::Settings::FromEnvironment();
    if (!settings) {
        LOG_FATAL << settings.error();
        return EXIT_FAILURE;
    }

    if (!OpenDatabase(settings->database_url)) {
        return EXIT_FAILURE;
    }

    const auto identity = IdentityChannel(*settings);
    if (!identity) {
        return EXIT_FAILURE;
    }

    auto clock = std::make_shared<communication::adapters::system::SystemClock>();
    auto identifiers = std::make_shared<communication::adapters::system::UuidIdentifierSource>();

    auto conversations =
        std::make_shared<communication::adapters::persistence::PostgresConversationRepository>(
            QString::fromLatin1(kConnection)
        );
    auto messages = std::make_shared<communication::adapters::persistence::PostgresMessageRepository>(
        QString::fromLatin1(kConnection)
    );

    auto directory = std::make_shared<communication::adapters::grpc::GrpcPrincipalDirectory>(
        identity, settings->identity_deadline
    );

    auto authenticator = std::make_shared<communication::adapters::grpc::GrpcPrincipalAuthenticator>(
        identity, settings->identity_deadline
    );

    auto broadcaster = std::make_shared<communication::adapters::mqtt::MqttMessageBroadcaster>(
        communication::adapters::mqtt::MqttMessageBroadcaster::Settings{
            .host = settings->mqtt_host,
            .port = settings->mqtt_port,
            .client_id = settings->mqtt_client_id,
            .topic_prefix = settings->mqtt_topic_prefix,
            .acknowledgement_timeout = settings->mqtt_acknowledgement_timeout,
        }
    );

    if (!broadcaster->Start()) {
        return EXIT_FAILURE;
    }

    auto open =
        std::make_shared<communication::app::OpenConversation>(conversations, directory, identifiers, clock);
    auto send = std::make_shared<communication::app::SendMessage>(
        conversations,
        messages,
        broadcaster,
        identifiers,
        clock
    );

    auto list_messages = std::make_shared<communication::app::ListMessages>(conversations, messages);
    auto list_conversations = std::make_shared<communication::app::ListConversations>(conversations);

    auto workers = std::make_shared<communication::adapters::http::RequestWorkers>(settings->worker_threads);
    auto controller = std::make_shared<communication::adapters::http::CommunicationController>(
        authenticator,
        open,
        send,
        list_messages,
        list_conversations,
        workers
    );

    drogon::app()
        .setLogLevel(trantor::Logger::kInfo)
        .addListener("0.0.0.0", settings->http_port)
        .setThreadNum(0)  // one event loop per core; the blocking work is on RequestWorkers
        .registerController(controller)
        .registerBeginningAdvice([port = settings->http_port] {
            LOG_INFO << "kinetix-communication-service listening on " << port;
        });

    drogon::app().run();

    broadcaster->Stop();
    return EXIT_SUCCESS;
}
