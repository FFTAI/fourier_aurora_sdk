# Fourier Aurora SDK

GR3-Mini 主板开发请参阅[环境检查与 Python/C++ 用例](examples/gr3mini/README_CN.md)。

[English](README.md)

Fourier Aurora SDK 1.0.1 是通过 FourierDDS 读取 Aurora 机器人状态、发送控制
命令的 C++17/Python 客户端库。应用只接触 SDK 自有类型，生成的 DDS 消息类型
不会暴露到 SDK 边界之外。

## 选择安装包

- C++ 用户安装 `fourier-aurora-sdk` Debian 包，其中包含头文件、动态库、CMake
  包配置、诊断客户端和 GR3 示例。
- Python 用户安装 `fourier-aurora-client` wheel，并导入
  `fourier_aurora_client`。
- 开发 SDK 或为部署环境制作安装包时，从源码构建。详见
  [SDK 总览与构建指南](docs/overview_CN.md)。

使用前必须先安装与你的架构匹配的 `fourier_dds 1.2.0-1` 和
`fourier-dds-msgs 0.2.0-1`。两种 SDK 安装包都依赖这些原生运行库，wheel 不会
捆绑它们。详见[依赖安装说明](docs/overview_CN.md#依赖)和
[兼容性清单](REVISIONS.yaml)。

## Python 快速开始：读取机器人状态

安装与你的平台匹配的 wheel，并且只创建程序实际需要的 endpoint：

```bash
python3 -m pip install fourier_aurora_client-1.0.1-*.whl
```

```python
from fourier_aurora_client import AuroraClient, Endpoint

client = AuroraClient.get_instance()
options = client.config()
options.domain_id = 123
options.source_name = "state-reader"
options.enabled_endpoints = [Endpoint.AURORA_STATE_SUBSCRIBER]
client.configure(options)

try:
    client.start()
    if not client.wait_for_endpoints(5.0):
        raise RuntimeError("Aurora state endpoint did not match")

    result, state = client.get_aurora_state()
    if not result.success():
        raise RuntimeError(f"[{result.code}] {result.message}")
    print(state.robot_info.robot_type, state.current_state.name)
finally:
    client.stop()
```

Python 的 timeout 单位是秒。`Running` 仅表示本地 transport 已启动；使用
`wait_for_endpoints()` 判断 DDS 是否完成发现。Endpoint 已匹配也不代表机器人
已经可以执行命令。

## C++ 快速开始：读取机器人状态

安装 Debian 包后，链接导出的 CMake target：

```cmake
find_package(fourier_aurora_sdk 1.0 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE fourier_aurora_sdk::fourier_aurora_sdk)
target_compile_features(my_app PRIVATE cxx_std_17)
```

```cpp
#include <chrono>
#include <iostream>
#include "fourier_aurora_sdk/aurora_client.hpp"

namespace sdk = fourier::aurora_sdk;

int main() {
    auto& client = sdk::AuroraClient::getInstance();
    auto options = client.config();
    options.domain_id = 123;
    options.source_name = "state-reader";
    options.enabled_endpoints = {sdk::Endpoint::AuroraStateSubscriber};
    client.configure(options);

    try {
        client.start();
        if (!client.waitForEndpoints(std::chrono::seconds(5))) {
            client.stop();
            return 1;
        }
        sdk::AuroraState state;
        const auto result = client.getAuroraState(state);
        if (result.success()) {
            std::cout << state.robot_info.robot_type << " "
                      << state.current_state.name << "\n";
        }
        client.stop();
        return result.success() ? 0 : 1;
    } catch (...) {
        try { client.stop(); } catch (...) {}
        throw;
    }
}
```

C++ timeout 使用 `std::chrono`。程序正常退出时必须显式调用 `stop()`。

## 发送受保护命令

运动、控制组和临时电机配置 publisher 必须持有控制租约。每个直接使用 SDK 的
client 都必须提供符合 `[A-Za-z0-9._-]{1,64}` 的可读 `source_name`。并发运行的
模块应使用不同名称，便于在权限状态和日志中区分。

```cpp
options.source_name = "walking-controller";
options.enabled_endpoints = {
    sdk::Endpoint::ManageLeaseService,
    sdk::Endpoint::MotionCommandPublisher,
};
client.configure(options);
client.start();
client.waitForEndpoints(std::chrono::seconds(5));

const auto lease = client.registerLease({sdk::AuroraResource::MotionCommand});
if (lease.usable()) {
    const auto result = client.setVelocity(0.1, 0.0, 0.0, 1.0);
}
client.releaseLease();
client.stop();
```

SDK 会自动续租有效租约。`replaceLease()` 会原子地申请一组新的完整资源；如果
请求被明确拒绝，只要旧租约仍在有效期内，就继续保留旧租约。租约状态只存在于
内存，进程重启后不会恢复。

本地 publish 成功只表示 DDS 接受了 sample，不表示 Aurora 已接受或执行命令。
应结合 Aurora 状态与错误遥测判断机器人结果。测试控制改动时，应支撑机器人、
清空工作区域并使用保守的速度和幅度限制。

## 文档

- [SDK 总览、架构、依赖与构建指南](docs/overview_CN.md)
- [C++ API 参考](docs/cpp_api_CN.md)
- [Python API 参考](docs/python_api_CN.md)
- [示例与诊断客户端](docs/examples_and_clients_CN.md)
- [开发者与 Agent 上手指南](AGENTS.md)
- [English documentation](README.md)

## 许可证

[Apache License 2.0](LICENSE)
