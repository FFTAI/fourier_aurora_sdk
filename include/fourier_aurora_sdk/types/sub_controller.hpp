#pragma once

#include <string>
#include <vector>

#include "fourier_aurora_sdk/types/common.hpp"

namespace fourier::aurora_sdk {

struct GroupOverrideParam {
    std::string group_name;
    std::vector<double> control_param;
};

struct ControllerParam {
    std::string controller_name;
    double rate = 0.0;
    std::vector<GroupOverrideParam> group_override_params;
};

struct SubControllerCommand {
    MessageHeader header;
    bool reset = false;
    std::vector<ControllerParam> controller_params;
};

}  // namespace fourier::aurora_sdk
