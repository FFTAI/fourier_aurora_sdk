## 安装指南

Aurora基础环境通过Docker镜像提供。其他模块可以使用deb包进行安装和升级。

### 下载Docker镜像

Docker镜像可以通过点击[fourier_aurora_sdk_v1.1.0.zip](https://pan.baidu.com/s/1fhSD7SUIvE2bTWAkjOqlUA?pwd=wbmu)下载。

该Docker镜像包含：
- **Aurora基础环境**
- **fourier_dds v1.1.0**
- **fourier_hardware v1.1.2**
- **fourier-aurora v1.1.0**
- **fourier-aurora-gr2 v1.1.0**

请确保在下载镜像前已安装**Docker**。您可以通过在终端运行以下命令检查Docker是否安装：

```bash
(sudo) docker --version
```

### 加载Docker镜像

下载并解压Docker镜像后，您可以通过在终端运行以下命令加载镜像：

```bash
(sudo) docker load -i fourier_aurora_sdk:v1.1.0.tar
```

请确保在运行此命令前已安装**Docker**。

您可以使用以下命令检查Docker镜像是否加载成功：

```bash
(sudo) docker images
```

### 检查Docker容器

加载Docker镜像后，您可以在**根目录**下运行以下命令启动容器：

```bash
bash docker_run.bash
```

这将从Docker镜像启动一个容器，您可以通过在终端运行以下命令检查模块安装情况：

```bash
dpkg -l | grep fourier
```

这将显示已安装的模块及其版本。示例输出：

```bash
root@df0484a:/workspace# dpkg -l | grep fourier
ii  fourier-aurora                         1.1.0                                   amd64        A motion control system for fourier humanoid robots.
ii  fourier-aurora-gr2                     1.1.0                                   amd64        An expansion package of fourier aurora for gr2 robot.
ii  fourier_dds                            1.1.0-1                                 amd64        A software to control fourier robots .
ii  fourier_hardware                       1.1.2-1                                 amd64        A software to control fourier robots .
```

## 提交更改(可选)

根据`docker_run.bash`脚本，创建的容器将在退出后被删除。如果您想提交对容器的更改，可以在另一个终端运行以下命令：

```bash
(sudo) docker commit <container_id> <image_name>
```
`<container_id>`可以通过运行`docker ps`命令获取。`<image_name>`是您想给新镜像起的名称。为避免混淆，建议使用原始镜像名称`fourier_aurora_sdk:v1.1.0`。
