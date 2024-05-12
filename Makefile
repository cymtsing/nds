export MOD=mmiyoo
export PATH=/opt/mmiyoo/bin:$(shell echo $$PATH)
export CROSS=/opt/mmiyoo/bin/arm-linux-gnueabihf-
export CC=${CROSS}gcc
export AR=${CROSS}ar
export AS=${CROSS}as
export LD=${CROSS}ld
export CXX=${CROSS}g++
export HOST=arm-linux

$(shell cd sdl2 && rm -rf libEGL.so libGLESv2.so)
$(shell cd sdl2 && ln -s ../drastic/libs/libEGL.so)
$(shell cd sdl2 && ln -s ../drastic/libs/libGLESv2.so)

SDL2_CFG+= --disable-video-opengl
SDL2_CFG+= --disable-video-opengles
SDL2_CFG+= --disable-video-opengles2

include Makefile.mk
