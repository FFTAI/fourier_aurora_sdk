xhost +local:root

docker run -it --rm \
 -e DISPLAY=$DISPLAY \
 -v "$(pwd)":/workspace \
 -w /workspace \
 -v $XSOCK:$XSOCK \
 -v $HOME/.Xauthority:/root/.Xauthority \
 --privileged \
 --net=host \
 --ipc=host \
 --cap-add=SYS_NICE \
 --name fourier_aurora_sdk \
 fourier_aurora_sdk:v1.1.0 bash