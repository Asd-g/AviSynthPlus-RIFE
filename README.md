## Description

Real-Time Intermediate Flow Estimation for Video Frame Interpolation, based on [rife-ncnn-vulkan](https://github.com/nihui/rife-ncnn-vulkan).

This is [a port of the VapourSynth plugin RIFE](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-RIFE-ncnn-Vulkan).

### Requirements:

- Vulkan device

- AviSynth+ r3682 (can be downloaded from [here](https://gitlab.com/uvz/AviSynthPlus-Builds) until official release is uploaded) (r3689 recommended) or later

- Microsoft VisualC++ Redistributable Package 2022 (can be downloaded from [here](https://github.com/abbodi1406/vcredist/releases))

### Usage:

```
RIFE(clip input, int "model", int "factor_num", int "factor_den", int "fps_num", int "fps_den", string "model_path", int "gpu_id", int "gpu_thread", bool "tta", bool "uhd", bool "sc", float "sc_threshold", bool "skip", float "skip_threshold", bool "list_gpu")
```

### Parameters:

- input\
    A clip to process.\
    It must be in RGB 32-bit planar format.

- model\
    Model to use.\
    `models` must be located in the same folder as RIFE.dll.\
    Some of the models have two versions: speed oriented (ensemble=False / fast=True) and quality oriented (ensemble=True / fast=False).\
    0: rife\
    1: rife-HD\
    2: rife-UHD\
    3: rife-anime\
    4: rife-v2\
    5: rife-v2.3\
    6: rife-v2.4\
    7: rife-v3.0\
    8: rife-v3.1\
    9: rife-v4 (ensemble=False / fast=True)\
    10: rife-v4 (ensemble=True / fast=False)\
    11: rife-v4.1 (ensemble=False / fast=True)\
    12: rife-v4.1 (ensemble=True / fast=False)\
    13: rife-v4.2 (ensemble=False / fast=True)\
    14: rife-v4.2 (ensemble=True / fast=False)\
    15: rife-v4.3 (ensemble=False / fast=True)\
    16: rife-v4.3 (ensemble=True / fast=False)\
    17: rife-v4.4 (ensemble=False / fast=True)\
    18: rife-v4.4 (ensemble=True / fast=False)\
    19: rife-v4.5 (ensemble=False)\
    20: rife-v4.5 (ensemble=True)\
    21: rife-v4.6 (ensemble=False)\
    22: rife-v4.6 (ensemble=True)\
    23: sudo_rife4 (ensemble=False / fast=True) (custom model)\
    24: sudo_rife4 (ensemble=True / fast=False) (custom model)\
    25: sudo_rife4 (ensemble=True / fast=True) (custom model)\
    Default: 5.

- factor_num, factor_den\
    Factor of target frame rate.\
    For example `factor_num=5, factor_den=2` will multiply input clip FPS by 2.5.\
    Only rife-v4 model supports custom frame rate.\
    Default: 2, 1.

- fps_num, fps_den\
    Target frame rate.\
    Only rife-v4 model supports custom frame rate.\
    Supersedes `factor_num`/`factor_den` parameter if specified.\
    Default: Not specified.

- model_path\
    RIFE model path.\
    Supersedes `model` parameter if specified.\
    Default: Not specified.

- gpu_id\
    GPU device to use.\
    By default the default device is selected.

- gpu_thread\
    Thread count for interpolation.\
    Using larger values may increase GPU usage and consume more GPU memory. If you find that your GPU is hungry, try increasing thread count to achieve faster processing.\
    Default: 2.

- tta\
    Enable TTA(Test-Time Augmentation) mode.\
    Default: False.

- uhd\
    Enable UHD mode.\
    Default: False.
- sc\
    Avoid interpolating frames over scene changes.\
    Default: False.

- sc_threshold\
    Threshold to determine whether the current frame and the next one are end/beginning of scene.\
    Must be between 0.0..1.0.\
    Default: 0.12.

- skip\
    Skip interpolating static frames.\
    Requires [VMAF](https://github.com/Asd-g/AviSynth-VMAF) plugin.\
    Default: False.

- skip_threshold\
    PSNR threshold to determine whether the current frame and the next one are static.\
    Must be between 0.0..60.0.\
    Default: 60.0.

- list_gpu\
    Simply print a list of available GPU devices on the frame and does no interpolation.\
    Default: False.

### Building:

- Requires `Boost`, `Vulkan SDK`, `ncnn`.

- Windows\
    Use solution files.
