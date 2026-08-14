#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "fourier_aurora_sdk/aurora_client.hpp"
#include "fourier_aurora_sdk/transport/fourierdds_transport.hpp"
#include "internal/transport.hpp"
#include "fourier_dds/dds_node.hpp"
#include "fourier_dds/dds_publisher.hpp"
#include "fourier_dds/dds_server.hpp"
#include "fourier_dds/dds_subscription.hpp"

#include "AuroraState/AuroraState.hpp"
#include "AuroraState/AuroraStatePubSubTypes.hpp"
#include "ManageLease/ManageLease.hpp"
#include "ManageLease/ManageLeasePubSubTypes.hpp"
#include "RobotMotionCmd/RobotMotionCmd.hpp"
#include "RobotMotionCmd/RobotMotionCmdPubSubTypes.hpp"
#include "SetInt/SetInt.hpp"
#include "SetInt/SetIntPubSubTypes.hpp"

#include "test_harness.hpp"

namespace sdk = fourier::aurora_sdk;
namespace testing = fourier::aurora_sdk::testing;
namespace dds_msg = fourier_msgs::msg;
namespace dds_srv = fourier_msgs::srv;

namespace {

using namespace std::chrono_literals;

constexpr uint32_t kDomainId = 214;
constexpr const char* kNamespace = "aurora_sdk_loopback";

template <typename Predicate>
bool waitUntil(Predicate&& predicate, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    do {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(10ms);
    } while (std::chrono::steady_clock::now() < deadline);
    return predicate();
}

std::shared_ptr<fourier_dds::DdsNode> makePeerNode(bool use_namespace) {
    auto node = std::make_shared<fourier_dds::DdsNode>(
        kDomainId,
        0,
        std::optional<bool>(false),
        use_namespace,
        std::optional<bool>(false));
    if (use_namespace) {
        node->setNamespace(kNamespace);
    }
    return node;
}

sdk::ConnectionOptions directOptions() {
    sdk::ConnectionOptions options;
    options.domain_id = kDomainId;
    options.namespace_name = kNamespace;
    options.dds_mode = 0;
    options.is_ros_compatible = false;
    options.use_namespace = true;
    options.use_discovery_server = false;
    options.service_timeout_ms = 200;
    options.source_name = "dds-loopback";
    options.enabled_endpoints = {
        sdk::Endpoint::ManageLeaseService,
        sdk::Endpoint::FsmService,
        sdk::Endpoint::MotionCommandPublisher,
        sdk::Endpoint::AuroraStateSubscriber,
    };
    return options;
}

void publishState(
    fourier_dds::DdsPublisher<dds_msg::AuroraStatePubSubType>& publisher,
    uint32_t state_id) {
    static constexpr const char* kResources[] = {
        "left_leg", "right_leg", "waist", "head", "left_manipulator",
        "right_manipulator", "left_end_effector", "right_end_effector",
        "motion_cmd",
    };
    dds_msg::AuroraState state;
    state.robot_info().robot_type("loopback_robot");
    state.robot_info().hardware_type("loopback_hardware");
    state.robot_info().end_effector_type("none");
    state.current_state().id(state_id);
    state.current_state().name("LoopbackState");
    state.current_state().active(true);
    for (const auto* name : kResources) {
        dds_msg::AuthorityResourceState resource;
        resource.resource(name);
        resource.acquired(false);
        resource.holder_source_name("");
        resource.holder_priority(0);
        state.authority_resources().push_back(std::move(resource));
    }
    publisher.publish(state);
}

}  // namespace

