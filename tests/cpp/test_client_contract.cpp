#include <algorithm>
#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "client_fixture.hpp"
#include "test_harness.hpp"

namespace sdk = fourier::aurora_sdk;
namespace testing = fourier::aurora_sdk::testing;

namespace {

template <typename Value, typename = void>
struct HasCartesianCommandMember : std::false_type {};

template <typename Value>
struct HasCartesianCommandMember<
    Value,
    std::void_t<decltype(std::declval<Value>().cartesian_cmd)>>
    : std::true_type {};

void testTraitsAndDefaults(testing::TestHarness& test) {
    static_assert(!std::is_copy_constructible_v<sdk::AuroraClient>);
    static_assert(!std::is_copy_assignable_v<sdk::AuroraClient>);
    static_assert(!std::is_move_constructible_v<sdk::AuroraClient>);
    static_assert(!std::is_move_assignable_v<sdk::AuroraClient>);
    static_assert(!std::is_destructible_v<sdk::AuroraClient>);
    static_assert(
        !HasCartesianCommandMember<sdk::ControlGroupCommand>::value);

    auto& client = testing::resetClientFixture();
    auto& alias = sdk::AuroraClient::getInstance();
    SDK_CHECK(test, &client == &alias);
    SDK_CHECK(test, client.state() == sdk::ClientState::Stopped);
    SDK_CHECK(test, client.domainId() == 123);
    SDK_CHECK(test, client.config().domain_id == 123);
    SDK_CHECK(test, client.config().source_id == 42);
    SDK_CHECK(test, client.config().topic_prefix == "aurora/");
    SDK_CHECK(test, client.config().enabled_endpoints.size() == 12);

    const sdk::OperationResult default_result;
    SDK_CHECK(test, default_result.code == sdk::result_codes::CLIENT_ERROR);
    SDK_CHECK(test, !default_result.message.empty());
    SDK_CHECK(test, sdk::resultCodeName(999) == "UNKNOWN_RESULT_CODE");
    SDK_CHECK(test, sdk::defaultResultMessage(999) == "unknown result code");
}

void testConfiguration(testing::TestHarness& test) {
    auto& client = testing::resetClientFixture();
    auto options = client.config();
    options.domain_id = 314;
    options.namespace_name = "test_namespace";
    options.topic_prefix = "test/";
    options.dds_mode = 2;
    options.is_ros_compatible = false;
    options.use_namespace = false;
    options.use_discovery_server = false;
    options.service_timeout_ms = 321;
    options.enabled_endpoints = {
        sdk::Endpoint::EstimatorDataSubscriber,
        sdk::Endpoint::FsmService,
        sdk::Endpoint::EstimatorDataSubscriber,
    };
    client.configure(options);

    const auto canonical = client.config();
    SDK_CHECK(test, canonical.domain_id == 314);
    SDK_CHECK(test, canonical.namespace_name == "test_namespace");
    SDK_CHECK(test, canonical.topic_prefix == "test/");
    SDK_CHECK(test, canonical.dds_mode == 2);
    SDK_CHECK(test, canonical.is_ros_compatible == false);
    SDK_CHECK(test, !canonical.use_namespace);
    SDK_CHECK(test, canonical.use_discovery_server == false);
    SDK_CHECK(test, canonical.service_timeout_ms == 321);
    SDK_CHECK(test, canonical.enabled_endpoints.size() == 2);
    SDK_CHECK(
        test,
        canonical.enabled_endpoints[0] == sdk::Endpoint::FsmService);
    SDK_CHECK(
        test,
        canonical.enabled_endpoints[1] ==
            sdk::Endpoint::EstimatorDataSubscriber);

    auto isolated_copy = client.config();
    isolated_copy.service_timeout_ms = 999;
    SDK_CHECK(test, client.config().service_timeout_ms == 321);

    const auto before_invalid = client.config();
    auto invalid = before_invalid;
    invalid.enabled_endpoints.clear();
    SDK_CHECK_THROWS(
        test, sdk::InvalidArgumentError, client.configure(invalid));
    SDK_CHECK(test, client.config() == before_invalid);

    invalid = before_invalid;
    invalid.dds_mode = 4;
    SDK_CHECK_THROWS(
        test, sdk::InvalidArgumentError, client.configure(invalid));
    SDK_CHECK(test, client.config() == before_invalid);

    invalid = before_invalid;
    invalid.topic_prefix = std::string(129, 'a');
    SDK_CHECK_THROWS(
        test, sdk::InvalidArgumentError, client.configure(invalid));
    SDK_CHECK(test, client.config() == before_invalid);

    invalid = before_invalid;
    invalid.enabled_endpoints = {static_cast<sdk::Endpoint>(255)};
    SDK_CHECK_THROWS(
        test, sdk::InvalidArgumentError, client.configure(invalid));
    SDK_CHECK(test, client.config() == before_invalid);

    client.setDomainId(42);
    auto renamed = client.config();
    renamed.source_name = "contract_test";
    client.configure(renamed);
    SDK_CHECK(test, client.domainId() == 42);
    SDK_CHECK(test, client.config().domain_id == 42);
    SDK_CHECK(test, client.config().source_name == "contract_test");
}

void testEndpointAndGetterSemantics(testing::TestHarness& test) {
    auto& client = testing::resetClientFixture();
    auto options = client.config();
    options.enabled_endpoints = {
        sdk::Endpoint::MotionStateSubscriber,
        sdk::Endpoint::ControlGroupStateSubscriber,
        sdk::Endpoint::EstimatorDataSubscriber,
    };
    client.configure(options);

    const auto stopped_status = client.endpointMatchStatus();
    SDK_CHECK(test, stopped_status.motion_state_subscriber.enabled);
    SDK_CHECK(test, !stopped_status.motion_state_subscriber.matched);
    SDK_CHECK(test, !stopped_status.fsm_service.enabled);

    sdk::AuroraState disabled_output;
    disabled_output.current_state.id = 777;
    auto result = client.getAuroraState(disabled_output);
    SDK_CHECK(test, result.code == sdk::result_codes::ENDPOINT_DISABLED);
    SDK_CHECK(test, disabled_output.current_state.id == 777);

    sdk::MotionState motion_output;
    motion_output.stable_level = 77;
    result = client.getMotionState(motion_output);
    SDK_CHECK(test, result.code == sdk::result_codes::CLIENT_NOT_RUNNING);
    SDK_CHECK(test, motion_output.stable_level == 77);

    client.start();
    auto session = testing::mockFactoryControl().latest();
    SDK_CHECK(test, static_cast<bool>(session));
    SDK_CHECK(test, client.state() == sdk::ClientState::Running);
    const auto immediate_match_start = std::chrono::steady_clock::now();
    SDK_CHECK(test, client.waitForEndpoints(std::chrono::milliseconds(0)));
    SDK_CHECK(
        test,
        std::chrono::steady_clock::now() - immediate_match_start >=
            std::chrono::milliseconds(190));

    result = client.getMotionState(motion_output);
    SDK_CHECK(test, result.code == sdk::result_codes::STATE_NOT_RECEIVED);
    SDK_CHECK(test, motion_output.stable_level == 77);

    sdk::MotionState motion;
    motion.stable_level = 42;
    motion.delta_height_cmd = 0.1;
    motion.attitude_cmd = {0.0, 0.2, 0.3};
    session->injectMotionState(motion);

    result = client.getMotionState(motion_output);
    SDK_CHECK(test, result.success());
    SDK_CHECK(test, motion_output.stable_level == 42);

    uint8_t stable_level = 0;
    result = client.getStableLevel(stable_level);
    SDK_CHECK(test, result.success());
    SDK_CHECK(test, stable_level == 42);

    std::vector<double> stand_pose;
    result = client.getStandPose(stand_pose);
    SDK_CHECK(test, result.success());
    SDK_CHECK(test, stand_pose == std::vector<double>({0.1, 0.2, 0.3, 42.0}));

    sdk::ControlGroupState groups;
    sdk::ControlGroupStateEntry group;
    group.group_name = "left_leg";
    group.joint_position = {1.0, 2.0};
    group.joint_velocity = {3.0, 4.0};
    group.joint_effort = {5.0, 6.0};
    groups.joint_state.push_back(group);
    groups.joint_cmd.push_back(group);
    sdk::CartesianStateEntry link;
    link.link_name = "left_foot";
    link.pose[0] = 9.0;
    groups.cartesian_state.push_back(link);
    groups.cartesian_cmd.push_back(link);
    session->injectControlGroupState(groups);

    std::vector<double> values{99.0};
    result = client.getJointState(
        "left_leg", sdk::JointField::Velocity, values);
    SDK_CHECK(test, result.success());
    SDK_CHECK(test, values == std::vector<double>({3.0, 4.0}));

    values = {99.0};
    result = client.getJointState(
        "missing", sdk::JointField::Position, values);
    SDK_CHECK(test, result.code == sdk::result_codes::CLIENT_ERROR);
    SDK_CHECK(test, result.message.find("missing") != std::string::npos);
    SDK_CHECK(test, values == std::vector<double>({99.0}));

    sdk::CartesianStateEntry cartesian;
    cartesian.link_name = "unchanged";
    result = client.getCartesianState("left_foot", cartesian);
    SDK_CHECK(test, result.success());
    SDK_CHECK(test, cartesian.pose[0] == 9.0);
    result = client.getCartesianCommand("left_foot", cartesian);
    SDK_CHECK(test, result.success());

    SDK_CHECK_THROWS(
        test,
        sdk::InvalidArgumentError,
        client.getJointState(
            "left_leg", static_cast<sdk::JointField>(99), values));

    sdk::EstimatorData estimator;
    estimator.rpy = {0.1, 0.2, 0.3};
    estimator.contact_force = {10.0, 20.0};
    estimator.stable_level = 8;
    session->injectEstimatorData(estimator);
    sdk::BaseData base;
    sdk::ContactData contacts;
    SDK_CHECK(test, client.getBaseData(base).success());
    SDK_CHECK(test, base.rpy == estimator.rpy);
    SDK_CHECK(test, base.stable_level == 8);
    SDK_CHECK(test, client.getContactData(contacts).success());
    SDK_CHECK(test, contacts.contact_force == estimator.contact_force);

    session->setMatched(sdk::Endpoint::MotionStateSubscriber, false);
    motion_output.stable_level = 91;
    std::ostringstream captured;
    auto* old_buffer = std::cerr.rdbuf(captured.rdbuf());
    result = client.getMotionState(motion_output);
    std::cerr.rdbuf(old_buffer);
    SDK_CHECK(test, result.code == sdk::result_codes::ENDPOINT_UNMATCHED);
    SDK_CHECK(test, motion_output.stable_level == 91);
    SDK_CHECK(test, captured.str().empty());

    session->setMatched(sdk::Endpoint::MotionStateSubscriber, true);
    result = client.getMotionState(motion_output);
    SDK_CHECK(test, result.success());
    SDK_CHECK(test, motion_output.stable_level == 42);

    session->setAllEnabledMatched(false);
    SDK_CHECK(test, !client.waitForEndpoints(std::chrono::milliseconds(0)));

    const auto delayed_match_start = std::chrono::steady_clock::now();
    std::thread match_later([session] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        session->setAllEnabledMatched(true);
    });
    SDK_CHECK(test, client.waitForEndpoints(std::chrono::milliseconds(500)));
    match_later.join();
    SDK_CHECK(
        test,
        std::chrono::steady_clock::now() - delayed_match_start >=
            std::chrono::milliseconds(240));

