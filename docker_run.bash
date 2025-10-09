xhost +local:root

docker run -it \
 -e DISPLAY=$DISPLAY \
 -v "$(pwd)":/workspace \
 -w /workspace \
 -v $XSOCK:$XSOCK \
 -v $HOME/.Xauthority:/root/.Xauthority \
 --privileged \
 --net=host \
 --ipc=host \
 --cap-add=SYS_NICE \
 --name fourier_aurora_server \
 fourier_aurora_sdk_gr2:v1.2.0 bash