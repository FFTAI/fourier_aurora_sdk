#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>

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

namespace dds_msg = fourier_msgs::msg;
namespace dds_srv = fourier_msgs::srv;

namespace {

using namespace std::chrono_literals;

constexpr uint32_t kDomainId = 214;
constexpr const char* kNamespace = "aurora_sdk_wheel_loopback";

void publishState(
    fourier_dds::DdsPublisher<dds_msg::AuroraStatePubSubType>& publisher) {
    static constexpr const char* kResources[] = {
        "left_leg", "right_leg", "waist", "head", "left_manipulator",
        "right_manipulator", "left_end_effector", "right_end_effector",
        "motion_cmd", "cartesian_cmd",
    };
    dds_msg::AuroraState state;
    state.robot_info().robot_type("wheel_loopback_robot");
    state.robot_info().hardware_type("wheel_loopback_hardware");
    state.robot_info().end_effector_type("none");
    state.current_state().id(13);
    state.current_state().name("Standing");
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
    auto node = std::make_shared<fourier_dds::DdsNode>(
        kDomainId,
        0,
        std::optional<bool>(false),
        true,
        std::optional<bool>(false));
    node->setNamespace(kNamespace);

    fourier_dds::DdsPublisher<dds_msg::AuroraStatePubSubType> state_publisher(
        node, "aurora/aurora_state");

    std::atomic<bool> motion_received{false};
    fourier_dds::DdsSubscription<dds_msg::RobotMotionCmdPubSubType>
        motion_subscription(
            node,
            "aurora/robot_motion_cmd",
            [&](const dds_msg::RobotMotionCmd&) {
                motion_received = true;
            },
            false,
            fourier_dds::SubscriptionDurability::Volatile);

    std::atomic<bool> request_received{false};
    fourier_dds::DdsServer<
        dds_srv::SetInt_RequestPubSubType,
        dds_srv::SetInt_ResponsePubSubType>
        fsm_server(
            node,
            "aurora/whole_body_fsm_state_change_service",
            [&](const dds_srv::SetInt_Request& request,
                dds_srv::SetInt_Response& response) {
                request_received = true;
                response.result(213);
                response.message("state=" + std::to_string(request.data()));
            });
    fsm_server.run();

    fourier_dds::DdsServer<
        dds_srv::ManageLease_RequestPubSubType,
        dds_srv::ManageLease_ResponsePubSubType>
        lease_server(
            node,
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

    const auto deadline = std::chrono::steady_clock::now() + 20s;
    while (std::chrono::steady_clock::now() < deadline) {
        publishState(state_publisher);
        if (motion_received && request_received) {
            std::cout << "wheel loopback peer received request and motion\n";
            return 0;
        }
        std::this_thread::sleep_for(100ms);
    }

    std::cerr << "wheel loopback peer timed out: request="
              << request_received.load()
              << " motion=" << motion_received.load() << "\n";
    return 1;
}
