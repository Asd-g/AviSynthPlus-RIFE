## Description

Real-Time Intermediate Flow Estimation for Video Frame Interpolation, based on [rife-ncnn-vulkan](https://github.com/nihui/rife-ncnn-vulkan).

This is [a port of the VapourSynth plugin RIFE](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-RIFE-ncnn-Vulkan).

### Requirements:

- Vulkan device

- AviSynth+ r3688 (can be downloaded from [here](https://gitlab.com/uvz/AviSynthPlus-Builds) until official release is uploaded) or later

- Microsoft VisualC++ Redistributable Package 2022 (can be downloaded from [here](https://github.com/abbodi1406/vcredist/releases))

### Usage:

```
RIFE(clip input, int "model", int "factor_num", int "factor_den", int "fps_num", int "fps_den", string "model_path", int "gpu_id", int "gpu_thread", bool "tta", bool "uhd", bool "sc", float "sc_threshold", bool "skip", float "skip_threshold", bool "list_gpu", bool "denoise", int "denoise_tr")
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
    9: rife-v4 (ensemble=False / fast=True)<br>
    10: rife-v4 (ensemble=True / fast=False)<br>
    11: rife-v4.1 (ensemble=False / fast=True)<br>
    12: rife-v4.1 (ensemble=True / fast=False)<br>
    13: rife-v4.2 (ensemble=False / fast=True)<br>
    14: rife-v4.2 (ensemble=True / fast=False)<br>
    15: rife-v4.3 (ensemble=False / fast=True)<br>
    16: rife-v4.3 (ensemble=True / fast=False)<br>
    17: rife-v4.4 (ensemble=False / fast=True)<br>
    18: rife-v4.4 (ensemble=True / fast=False)<br>
    19: rife-v4.5 (ensemble=False)<br>
    20: rife-v4.5 (ensemble=True)<br>
    21: rife-v4.6 (ensemble=False)<br>
    22: rife-v4.6 (ensemble=True)<br>
    23: sudo_rife4 (ensemble=False / fast=True) (custom model)<br>
    24: sudo_rife4 (ensemble=True / fast=False) (custom model)<br>
    25: sudo_rife4 (ensemble=True / fast=True) (custom model)<br>
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

### More models:

There are more models [here](https://github.com/styler00dollar/VapourSynth-RIFE-ncnn-Vulkan/tree/master/models).

They can be used with parameter `model_path`. It's important to not rename the folder name of the model.

### Building:

- Requires `Boost`, `Vulkan SDK`, `ncnn`.

- Windows<br>
    Use solution files.