int main() {
    testing::TestHarness test;
    SDK_CHECK(test, kDomainId != 123);

    auto peer = makePeerNode(true);
    fourier_dds::DdsPublisher<dds_msg::AuroraStatePubSubType> state_publisher(
        peer, "aurora/aurora_state");
    std::atomic<int> motion_samples{0};
    fourier_dds::DdsSubscription<dds_msg::RobotMotionCmdPubSubType>
        motion_subscription(
            peer,
            "aurora/robot_motion_cmd",
            [&](const dds_msg::RobotMotionCmd&) { ++motion_samples; },
            false,
            fourier_dds::SubscriptionDurability::Volatile);

    std::atomic<int> service_delay_ms{0};
    fourier_dds::DdsServer<
        dds_srv::SetInt_RequestPubSubType,
        dds_srv::SetInt_ResponsePubSubType>
        fsm_server(
            peer,
            "aurora/whole_body_fsm_state_change_service",
            [&](const dds_srv::SetInt_Request& request,
                dds_srv::SetInt_Response& response) {
                const auto delay = service_delay_ms.load();
                if (delay != 0) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(delay));
                }
                response.result(213);
                response.message(
                    "state=" + std::to_string(request.data()));
            });
    fsm_server.run();

    fourier_dds::DdsServer<
        dds_srv::ManageLease_RequestPubSubType,
        dds_srv::ManageLease_ResponsePubSubType>
        lease_server(
            peer,
            "aurora/ManageLease",
            [](const dds_srv::ManageLease_Request& request,
               dds_srv::ManageLease_Response& response) {
                response.header(request.header());
                response.result(dds_srv::LEASE_RESULT_OK);
                response.lease_id(request.action() == dds_srv::LEASE_ACTION_RELEASE
                                      ? request.lease_id()
                                      : 9001);
                response.renew_duration(
                    request.action() == dds_srv::LEASE_ACTION_RELEASE
                        ? 0.0
                        : request.renew_duration());
                response.message("ok");
            });
    lease_server.run();

    sdk::FourierDdsTransport direct(directOptions());
    sdk::ConnectionOptions second_options;
    second_options.domain_id = kDomainId;
    second_options.namespace_name = kNamespace;
    second_options.is_ros_compatible = false;
    second_options.use_discovery_server = false;
    second_options.source_name = "dds-loopback-second";
    second_options.enabled_endpoints = {
        sdk::Endpoint::AuroraStateSubscriber,
    };
    sdk::FourierDdsTransport second_direct(second_options);

    SDK_CHECK(
        test,
        waitUntil(
            [&] {
                const auto status = direct.endpointMatchStatus();
                return status.fsm_service.matched &&
                       status.motion_command_publisher.matched &&
                       status.aurora_state_subscriber.matched;
            },
            5s));
    const auto direct_status = direct.endpointMatchStatus();
    SDK_CHECK(test, direct_status.fsm_service.enabled);
    SDK_CHECK(test, !direct_status.estimator_data_subscriber.enabled);
    SDK_CHECK(test, !direct_status.estimator_data_subscriber.matched);
    SDK_CHECK_THROWS(
        test,
        sdk::InvalidStateError,
        direct.setEstimatorDataHandler({}));

    std::atomic<int> state_callbacks{0};
    std::atomic<bool> current_stop_rejected{false};
    std::atomic<bool> other_stop_rejected{false};
    direct.setAuroraStateHandler([&](sdk::AuroraState state) {
        if (state.current_state.id == 13) {
            ++state_callbacks;
        }
        try {
            direct.stop();
        } catch (const sdk::InvalidStateError&) {
            current_stop_rejected = true;
        }
        try {
            second_direct.stop();
        } catch (const sdk::InvalidStateError&) {
            other_stop_rejected = true;
        }
    });
    publishState(state_publisher, 13);
    SDK_CHECK(
        test,
        waitUntil([&] { return state_callbacks.load() == 1; }, 3s));
    SDK_CHECK(test, current_stop_rejected);
    SDK_CHECK(test, other_stop_rejected);

    sdk::MotionCommand direct_command;
    sdk::detail::LeaseToken direct_token{9001, 0};
    direct_command.header.send_time =
        std::chrono::system_clock::time_point(std::chrono::seconds(123));
    direct_command.lin_velocity = {0.1, 0.0, 0.0};
    auto result = direct.publishMotionCommand(direct_command, direct_token);
    SDK_CHECK(test, result.success());
    SDK_CHECK(
        test,
        waitUntil([&] { return motion_samples.load() >= 1; }, 3s));

    result = direct.requestFsmState(13);
    SDK_CHECK(test, result.code == 213);
    SDK_CHECK(test, result.message == "state=13");

    auto timeout_options = directOptions();
    timeout_options.service_timeout_ms = 10;
    sdk::FourierDdsTransport timeout_direct(timeout_options);
    SDK_CHECK(
        test,
        waitUntil([&] { return timeout_direct.isFsmServiceMatched(); }, 5s));
    service_delay_ms = 100;
    result = timeout_direct.requestFsmState(99);
    SDK_CHECK(test, result.code == sdk::result_codes::SERVICE_TIMEOUT);
    service_delay_ms = 0;
    timeout_direct.stop();

    direct.stop();
    direct.stop();
    second_direct.stop();
    result = direct.publishMotionCommand(direct_command, direct_token);
    SDK_CHECK(test, result.code == sdk::result_codes::CLIENT_NOT_RUNNING);
    SDK_CHECK(test, !direct.isFsmServiceMatched());

    auto& client = sdk::AuroraClient::getInstance();
    client.stop();
    client.setDomainId(kDomainId);
    auto client_options = directOptions();
    client.configure(client_options);
    client.start();
    SDK_CHECK(test, client.waitForEndpoints(5s));
    const auto lease = client.registerLease({sdk::AuroraResource::MotionCommand});
    SDK_CHECK(test, lease.usable());
    publishState(state_publisher, 12);
    sdk::AuroraState observed;
    SDK_CHECK(
        test,
        waitUntil(
            [&] {
                const auto get_result = client.getAuroraState(observed);
                return get_result.success() &&
                       observed.current_state.id == 12;
            },
            3s));
    result = client.requestFsmState(12);
    SDK_CHECK(test, result.code == 213);
    SDK_CHECK(test, result.message == "state=12");
    result = client.setVelocity(0.2, 0.0, 0.0, 0.1);
    SDK_CHECK(test, result.success());
    SDK_CHECK(
        test,
        waitUntil([&] { return motion_samples.load() >= 2; }, 3s));
    client.stop();
    client.start();
    SDK_CHECK(test, client.waitForEndpoints(5s));
    client.stop();

    auto plain_peer = makePeerNode(false);
    std::atomic<int> plain_motion_samples{0};
    fourier_dds::DdsSubscription<dds_msg::RobotMotionCmdPubSubType>
        plain_motion_subscription(
            plain_peer,
            "robot_motion_cmd",
            [&](const dds_msg::RobotMotionCmd&) {
                ++plain_motion_samples;
            },
            false,
            fourier_dds::SubscriptionDurability::Volatile);
    fourier_dds::DdsServer<
        dds_srv::ManageLease_RequestPubSubType,
        dds_srv::ManageLease_ResponsePubSubType>
        plain_lease_server(
            plain_peer,
            "ManageLease",
            [](const dds_srv::ManageLease_Request& request,
               dds_srv::ManageLease_Response& response) {
                response.header(request.header());
                response.result(dds_srv::LEASE_RESULT_OK);
                response.lease_id(9002);
                response.renew_duration(request.renew_duration());
                response.message("ok");
            });
    plain_lease_server.run();
    sdk::ConnectionOptions plain_options;
    plain_options.domain_id = kDomainId;
    plain_options.topic_prefix.clear();
    plain_options.use_namespace = false;
    plain_options.is_ros_compatible = false;
    plain_options.use_discovery_server = false;
    plain_options.source_name = "dds-plain-loopback";
    plain_options.enabled_endpoints = {
        sdk::Endpoint::ManageLeaseService,
        sdk::Endpoint::MotionCommandPublisher,
    };
    sdk::FourierDdsTransport plain_direct(plain_options);
    SDK_CHECK(
        test,
        waitUntil(
            [&] {
                return plain_direct
                    .isMotionCommandPublisherMatched();
            },
            5s));
    result = plain_direct.publishMotionCommand(direct_command, direct_token);
    SDK_CHECK(test, result.success());
    SDK_CHECK(
        test,
        waitUntil(
            [&] { return plain_motion_samples.load() == 1; },
            3s));
    plain_direct.stop();

    return test.result();
}
