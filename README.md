# BEPCT4K: A pure assembly framework for making 4k intros
Very simple framework for making generic 4klang + fullscreen fragment shader 4k intros. Written in pure assembly and does not require Visual Studio installed.
Just add `4klang.inc` and `shader.frag` files and run `build-slow.bat` to build a release binary of your intro.

## How to use
### Make music
1. Make music in [4klang](https://github.com/hzdgopher/4klang)
2. Export it into `4klang.inc`, replacing existing file here 

### Make visuals
1. Make a full screen GLSL fragment shader somewhere, e.g. in [Shadertoy](https://shadertoy.com) or [Bonzomatic](https://github.com/Gargaj/Bonzomatic)
  * We plan to provide a more convenient tool for editing shader in sync with music, see [issue #4](https://github.com/w23/bepct4k/issues/4)
2. Copy shader source to `shader.frag` file
3. "Port" shader from your tool, e.g. (it may require more editing):
  * This tool will create OpenGL 2.1 context, so GL is limited to `#version 130`
  * add `uniform float t;`, this is the uniform that will get current time in BPM-dependent music ticks, *not seconds*.
  * make `void main()` and `gl_FragColor` and `gl_FragCoord` instead of whatever your tools have
  * replace Shadertoy's:
    * `iTime` with `t`
    * `fResolution` with `vec2(1920., 1080.)`
  * Bonzomatic's:
    * `fGlobalTime` with `t`

### Build
1. Run `build-fast.bat` to build `intro-fast.exe`

### Usage recommendations
Use `build-debug.bat` when first porting your shader. It will build a debug configuration that won't have music, will be windowed, will check all gl calls and `MessageBox` in your face about shader compilation errors.

Use `build-fast.bat` when iterating and estimating intro size. It will build your intro in seconds.

Use `build-slow.bat` when building final. It will be 50-100 bytes less than fast variant, but will take several minutes to build.
