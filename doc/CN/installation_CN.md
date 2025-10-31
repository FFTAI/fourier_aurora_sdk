## 服务安装

Aurora 作为服务在机器人上运行，并提供了一个 docker 镜像以便于安装。请按照以下步骤在机器人胸部计算机上安装 Aurora。**您可以通过 SSH 访问机器人胸部计算机，或直接使用显示器和键盘。**

### 安装 fourierassets

*fourierassets* 是一个基于 S3 的资源管理工具。您可以通过在终端中运行以下命令来安装它：

```bash
pip3 install fourierassets
```
**请确保您的 python 版本为 3.8 或更高。**

安装后，您可以通过运行以下命令来设置凭据：

```bash
fourierassets config set-credentials $access_key $secrect_key --endpoint-url https://oss-cn-wulanchabu.aliyuncs.com
```
对于 `$access_key` 和 `$secrect_key`，请发送电子邮件至 <gr_support@fftai.com> 以获取访问密钥和秘密密钥。

通过运行以下命令检查凭据是否设置正确：

```bash
fourierassets ls s3://fftai-opensource
```

### 下载 Docker 镜像

安装 *fourierassets* 后，您可以通过在终端中运行以下命令来下载 docker 镜像：

1. 对于 Fourier GR-1P：
```bash
fourierassets download -v s3://fftai-opensource/fourier_aurora_sdk_gr1p_v1.2.3.zip --cache-dir $your_download_directory
```
2. 对于 Fourier GR-2：
```bash
fourierassets download -v s3://fftai-opensource/fourier_aurora_sdk_gr2_v1.2.1.zip --cache-dir $your_download_directory
```
3. 对于 Fourier GR-3：
```bash
fourierassets download -v s3://fftai-opensource/fourier_aurora_sdk_gr3_v1.2.1.zip --cache-dir $your_download_directory
```
4. 对于 Fourier-N1：
```bash
fourierassets download -v s3://fftai-opensource/fourier_aurora_sdk_fouriern1_v1.2.0.zip --cache-dir $your_download_directory
```

### 加载 Docker 镜像

下载并提取 docker 镜像后，您可以通过在终端中运行以下命令来加载它（将 `fourier_aurora_sdk_gr1p:v1.2.3.tar` 替换为您下载的实际文件名）：

```bash
(sudo) docker load -i fourier_aurora_sdk_gr1p:v1.2.3.tar
```
请确保在下载镜像之前已安装 **Docker**。您可以通过在终端中运行以下命令来检查 Docker 的安装情况：

```bash
(sudo) docker --version
```
如果未安装 Docker，请参考官方 Docker 安装指南：[获取 Docker](https://docs.docker.com/get-docker/)。

您可以使用以下命令检查 docker 镜像是否成功加载：

```bash
(sudo) docker images
```

## 客户端安装

Aurora 客户端可以安装在机器人胸部计算机或任何其他设备上，以与运行在胸部计算机上的 Aurora 服务器进行通信。您可以通过在终端中运行以下命令轻松安装 Aurora 客户端：

```bash
pip3 install fourier_aurora_client
```
**请确保您的 python 版本为 3.9 或更高。**