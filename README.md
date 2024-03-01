## Description

Real-Time Intermediate Flow Estimation for Video Frame Interpolation, based on [rife-ncnn-vulkan](https://github.com/nihui/rife-ncnn-vulkan).

This is [a port of the VapourSynth plugin RIFE](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-RIFE-ncnn-Vulkan).

### Requirements:

- Vulkan device

- AviSynth+ r3688 (can be downloaded from [here](https://gitlab.com/uvz/AviSynthPlus-Builds) until official release is uploaded) or later

- Microsoft VisualC++ Redistributable Package 2022 (can be downloaded from [here](https://github.com/abbodi1406/vcredist/releases))

### Usage:

```
RIFE(clip input, int "model", int "factor_num", int "factor_den", int "fps_num", int "fps_den", string "model_path", int "gpu_id", int "gpu_thread", bool "tta", bool "uhd", bool "sc", bool "sc1", float "sc_threshold", bool "skip", float "skip_threshold", bool "list_gpu", bool "denoise", int "denoise_tr")
```

### Parameters:

- input<br>
    A clip to process.<br>
    It must be in RGB 32-bit planar format.

- model<br>
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
    47: sudo_rife4_ensembleFalse_fastTrue<br>
    48: sudo_rife4_ensembleTrue_fastFalse<br>
    49: sudo_rife4_ensembleTrue_fastTrue)<br>
    Default: 5.

- factor_num, factor_den<br>
    Factor of target frame rate.<br>
    For example `factor_num=5, factor_den=2` will multiply input clip FPS by 2.5.<br>
    Only rife-v4 model supports custom frame rate.<br>
    Default: 2, 1.

- fps_num, fps_den<br>
    Target frame rate.<br>
    Only rife-v4 model supports custom frame rate.<br>
    Supersedes `factor_num`/`factor_den` parameter if specified.<br>
    Default: Not specified.

- model_path<br>
    RIFE model path.<br>
    Supersedes `model` parameter if specified.<br>
    Default: Not specified.

- gpu_id<br>
    GPU device to use.<br>
    By default the default device is selected.

- gpu_thread<br>
    Thread count for interpolation.<br>
    Using larger values may increase GPU usage and consume more GPU memory.<br>
    If you find that your GPU is hungry, try increasing thread count to achieve faster processing.<br>
    Must be between 1 and the max compute queue count supported by the GPU.<br>
    Default: 2.

- tta<br>
    Enable TTA(Test-Time Augmentation) mode.<br>
    Default: False.

- uhd<br>
    Enable UHD mode.<br>
    Default: False.
- sc<br>
    Avoid interpolating frames over scene changes.<br>
    This cannot be true when `sc1=true`.<br>
    Default: False.

- sc1<br>
    Blend frames (average) frames over scene changes.<br>
    This cannot be true when `sc=true`.<br>
    Default: False.

- sc_threshold<br>
    Threshold to determine whether the current frame and the next one are end/beginning of scene.<br>
    Must be between 0.0..1.0.<br>
    Default: 0.12.

- skip<br>
    Skip interpolating static frames.<br>
    Requires [VMAF](https://github.com/Asd-g/AviSynth-VMAF) plugin.<br>
    Default: False.

- skip_threshold<br>
    PSNR threshold to determine whether the current frame and the next one are static.<br>
    Must be between 0.0..60.0.<br>
    Default: 60.0.

- list_gpu<br>
    Simply print a list of available GPU devices on the frame and does no interpolation.<br>
    Default: False.

- denoise<br>
    Whether to return only the interpolated frames.<br>
    Default: False.

- denoise_tr<br>
    Frame radius.<br>
    For example, `denoise_tr=1` means frames `n-1` and `n+1` are used.<br>
    Must be greater than 0.<br>
    Default: 1.

### Building:

- Requires `Boost`, `Vulkan SDK`, `ncnn`.

```
git clone --recurse-submodules --depth 1 https://github.com/Asd-g/AviSynthPlus-RIFE
cd AviSynthPlus-RIFE

# Building boost (optional):
#b2 --with-system --with-filesystem --with-chrono -q --toolset=msvc-14.3 address-model=64 variant=release link=static runtime-link=shared threading=multi --hash --prefix=.\bin\x64
#b2 --with-system --with-filesystem --with-chrono -q --toolset=msvc-14.3 address-model=64 variant=release link=static runtime-link=shared threading=multi --hash --prefix=.\bin\x64 install

cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=<path_to_boost_installation>;<path_to_ncnn_installation>;<path_to_vulkan_installation>
```
