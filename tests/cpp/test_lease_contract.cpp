#include <chrono>
#include <thread>

#include "client_fixture.hpp"
#include "test_harness.hpp"

namespace sdk = fourier::aurora_sdk;
namespace testing = fourier::aurora_sdk::testing;

int main() {
  testing::TestHarness test;
  SDK_CHECK(test,
            sdk::LeaseOption{}.duration == std::chrono::milliseconds{4000});
  auto &client = testing::resetClientFixture();
  auto options = client.config();
  options.source_name = "lease-contract-test";
  options.enabled_endpoints = {
      sdk::Endpoint::ManageLeaseService,
      sdk::Endpoint::MotionCommandPublisher,
      sdk::Endpoint::ControlGroupCommandPublisher,
  };
  client.configure(options);
  client.start();
  auto session = testing::mockFactoryControl().latest();

  sdk::MotionCommand motion;
  auto command_result = client.publishMotionCommand(motion);
  SDK_CHECK(test, command_result.code == sdk::result_codes::LEASE_NOT_ACTIVE);

  sdk::LeaseOption lease_option;
  lease_option.duration = std::chrono::milliseconds{400};
  auto lease = client.registerLease({sdk::AuroraResource::LeftLeg,
                                     sdk::AuroraResource::MotionCommand,
                                     sdk::AuroraResource::CartesianCommand},
                                    lease_option);
  SDK_CHECK(test, lease.success());
  SDK_CHECK(test, lease.usable());
  SDK_CHECK(test, lease.source_name == "lease-contract-test");

  sdk::ControlGroupCommand cartesian_group;
  sdk::CartesianCommand cartesian;
  cartesian.link_name = "base_link_world";
  cartesian.twist = {0.0, 0.0, 0.0, 0.0, 0.0, 0.1};
  cartesian_group.cartesian_cmd.push_back(cartesian);
  command_result = client.publishControlGroupCommand(cartesian_group);
  SDK_CHECK(test, command_result.success());

  command_result = client.publishMotionCommand(motion);
  SDK_CHECK(test, command_result.success());
  {
    std::lock_guard<std::mutex> lock(session->mutex);
    SDK_CHECK(test, session->motion_tokens.size() == 1);
    SDK_CHECK(test, session->motion_tokens[0].lease_id == lease.lease_id);
    SDK_CHECK(test, session->motion_tokens[0].sequence_id == 0);
  }

  sdk::ControlGroupCommand right_leg;
  sdk::JointCommand group;
  group.group_name = "right_leg";
  right_leg.joint_cmd.push_back(group);
  command_result = client.publishControlGroupCommand(right_leg);
  SDK_CHECK(test, command_result.code ==
                      sdk::result_codes::LEASE_RESOURCE_NOT_COVERED);

  {
    std::lock_guard<std::mutex> lock(session->mutex);
    session->lease_response.wire_result = 104;
    session->lease_response.message = "busy";
  }
  auto replaced =
      client.replaceLease({sdk::AuroraResource::RightLeg}, lease_option);
  SDK_CHECK(test, !replaced.success());
  SDK_CHECK(test,
            replaced.operation.code == sdk::result_codes::LEASE_RESOURCE_BUSY);
  SDK_CHECK(test, replaced.usable());
  SDK_CHECK(test, replaced.lease_id == lease.lease_id);

  {
    std::lock_guard<std::mutex> lock(session->mutex);
    session->lease_response.wire_result = 0;
    session->lease_response.message = "accepted";
  }
  const auto released = client.releaseLease();
  SDK_CHECK(test, released.success());
  SDK_CHECK(test, !released.usable());

  lease_option.duration = std::chrono::milliseconds{200};
  lease =
      client.registerLease({sdk::AuroraResource::MotionCommand}, lease_option);
  SDK_CHECK(test, lease.success());
  std::this_thread::sleep_for(std::chrono::milliseconds{90});
  {
    std::lock_guard<std::mutex> lock(session->mutex);
    bool observed_renew = false;
    for (const auto &request : session->lease_requests) {
      observed_renew = observed_renew || request.action == 1;
    }
    SDK_CHECK(test, observed_renew);
  }

  client.stop();
  return test.result();
}
