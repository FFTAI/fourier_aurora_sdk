## Installation

Aurora base environment is provided in the docker image. Other modules can be installed and upgraded using deb packages.

### Downloading Docker Image

The docker image can be downloaded clicking [fourier_aurora_sdk_v1.1.0.zip](https://pan.baidu.com/s/1fhSD7SUIvE2bTWAkjOqlUA?pwd=wbmu). 

This docker image contains:
- **Aurora base environment**
- **fourier_dds v1.1.0** 
- **fourier_hardware v1.1.2**
- **fourier-aurora v1.1.0**
- **fourier-aurora-gr2 v1.1.0**

Please make sure you have installed **Docker** before downloading the image. You can check the installation of Docker by running the following command in your terminal:

```bash
(sudo) docker --version
```

### Loading Docker Image

After downloading and extracting the docker image, you can load it by running the following command in your terminal:

```bash
(sudo) docker load -i fourier_aurora_sdk:v1.1.0.tar
```

Please make sure that you have installed **Docker** before running this command.

you can use following command to check if the docker image is loaded successfully:

```bash
(sudo) docker images
```
### Checking Docker Container

After loading the docker image, you can start the container by running the following command in your terminal under the **root directory** :

```bash
bash docker_run.bash
```

This will start a container from the docker image and you can check the installation of the modules by running the following command in your terminal:

```bash
dpkg -l | grep fourier
```

This will show the installed modules and their versions. Example output:

```bash
root@df0484a:/workspace# dpkg -l | grep fourier
ii  fourier-aurora                         1.1.0                                   amd64        A motion control system for fourier humanoid robots.
ii  fourier-aurora-gr2                     1.1.0                                   amd64        An expansion package of fourier aurora for gr2 robot.
ii  fourier_dds                            1.1.0-1                                 amd64        A software to control fourier robots .
ii  fourier_hardware                       1.1.2-1                                 amd64        A software to control fourier robots .
```
## Committing Changes(Optional)

According to the `docker_run.bash` script, the container created will be removed after exiting. If you want to commit changes to the container, you can run the following command in another terminal:

```bash
(sudo) docker commit <container_id> <image_name>
```
<container_id> can be obtained by running `docker ps` command. <image_name> is the name you want to give to the new image. It is recommended to use the original image name `fourier_aurora_sdk:v1.1.0` to avoid confusion.