    session->setAllEnabledMatched(false);
    SDK_CHECK_THROWS(
        test,
        sdk::InvalidArgumentError,
        client.waitForEndpoints(std::chrono::milliseconds(-1)));

    client.stop();
    SDK_CHECK(test, client.state() == sdk::ClientState::Stopped);
}

void testFsmObservationGenerations(testing::TestHarness& test) {
    auto& client = testing::resetClientFixture();
    auto options = client.config();
    options.enabled_endpoints = {
        sdk::Endpoint::FsmService,
        sdk::Endpoint::AuroraStateSubscriber,
    };
    client.configure(options);
    client.start();
    auto session = testing::mockFactoryControl().latest();

    sdk::AuroraState state;
    state.current_state.id = 13;
    session->injectAuroraState(state);
    SDK_CHECK(
        test,
        client.waitForFsmState(13, std::chrono::milliseconds(0)));

    session->setMatched(sdk::Endpoint::AuroraStateSubscriber, false);
    static_cast<void>(client.endpointMatchStatus());
    session->setMatched(sdk::Endpoint::AuroraStateSubscriber, true);
    SDK_CHECK(
        test,
        !client.waitForFsmState(13, std::chrono::milliseconds(0)));
    session->injectAuroraState(state);
    SDK_CHECK(
        test,
        client.waitForFsmState(13, std::chrono::milliseconds(0)));

    session->fsm_result = {1, "legacy rejected"};
    auto result = client.requestFsmState(7);
    SDK_CHECK(test, result.code == 1);
    SDK_CHECK(test, result.message == "legacy rejected");
    session->fsm_result = {2, ""};
    result = client.requestFsmState(8);
    SDK_CHECK(test, result.code == 2);
    SDK_CHECK(test, result.message.empty());
    session->fsm_result = {250, "Aurora result"};
    result = client.requestFsmState(9);
    SDK_CHECK(test, result.code == 250);
    SDK_CHECK(test, result.message == "Aurora result");
    session->fsm_result = {
        sdk::result_codes::INVALID_AURORA_RESPONSE,
        "malformed"};
    result = client.requestFsmState(10);
    SDK_CHECK(
        test,
        result.code == sdk::result_codes::INVALID_AURORA_RESPONSE);

    client.stop();
}

