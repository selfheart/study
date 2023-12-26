set(CMAKE_SYSTEM_NAME Linux) 

set(POKY_ROOT /opt/fsl-imx-x11/4.9.11-1.0.0/)
set(CMAKE_SYSROOT ${POKY_ROOT}/sysroots/cortexa9hf-neon-poky-linux-gnueabi) 
 
set(SDKTARGETSYSROOT ${POKY_ROOT}/sysroots/cortexa9hf-neon-poky-linux-gnueabi) 
set(CMAKE_SYSTEM_PROCESSOR "armv7-a") 
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER) 
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY) 
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY) 
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY) 
 
