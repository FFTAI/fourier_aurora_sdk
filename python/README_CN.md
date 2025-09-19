# fourier_aurora_client

这是 **Fourier Aurora** *(高级统一机器人操作与资源架构)* 的Python客户端。它允许您与Aurora API交互，以获取数据和在Aurora平台上执行操作。

如果您是Aurora的新手，请阅读以下文档，了解系统的基本概念：

- [Aurora介绍](./doc/CN/introduction_CN.md)
- [开发者指南](./doc/CN/developer_guide_CN.md)

## 安装

```python
pip install fourier-aurora-client
```

## 使用客户端

在成功运行Aurora之后，打开另一个终端，进入安装了fourier-aurora-client的Python环境。运行示例。

```python
cd examples
python3 client_usage.py
```

运行示例后，终端显示Aurora与Aurora客户端的订阅和发布互相匹配，DDS通讯成功建立。

![图1](./images/matched.png)



更为详尽的使用教程以及接口文档请参考:

- [客户端使用教程](./docs/CN/client_usage_tutorial_CN.md)
- [接口文档](./docs/CN/client_CN.md)


## 注意事项
如果Aurora是在docker环境下运行，Aurora client是在宿主机下运行，请运行以下命令设置环境变量：

```python
export FASTDDS_BUILTIN_TRANSPORTS=UDPv4
```