# How to can tx process setting #

## 1. CAN utils

```bash
vi ~/.bashrc
# CAN utils command alias
alias canstart="ip link set can0 up type can bitrate 500000"
alias canreceive="candump can0"
alias iscan="ip -details link show can0"
alias cansend="cansend can0 777#DEADBEEF"
```


```bash
# use can alias command
source ~/.bashrc
canstart
```


## 2. Execute can tx process


```bash
# First, Execute post_processing.c -> tcnnapp 
tcnnapp -n ./<model_name>_quantized -i camera -o rtpm

# build can tx process and execute
make clean
make
./can_tx_process
```


## 3. if you want to check the shared memory, command like this
```bash
ls /dev/shm
# /shm_traffic_sign
```