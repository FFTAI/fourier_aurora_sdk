# Fourier Aurora SDK

这是**Fourier Aurora**（Advanced Unified Robot Operation and Resource Architecture）的SDK仓库。

如果您是第一次接触Aurora，请参考傅里叶文档中心以了解系统：

- [傅里叶文档中心](https://support.fftai.com)

## v1.3.0 版本

支持的机器人：

- GR-1P
- GR-2
- GR-3
- Fourier-N1

先决条件：
- 执行器版本：
  - 通信固件版本 0.3.12.31 或更高版本。
  - 驱动固件版本 0.2.10.30 或更高版本。
  - 注意：执行器版本可以使用**FSA助手**进行升级。点击 [FSA助手 for Linux](https://fsa-1302548221.cos.ap-shanghai.myqcloud.com/tool/FSA_Assistant/FSA_Assistant_V0.0.1.24_155_31_x64_Linux_2025-07-08.tar.gz) 下载最新版本。
 
## 仓库结构

```
fourier_aurora_sdk/
├── config/               # 配置文件
│   └── config.yaml      # 主配置文件
├── python/              # Python SDK
│   ├── example/        # 各机器人模型的示例脚本
│   │   ├── fouriern1/  # Fourier-N1 示例
│   │   ├── gr1p/       # GR-1P 示例
│   │   ├── gr2/        # GR-2 示例
│   │   └── gr3/        # GR-3 示例
│   └── docs/           # API 文档
│       ├── CN/         # 中文 API 文档
│       └── EN/         # 英文 API 文档
├── sim/                 # 仿真工具
│   └── mujoco/         # MuJoCo 仿真器集成
├── docker_run.bash      # Docker 运行脚本
├── LICENSE              # Apache 2.0 许可证
├── README.md            # 英文自述文件
├── README_CN.md         # 本文件
├── CHANGELOG            # 版本历史
└── VERSION              # 当前版本
```

## 安装

### Aurora

请参考[傅里叶文档中心](https://support.fftai.com)。

## Fourier Aurora 客户端

```
pip install fourier_aurora_client==0.1.8
```

## 问题反馈

请报告任何问题或错误！我们将尽力修复它们。

## 许可证

[Apache 2.0](LICENSE) © Fourier