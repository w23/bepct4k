# Current status: probably broken
# Overview
## Setup
Single full-screen GLSL fragment shader receiving `uniform float t;` as time in music ticks. 4klang is used to synthesize music.
## Files
* `intro.asm` is the main intro assembly source

TBD

# HOWTO
TBD: How to best adopt this repo? Just copy it? Submodule?
* `make capture` on Linux will dump music to `audio.raw` and encode a perfect 2160p60 (GL_NEAREST-upscaled from 1080p60 to fight possibly noisy content) capture
* `make fast` will build `intro-fast.exe` win32 binary (if crinkler doesn't crash, which it probably will)
* bat files can be used on Windows to build packed release binaries without installing Visual Studio (yay nasm!)