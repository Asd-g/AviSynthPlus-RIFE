## Description

Real-Time Intermediate Flow Estimation for Video Frame Interpolation, based on
[rife-ncnn-vulkan](https://github.com/nihui/rife-ncnn-vulkan).

This is [a port of the VapourSynth plugin RIFE](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-RIFE-ncnn-Vulkan).

### Requirements:

- Vulkan device

- AviSynth+ r3688 or later ([1](https://github.com/AviSynth/AviSynthPlus/releases) / [2](https://forum.doom9.org/showthread.php?t=181351) /
 [3](https://gitlab.com/uvz/AviSynthPlus-Builds))

- **Windows:** Microsoft VisualC++ Redistributable Package 2022 (can be downloaded from [here](https://github.com/abbodi1406/vcredist/releases))

### Installation / Packages:

- **Windows:** Download pre-compiled binaries from [Releases](https://github.com/Asd-g/AviSynthPlus-RIFE/releases).
- **Arch Linux / Debian:** Pre-built packages and distribution builds are maintained at [mysteryx93/AviSynth-Plugins-AUR](https://github.com/mysteryx93/AviSynth-Plugins-AUR/releases).

### Usage:

```
RIFE(clip input, int "model", int "factor_num", int "factor_den", int "fps_num", int "fps_den", string "model_path", int "gpu_id",
 int "gpu_thread", bool "tta", bool "uhd", bool "sc", bool "sc1", float "sc_threshold", bool "skip", float "skip_threshold",
  bool "list_gpu", bool "denoise", int "denoise_tr", int "matrix_in", bool "full_range", bool "cache",
  clip "sc_clip", string "sc_prop", bool "sc_next", clip "skip_clip", string "skip_prop", string "cache_path")
```

### Parameters:

##### ***`input`***
A clip to process.<br>
It must be in planar format.<br>
The output format is `RGBPS`.

##### ***`model`***
Model to use.<br>
`models` must be located in the same folder as RIFE.dll.<br>
Some of the models have two versions: speed oriented (ensemble=False / fast=True) and quality oriented (ensemble=True / fast=False).<br>
0: rife<br>
1: rife-HD<br>
2: rife-UHD<br>
3: rife-anime<br>
4: rife-v2<br>
5: rife-v2.3<br>
6: rife-v2.4<br>
7: rife-v3.0<br>
8: rife-v3.1<br>
9: rife-v3.9_ensembleFalse_fastTrue<br>
10: rife-v3.9_ensembleTrue_fastFalse<br>
11: rife-v4_ensembleFalse_fastTrue<br>
12: rife-v4_ensembleTrue_fastFalse<br>
13: rife-v4.1_ensembleFalse_fastTrue<br>
14: rife-v4.1_ensembleTrue_fastFalse<br>
15: rife-v4.2_ensembleFalse_fastTrue<br>
16: rife-v4.2_ensembleTrue_fastFalse<br>
17: rife-v4.3_ensembleFalse_fastTrue<br>
18: rife-v4.3_ensembleTrue_fastFalse<br>
19: rife-v4.4_ensembleFalse_fastTrue<br>
20: rife-v4.4_ensembleTrue_fastFalse<br>
21: rife-v4.5_ensembleFalse<br>
22: rife-v4.5_ensembleTrue<br>
23: rife-v4.6_ensembleFalse<br>
24: rife-v4.6_ensembleTrue<br>
25: rife-v4.7_ensembleFalse<br>
26: rife-v4.7_ensembleTrue<br>
27: rife-v4.8_ensembleFalse<br>
28: rife-v4.8_ensembleTrue<br>
29: rife-v4.9_ensembleFalse<br>
30: rife-v4.9_ensembleTrue<br>
31: rife-v4.10_ensembleFalse<br>
32: rife-v4.10_ensembleTrue<br>
33: rife-v4.11_ensembleFalse<br>
34: rife-v4.11_ensembleTrue<br>
35: rife-v4.12_ensembleFalse<br>
36: rife-v4.12_ensembleTrue<br>
37: rife-v4.12_lite_ensembleFalse<br>
38: rife-v4.12_lite_ensembleTrue<br>
39: rife-v4.13_ensembleFalse<br>
40: rife-v4.13_ensembleTrue<br>
41: rife-v4.13_lite_ensembleFalse<br>
42: rife-v4.13_lite_ensembleTrue<br>
43: rife-v4.14_ensembleFalse<br>
44: rife-v4.14_ensembleTrue<br>
45: rife-v4.14_lite_ensembleFalse<br>
46: rife-v4.14_lite_ensembleTrue<br>
47: rife-v4.15_ensembleFalse<br>
48: rife-v4.15_ensembleTrue<br>
49: rife-v4.15_lite_ensembleFalse<br>
50: rife-v4.15_lite_ensembleTrue<br>
51: rife-v4.16_lite_ensembleFalse<br>
52: rife-v4.16_lite_ensembleTrue<br>
53: rife-v4.17 (ensemble=False)<br>
54: rife-v4.17 (ensemble=True)<br>
55: rife-v4.17-lite (ensemble=False)<br>
56: rife-v4.17-lite (ensemble=True)<br>
57: rife-v4.18 (ensemble=False)<br>
58: rife-v4.18 (ensemble=True)<br>
59: rife-v4.19-beta (ensemble=False)<br>
60: rife-v4.19-beta (ensemble=True)<br>
61: rife-v4.20 (ensemble=False)<br>
62: rife-v4.20 (ensemble=True)<br>
63: rife-v4.21 (ensemble=False)<br>
64: rife-v4.22 (ensemble=False)<br>
65: rife-v4.22-lite (ensemble=False)<br>
66: rife-v4.23-beta (ensembleFalse)<br>
67: rife-v4.24 (ensembleFalse)<br>
68: rife-v4.24 (ensembleTrue)<br>
69: rife-v4.25 (ensembleFalse)<br>
70: rife-v4.25-lite (ensembleFalse)<br>
71: rife-v4.25-heavy_beta (ensembleFalse)<br>
72: rife-v4.26 (ensembleFalse)<br>
73: rife-v4.26-large (ensembleFalse)<br>
Default: 5.

##### ***`factor_num, factor_den`***
Factor of target frame rate.<br>
For example `factor_num=5, factor_den=2` will multiply input clip FPS by 2.5.<br>
Only rife-v4 model supports custom frame rate.<br>
Default: 2, 1.

##### ***`fps_num, fps_den`***
Target frame rate.<br>
Only rife-v4 model supports custom frame rate.<br>
Supersedes `factor_num`/`factor_den` parameter if specified.<br>
Default: Not specified.

##### ***`model_path`***
RIFE model path.<br>
Supersedes `model` parameter if specified.<br>
Default: Not specified.

##### ***`gpu_id`***
GPU device to use.<br>
By default the default device is selected.

##### ***`gpu_thread`***
Thread count for interpolation.<br>
Using larger values may increase GPU usage and consume more GPU memory.<br>
If you find that your GPU is hungry, try increasing thread count to achieve faster processing.<br>
Must be between 1 and the max compute queue count supported by the GPU.<br>
Default: 2.

##### ***`tta`***
Enable TTA(Test-Time Augmentation) mode.<br>
Default: False.

##### ***`uhd`***
Enable UHD mode.<br>
Default: False.

##### ***`sc` (deprecated, use sc_clip instead)***
Avoid interpolating frames over scene changes using internal SAD-based detection..<br>
This cannot be true when `sc1=true`.<br>
Ignored if `sc_clip` is provided.<br>
Default: False.

##### ***`sc1`***
Blend frames (average) frames over scene changes using internal SAD-based detection.<br>
This cannot be true when `sc=true`.<br>
Ignored if `sc_clip` is provided.<br>
Default: False.

##### ***`sc_threshold`***
Threshold to determine whether the current frame and the next one are end/beginning of scene using internal detection.<br>
Must be between 0.0..1.0.<br>
Ignored if `sc_clip` is provided.<br>
Default: 0.12.

##### ***`skip` (deprecated, use skip_clip instead)***
Skip interpolating static frames using internal VMAF-based detection.<br>
Requires [VMAF](https://github.com/Asd-g/AviSynth-VMAF) plugin.<br>
Ignored if `skip_clip` is provided.<br>
Default: False.

##### ***`skip_threshold`***
PSNR threshold to determine whether the current frame and the next one are static using internal detection.<br>
Must be between 0.0..60.0.<br>
Default: 60.0.

##### ***`list_gpu`***
Simply print a list of available GPU devices on the frame and does no interpolation.<br>
Default: False.

##### ***`denoise`***
Whether to return only the interpolated frames.<br>
Default: False.

##### ***`denoise_tr`***
Frame radius.<br>
For example, `denoise_tr=1` means frames `n-1` and `n+1` are used.<br>
Must be greater than 0.<br>
Default: 1.

##### ***`matrix_in`***
Matrix for YUV->RGB conversion.<br>
Mandatory for YUV input.<br>
0: 601<br>
1: 709<br>
2: 2020<br>
Default: not specified.

##### ***`full_range`***
Input pixel_range.<br>
Default: True for 32-bit or RGB input.

##### ***`cache`***
Whether to share the RIFE model instance between multiple filter calls.<br>
When enabled, instances using the same model, GPU ID, and video format (bit depth, color space, etc.) will share the same memory,
reducing VRAM usage.<br>
If set to False, a private instance of the model will be loaded into VRAM for that specific call.<br>
Default: True.

##### ***`denoise_bf`***
Backward frame radius for denoise=true.<br>
Allows asymmetric reference frame selection.<br>
Must be greater than 0.<br>
Default: Value of denoise_tr.

##### ***`denoise_ff`***
Forward frame radius for denoise=true.<br>
Allows asymmetric reference frame selection.<br>
Must be greater than 0.<br>
Default: Value of denoise_tr.

##### ***`sc_clip`***
External clip for scene change detection.<br>
If provided, it supersedes the internal SAD-based detection (`sc`/`sc1`).<br>
Must have the exact same number of frames as the input clip.<br>
Can be a mask clip (where a pixel value > 0 indicates a scene change) or a clip carrying frame properties.<br>
Default: Not specified.

##### ***`sc_prop`***
Name of the integer frame property in `sc_clip` to read for scene change flags (value > 0).<br>
If specified, `sc_clip` is evaluated in property mode.<br>
If omitted, `sc_clip` is evaluated in mask mode (reading the first pixel of the default plane).<br>
Default: Not specified.

##### ***`sc_next`***
Sets whether scene-change markers in `sc_clip` refer to the frame before or after the cut.<br>
If False, a flag on frame `N` means the scene change occurs *before* frame `N` (between `N-1` and `N`).<br>
If True, a flag on frame `N` means the scene change occurs *after* frame `N` (between `N` and `N+1`).<br>
Default: False.

Example replicating `sc=true`:

```
source
propset("Next", 0)
props = propSet("Next", 1)
ConditionalFilter(last, props, last, "YDifferenceFromPrevious()", ">", "20")
RIFE(gpu_thread=1, matrix_in=1, sc_clip=last, sc_prop="Next")
```

Example replicating `sc1=true`:

```
src

src = src.propset("Next", 0)
props = src.propSet("Next", 1)
tagged_30 = ConditionalFilter(src, props, src, "YDifferenceFromPrevious()", ">", "20")

rife_60 = RIFE(tagged_30, matrix_in=1, sc_clip=tagged_30, sc_prop="Next")

shifted_f = Trim(DuplicateFrame(src, 0), 0, FrameCount(src) - 1)
average = Average(src, 0.5, shifted_f, 0.5)

average_30_aligned = DuplicateFrame(Trim(average, 1, 0), FrameCount(average) - 2)
average_60 = Interleave(src, average_30_aligned)

final_60 = ConditionalFilter(rife_60, z_ConvertFormat(average_60, pixel_type="rgbps"), rife_60,  """propGetInt(rife_60, "_SceneChangeNext") == 1""", "==", "true")

return final_60
```

Example using MVTools for extrapolation:

```
src =

src = src.propset("Next", 0)
props = src.propSet("Next", 1)
tagged_30 = ConditionalFilter(src, props, src, "YDifferenceFromPrevious()", ">", "20")

rife_60 = RIFE(tagged_30, matrix_in=1, sc_clip=tagged_30, sc_prop="Next")

super = MSuper(tagged_30, pel=2, sharp=2)
fv_coarse = MAnalyse(super, isb=false, delta=1, blksize=16, overlap=8, search=3, truemotion=true)
fv_refined = MRecalculate(super, fv_coarse, blksize=8, overlap=4, thSAD=200, search=3)

shifted_fv = DuplicateFrame(fv_refined, 0)
extrap_30 = MFlow(tagged_30, super, shifted_fv, time=50.0)

extrap_30_aligned = DuplicateFrame(Trim(extrap_30, 1, 0), FrameCount(extrap_30) - 2)
extrap_60 = Interleave(tagged_30, extrap_30_aligned)

final_60 = ConditionalFilter(rife_60, z_ConvertFormat(extrap_60, pixel_type="rgbps"), rife_60,  """propGetInt(rife_60, "_SceneChangeNext") == 1""", "==", "true")

return final_60
```

##### ***`skip_clip`***
External clip to detect static frames and skip interpolation.<br>
If provided, it supersedes the internal VMAF-based detection (`skip`).<br>
Must have the exact same number of frames as the input clip.<br>
Can be a mask clip (where a pixel value > 0 indicates a static frame) or a clip carrying frame properties.<br>
Default: Not specified.

##### ***`skip_prop`***
Name of the integer frame property in `skip_clip` to read for skip flags (value > 0).<br>
If specified, `skip_clip` is evaluated in property mode.<br>
If omitted, `skip_clip` is evaluated in mask mode (reading the first pixel of the default plane).<br>
Default: Not specified.

Example replicating the internal static frame detection:

```
source
skip_clip = VMAF2(last, DuplicateFrame(Trim(1, 0), FrameCount() - 1), feature=0)
RIFE(gpu_thread=1, matrix_in=1, skip_clip=skip_clip, skip_prop="psnr_y")
```

##### ***`cache_path`***
File path to save and load the Vulkan pipeline cache.<br>
When provided, compiled Vulkan shaders and driver pipeline state objects are cached to this file,
 reducing the cold-start latency on subsequent runs.<br>
The target directory must be writable. The cache file is strictly tied to the specific GPU, driver, and ncnn version used to create it.<br>
If omitted or empty, Vulkan pipeline caching is disabled.<br>
Default: Not specified.

### Frame Properties:

RIFE sets the following frame properties on its output clip:

##### ***`_SceneChangeNext`***
Standard frame property indicating scene change boundaries as evaluated by RIFE.<br>
1: The frame is the final frame before a scene boundary.<br>
0: Regular frame.<br>

##### ***`RIFE_static`***
Indicates whether an in-between frame was bypassed and duplicated due to static frame detection (`skip` / `skip_clip`).<br>
1: Intermediate frame copied from the previous frame because motion was below the static threshold.<br>
0: Regular frame (source passthrough, AI-interpolated frame, or scene change fallback frame).

### Building:

- Requires `Vulkan SDK`.

```
git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/Asd-g/AviSynthPlus-RIFE
cd AviSynthPlus-RIFE

cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=<path_to_vulkan_installation>
```