void testPublicationAndCommandCaches(testing::TestHarness& test) {
    auto& client = testing::resetClientFixture();
    auto options = client.config();
    options.enabled_endpoints = {
        sdk::Endpoint::ManageLeaseService,
        sdk::Endpoint::MotionCommandPublisher,
        sdk::Endpoint::ControlGroupCommandPublisher,
        sdk::Endpoint::SubControllerService,
    };
    client.configure(options);
    client.start();
    auto session = testing::mockFactoryControl().latest();
    const auto lease = client.registerLease({
        sdk::AuroraResource::MotionCommand,
        sdk::AuroraResource::LeftLeg,
    });
    SDK_CHECK(test, lease.success());
    SDK_CHECK(test, lease.usable());

    sdk::MotionCommand command;
    command.header.send_time =
        std::chrono::system_clock::time_point(std::chrono::seconds(1));
    command.header.frame_id = "caller";
    command.header.received_time =
        std::chrono::system_clock::time_point(std::chrono::seconds(2));
    command.lin_velocity = {1.0, 2.0, 3.0};
    const auto caller_copy = command;
    auto result = client.publishMotionCommand(command);
    SDK_CHECK(test, result.success());
    SDK_CHECK(test, command.header.send_time == caller_copy.header.send_time);
    SDK_CHECK(test, command.header.frame_id == "caller");

    sdk::MotionCommand cached;
    result = client.getLastMotionCommand(cached);
    SDK_CHECK(test, result.success());
    SDK_CHECK(test, cached.lin_velocity == command.lin_velocity);
    SDK_CHECK(test, cached.header.send_time != command.header.send_time);
    SDK_CHECK(test, cached.header.frame_id.empty());
    SDK_CHECK(
        test,
        cached.header.received_time ==
            std::chrono::system_clock::time_point{});

    {
        std::lock_guard<std::mutex> lock(session->mutex);
        SDK_CHECK(test, session->motion_commands.size() == 1);
        SDK_CHECK(
            test,
            session->motion_commands.front().header.send_time ==
                cached.header.send_time);
        session->motion_publish_result = {
            sdk::result_codes::PUBLISH_FAILED,
            "injected publish failure"};
    }
    sdk::MotionCommand failed_command;
    failed_command.lin_velocity = {9.0, 9.0, 9.0};
    result = client.publishMotionCommand(failed_command);
    SDK_CHECK(test, result.code == sdk::result_codes::PUBLISH_FAILED);
    sdk::MotionCommand after_failure;
    SDK_CHECK(test, client.getLastMotionCommand(after_failure).success());
    SDK_CHECK(test, after_failure.lin_velocity == command.lin_velocity);

    sdk::ControlGroupCommand group_command;
    sdk::JointCommand joint;
    joint.group_name = "left_leg";
    joint.control_type = static_cast<sdk::JointControlType>(200);
    joint.motor_mode = static_cast<sdk::MotorMode>(201);
    joint.position = {1.0, 2.0, 3.0};
    group_command.joint_cmd.push_back(joint);
    result = client.publishControlGroupCommand(group_command);
    SDK_CHECK(test, result.success());
    {
        std::lock_guard<std::mutex> lock(session->mutex);
        SDK_CHECK(test, session->control_group_commands.size() == 1);
        const auto& forwarded =
            session->control_group_commands.front().joint_cmd.front();
        SDK_CHECK(
            test,
            static_cast<uint8_t>(forwarded.control_type) == 200);
        SDK_CHECK(test, static_cast<uint8_t>(forwarded.motor_mode) == 201);
    }

    result = client.resetUnicontroller();
    SDK_CHECK(test, result.success());
    sdk::SubControllerCommand sub_command;
    SDK_CHECK(
        test,
        client.getLastSubControllerCommand(sub_command).success());
    SDK_CHECK(test, sub_command.reset);

    client.stop();
    result = client.getLastMotionCommand(cached);
    SDK_CHECK(test, result.code == sdk::result_codes::CLIENT_NOT_RUNNING);
}

}  // namespace

int main() {
    testing::TestHarness test;
    testTraitsAndDefaults(test);
    testConfiguration(test);
    testEndpointAndGetterSemantics(test);
    testFsmObservationGenerations(test);
    testPublicationAndCommandCaches(test);
    return test.result();
}
