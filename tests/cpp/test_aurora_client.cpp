#include <cassert>
#include <memory>

#include "fourier_aurora_sdk/aurora_client.hpp"
#include "fourier_aurora_sdk/transport/mock_transport.hpp"

using namespace fourier::aurora_sdk;

int main() {
    auto transport = std::make_unique<MockTransport>();
    auto* raw_transport = transport.get();
    auto& client = AuroraClient::getInstanceForTesting("tester", 99, {}, std::move(transport));

    bool callback_seen = false;
    client.onMotionState([&](MotionState state) {
        callback_seen = true;
        assert(state.stable_level == 42);
    });

    MotionState motion;
    motion.stable_level = 42;
    motion.delta_height_cmd = 0.1;
    motion.attitude_cmd = {0.0, 0.2, 0.3};
    raw_transport->injectMotionState(motion);

    assert(callback_seen);
    assert(client.getStableLevel() == 42);
    const auto stand_pose = client.getStandPose();
    assert(stand_pose.size() == 4);
    assert(stand_pose[0] == 0.1);
    assert(stand_pose[1] == 0.2);
    assert(stand_pose[2] == 0.3);
    assert(stand_pose[3] == 42.0);

    client.setVelocity(1.0, 2.0, 3.0, 0.5, 7);
    assert(raw_transport->motion_commands.size() == 1);
    assert(raw_transport->motion_commands[0].lin_velocity[0] == 1.0);
    assert(raw_transport->motion_commands[0].lin_velocity[1] == 2.0);
    assert(raw_transport->motion_commands[0].ang_velocity[2] == 3.0);
    assert(raw_transport->motion_commands[0].gait_type == 7);

    raw_transport->next_service_result.result_code = 0;
    raw_transport->next_service_result.message = "ok";
    const auto result = client.requestFsmState(13);
    assert(result.success());
    assert(raw_transport->fsm_requests.size() == 1);
    assert(raw_transport->fsm_requests[0] == 13);

    client.close();
    auto transport2 = std::make_unique<MockTransport>();
    auto& client2 = AuroraClient::getInstanceForTesting("tester2", 100, {}, std::move(transport2));
    assert(client2.domainId() == 100);
    client2.close();
    return 0;
}
