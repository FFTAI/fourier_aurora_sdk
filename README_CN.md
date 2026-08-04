# Fourier Aurora SDK

Fourier Aurora SDK 2.0.1 是通过 FourierDDS 控制单台 Aurora 机器人的 C++/Python
内部预览版客户端。

- C++ 主实现位于 `fourier::aurora_sdk`
- Python 包为 `fourier_aurora_client`，由 pybind11 绑定
- 高层客户端是可重启的进程级单例
- 可选择创建 endpoint，并查询实际匹配状态
- 公开 C++ `FourierDdsTransport` 供直接使用传输层

生成的 DDS 消息类仍由独立消息模块管理；高层 API 只使用 SDK 自有类型。

## 生命周期

`AuroraClient::getInstance()` 只返回稳定单例，不启动 DDS。默认 domain 为
`123`。启用租约能力时必须配置部署分配的非零 `source_id`；未填写 Source Name
时使用 `aurora_sdk_{source_id}`。

配置和 C++ 回调覆盖只能在 `Stopped` 状态修改。`start()` 用冻结后的配置创建
一次 transport session；同步 `stop()` 等待已进入的操作和回调结束，销毁
transport，清理本次 session 缓存，并回到可重新配置、重新启动的 `Stopped`。

```cpp
using namespace fourier::aurora_sdk;

auto& client = AuroraClient::getInstance();
client.setDomainId(123);
auto options = client.config();  // 返回副本。
options.source_id = 42;          // 部署分配的稳定模块标识。
options.enabled_endpoints = {
    Endpoint::AuroraStateSubscriber,
    Endpoint::FsmService,
};
client.configure(options);       // 替换完整配置。

client.start();
if (client.waitForEndpoints(std::chrono::seconds(5))) {
    AuroraState state;
    const OperationResult result = client.getAuroraState(state);
}
client.stop();
```

## 控制租约

运动、控制组和临时电机配置命令只有在当前租约覆盖全部目标资源时才会发布。
用户显式注册租约，SDK 自动续租；替换是原子的，替换被拒绝时旧租约在截止时间
前仍可用。

```cpp
auto lease = client.registerLease({
    AuroraResource::MotionCommand,
    AuroraResource::LeftLeg,
});
if (lease.usable()) {
    client.publishMotionCommand(command);
}
lease = client.replaceLease({AuroraResource::RightLeg});
client.releaseLease();
```

SDK 不把租约状态写入本地文件。进程重启后重新注册；如果旧租约仍在，等待其
短暂过期后重试。
部署方还应维护经过评审的 [Source ID 分配表](docs/source_id_registry.md)。

Python 使用相同生命周期和 snake_case API：

```python
from fourier_aurora_client import AuroraClient, Endpoint

client = AuroraClient.get_instance()
options = client.config()
options.enabled_endpoints = [
    Endpoint.AURORA_STATE_SUBSCRIBER,
    Endpoint.FSM_SERVICE,
]
client.configure(options)

client.start()
if client.wait_for_endpoints(5.0):
    result, state = client.get_aurora_state()
client.stop()
```

程序正常结束前必须显式调用 `stop()`。否则只能依赖进程析构清理；尚未返回的
service 请求或阻塞的 C++ 回调可能延迟甚至卡住退出。C++ 回调在线程内直接由
DDS 调用，必须在有限时间内返回，也不能在回调里直接调用 `stop()`。Stage 1
不提供 Python 回调 API。

`Running` 只表示本地 transport 已启动。endpoint 匹配状态不等于机器人已准备
执行命令。

## 返回值与命令约定

getter、setter、publish 和 RPC 都返回 `OperationResult`。C++ getter 使用输出
参数，失败时不修改输出；Python getter 返回 `(OperationResult, value)`。

- `0`：成功
- `100-199`：SDK 客户端返回值
- `200-299`：Aurora 返回值
- `300-339`：SDK 租约与子控制器结构化返回值

Aurora RPC code/message 原样返回，包括迁移期 FSM code `1` 和 `2`。普通非零
返回值不会由 SDK 自动记录，是否打印由调用方决定。API 误用抛出明确异常；
致命连接错误会记录日志并抛出 `ConnectionError`。

除特别说明外，所有数值使用 SI 单位。运动线速度/角速度和只读笛卡尔状态使用
机器人机身坐标系；运动位置使用世界坐标系。本地 DDS 发布成功不表示 Aurora
已接受或执行命令。参见[命令约定](docs/command_contract.md)和
[返回码](docs/result_codes.md)。

## 依赖

Stage 1 与指定依赖版本绑定：

- `fourier_dds 1.2.0-1`，源码 commit
  `d209ad3c339540ae737ae8f24722186c1631bba2`
- `fourier-dds-msgs 0.1.0-1`，源码 commit
  `18a24585984e04be401cf1acc0151d5b5e95b2cd`
- 消息包对应的 FastDDS/FastCDR runtime
- 启用 Python 时需要 pybind11

CMake 只接收一个依赖前缀：

```bash
-DFOURIER_AURORA_SDK_DDS_PREFIX=/opt/fftai
```

验证环境为 `docker.fftaicorp.com/base/aurora_medium:latest`。详见
[依赖发现](docs/dependency_discovery.md)。

## 构建

原生 Release 默认启用 DDS transport、Python 绑定、诊断工具和 C++ recipe，
测试默认关闭：

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DFOURIER_AURORA_SDK_DDS_PREFIX=/opt/fftai
cmake --build build -j4
```

可独立设置：

```text
FOURIER_AURORA_SDK_BUILD_PYTHON
FOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS
FOURIER_AURORA_SDK_BUILD_EXAMPLES
FOURIER_AURORA_SDK_BUILD_TESTS
FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT
```

完整测试方式见[测试文档](docs/testing.md)。

## 工具与 Recipe

原生包把诊断工具安装到
`/opt/fftai/fourier_aurora_sdk/bin/client/`。参数、固定 endpoint、monitor
行为和退出码见[诊断工具参考](docs/client_tools_CN.md)。

独立 GR3 recipe 位于：

- [C++ recipe](examples/gr3/cpp)
- [Python recipe](examples/gr3/python)

其中包含单次信息读取、限时向前行走和限时摆臂流程。运动 recipe 执行前要求
操作员确认，并且必须先在指定型号机器人上审核。GR3 Mini recipe 是公开前
必须完成的 TODO；不同机器人即使流程一致，也不会共享 recipe 源码。

直接使用 C++ transport 前请阅读
[FourierDDS transport 参考](docs/fourierdds_transport.md)。机器人控制仍推荐
使用高层单例。

## 发布范围

Stage 1 在本地验证 Linux amd64 的 C++ Debian 和 CPython 3.10 wheel。
Linux arm64 留给 CI 集成验证。最终公开矩阵计划为 Linux amd64/arm64 C++
Debian，以及 amd64/arm64 上标准 GIL 的 CPython 3.9-3.13 wheel。

面向外部开发者的教程、机器人能力表、治理、`SECURITY.md` 和完整第三方许可
清单属于公开发布 TODO。本仓库当前覆盖内部开发者需要的接口和示例。完整的
延后矩阵和发布门禁见 [发布检查表](docs/release_checklist.md)。

## 许可证

[Apache License 2.0](LICENSE)
