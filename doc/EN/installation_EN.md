## Server Installation

Aurora is run as a server on the robot and provided in a docker image for easy installation. Please follow the steps below to install Aurora on the robot chest computer. **You can access the robot chest computer via SSH or directly use a monitor and keyboard.**

### Installing fourierassets

*fourierassets* is a S3 based asset management tool for fourier resources. You can install it by running the following command in your terminal:

```bash
pip3 install fourierassets
```
**Please make sure your python version is 3.8 or above.**

After installing, you can set the credentials by running the following command:

```bash
fourierassets config set-credentials LTAI5tPoZbrPHCdXqnyaUyKB Re8LnYpXs4kazhQXD3GWR5QJ9IEQHZ --endpoint-url https://oss-cn-wulanchabu.aliyuncs.com
```
Check if the credentials are set correctly by running:

```bash
fourierassets test
```
You should see a message like this:

```bash
✓ Successfully connected to S3!
```

### Downloading Docker Image

After installing *fourierassets*, you can download the docker image by running the following command in your terminal:

1. For Fourier GR-1P:
```bash
fourierassets download -v s3://fftai-opensource/fourier_aurora_sdk_gr1p_v1.2.0.zip --cache-dir $your_download_directory
```
2. For Fourier GR-2:
```bash
fourierassets download -v s3://fftai-opensource/fourier_aurora_sdk_gr2_v1.2.0.zip --cache-dir $your_download_directory
```
3. For Fourier GR-3:
```bash
fourierassets download -v s3://fftai-opensource/fourier_aurora_sdk_gr3_v1.2.0.zip --cache-dir $your_download_directory
```

### Loading Docker Image

After downloading and extracting the docker image, you can load it by running the following command in your terminal (replace `fourier_aurora_sdk_gr1p:v1.2.0.tar` with the actual file name you downloaded):

```bash
(sudo) docker load -i fourier_aurora_sdk_gr1p:v1.2.0.tar
```
Please make sure you have installed **Docker** before downloading the image. You can check the installation of Docker by running the following command in your terminal:

```bash
(sudo) docker --version
```
If Docker is not installed, please refer to the official Docker installation guide: [Get Docker](https://docs.docker.com/get-docker/).  

You can use following command to check if the docker image is loaded successfully:

```bash
(sudo) docker images
```

## Client Installation

Aurora client can be installed on the robot chest computer or any other devices to communicate with Aurora server, which should be run on the chest computer. You can install Aurora client by simply running the following command in your terminal:

```bash
pip3 install fourier_aurora_client
```
**Please make sure your python version is 3.9 or above.**
