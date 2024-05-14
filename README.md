# wall-e
A silly little wallpaper programme for X that has a few features for vaguely fun visual effects, for example my set-up with [bspwm](https://github.com/baskerville/bspwm/), [eww](https://github.com/elkowar/eww), and [picom](https://github.com/dccsillag/picom):

https://github.com/mmmsoup/walle/assets/42009575/f36c8e74-92c0-4d59-b3ce-6b0d311c52a7

## Building
Requirements:
- `cmake`
- `gperf`
- OpenGL C libraries (including `glu`)
- `python` (v3 or above)
- `stb_image` from [stb](https://github.com/nothings/stb)
- `xxd`

```shell
git clone --recurse-submodules https://github.com/mmmsoup/walle
cd walle
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=release ..
make
```
