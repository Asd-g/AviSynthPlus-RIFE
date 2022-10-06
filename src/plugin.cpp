/*
    MIT License

    Copyright (c) 2021-2022 HolyWu
    Copyright (c) 2022 Asd-g

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <atomic>
#include <fstream>
#include <memory>
#include <semaphore>
#include <string>
#include <vector>

#include "avisynth_c.h"
#include "boost/dll/runtime_symbol_info.hpp"
#include "rife.h"

using namespace std::literals;

static std::atomic<int> numGPUInstances{ 0 };

struct RIFEData
{
    AVS_FilterInfo* fi;
    int sceneChange;
    double sc_threshold;
    int skip;
    double skipThreshold;
    int64_t factor;
    int64_t factorNum;
    int64_t factorDen;
    std::unique_ptr<RIFE> rife;
    std::unique_ptr<std::counting_semaphore<>> semaphore;
    std::unique_ptr<char[]> msg;
    int oldNumFrames;
};

static void filter(const AVS_VideoFrame* src0, const AVS_VideoFrame* src1, AVS_VideoFrame* dst, const float timestep, const RIFEData* const __restrict d) noexcept
{
    const auto width{ avs_get_row_size(src0) / avs_component_size(&d->fi->vi) };
    const auto height{ avs_get_height(src0) };
    const auto src_stride{ avs_get_pitch(src0) / avs_component_size(&d->fi->vi) };
    const auto dst_stride{ avs_get_pitch(dst) / avs_component_size(&d->fi->vi) };
    auto src0R{ reinterpret_cast<const float*>(avs_get_read_ptr_p(src0, AVS_PLANAR_R)) };
    auto src0G{ reinterpret_cast<const float*>(avs_get_read_ptr_p(src0, AVS_PLANAR_G)) };
    auto src0B{ reinterpret_cast<const float*>(avs_get_read_ptr_p(src0, AVS_PLANAR_B)) };
    auto src1R{ reinterpret_cast<const float*>(avs_get_read_ptr_p(src1, AVS_PLANAR_R)) };
    auto src1G{ reinterpret_cast<const float*>(avs_get_read_ptr_p(src1, AVS_PLANAR_G)) };
    auto src1B{ reinterpret_cast<const float*>(avs_get_read_ptr_p(src1, AVS_PLANAR_B)) };
    auto dstR{ reinterpret_cast<float*>(avs_get_write_ptr_p(dst, AVS_PLANAR_R)) };
    auto dstG{ reinterpret_cast<float*>(avs_get_write_ptr_p(dst, AVS_PLANAR_G)) };
    auto dstB{ reinterpret_cast<float*>(avs_get_write_ptr_p(dst, AVS_PLANAR_B)) };

    d->semaphore->acquire();
    d->rife->process(src0R, src0G, src0B, src1R, src1G, src1B, dstR, dstG, dstB, width, height, src_stride, dst_stride, timestep);
    d->semaphore->release();
}

/* multiplies and divides a rational number, such as a frame duration, in place and reduces the result */
AVS_FORCEINLINE void muldivRational(unsigned* num, unsigned* den, int64_t mul, int64_t div)
{
    /* do nothing if the rational number is invalid */
    if (!*den)
        return;

    int64_t a;
    int64_t b;
    *num *= static_cast<unsigned>(mul);
    *den *= static_cast<unsigned>(div);
    a = *num;
    b = *den;

    while (b != 0)
    {
        int64_t t{ a };
        a = b;
        b = t % b;
    }

    if (a < 0)
        a = -a;

    *num /= static_cast<unsigned>(a);
    *den /= static_cast<unsigned>(a);
}

// from avs_core/filters/conditional/conditional_functions.cpp
AVS_FORCEINLINE double get_sad_c(const AVS_VideoFrame* src, const AVS_VideoFrame* src1)
{
    const int c_pitch{ avs_get_pitch(src) / 4 };
    const int t_pitch{ avs_get_pitch(src1) / 4 };
    const int width{ avs_get_row_size(src) / 4 };
    const int height{ avs_get_height(src) };
    const float* c_plane{ reinterpret_cast<const float*>(avs_get_read_ptr(src)) };
    const float* t_plane{ reinterpret_cast<const float*>(avs_get_read_ptr(src1)) };

    double accum{ 0.0 };

    for (int y{ 0 }; y < height; ++y)
    {
        for (int x{ 0 }; x < width; ++x)
            accum += std::abs(t_plane[x] - c_plane[x]);

        c_plane += c_pitch;
        t_plane += t_pitch;
    }

    return (accum / (height * width));
}

static void copy_frame(const AVS_VideoFrame* src, AVS_VideoFrame* dst, AVS_ScriptEnvironment* env)
{
    const int planes[3]{ AVS_PLANAR_R, AVS_PLANAR_G, AVS_PLANAR_B };

    for (int i{ 0 }; i < 3; ++i)
        avs_bit_blt(env, avs_get_write_ptr_p(dst, planes[i]), avs_get_pitch_p(dst, planes[i]), avs_get_read_ptr_p(src, planes[i]), avs_get_pitch_p(src, planes[i]), avs_get_row_size_p(src, planes[i]), avs_get_height_p(src, planes[i]));
}

static AVS_VideoFrame* AVSC_CC RIFE_get_frame(AVS_FilterInfo* fi, int n)
{
    RIFEData* d{ static_cast<RIFEData*>(fi->user_data) };
    const char* ErrorText{ 0 };

    auto frameNum{ static_cast<int>(n * d->factorDen / d->factorNum) };
    auto remainder{ n * d->factorDen % d->factorNum };

    auto src0{ avs_get_frame(fi->child, frameNum) };
    if (!src0)
        return nullptr;

    AVS_VideoFrame* dst{ avs_new_video_frame_p(fi->env, &fi->vi, src0) };

    if (remainder != 0 && n < fi->vi.num_frames - d->factor)
    {
        bool sceneChange{};
        double psnrY{ -1.0 };

        if (d->sceneChange)
        {
            AVS_Value cl{ avs_new_value_clip(fi->child) };
            AVS_Value args_[5]{ cl, avs_new_value_bool(false), avs_new_value_string("pc709"), avs_new_value_string("left"), avs_new_value_string("spline36") };
            AVS_Value inv{ avs_invoke(fi->env, "ConvertToYUV420", avs_new_value_array(args_, 5), 0) };
            if (avs_is_error(inv))
            {
                ErrorText = "RIFE: cannot convert to YUV420. (sc)";

                avs_release_value(inv);
                avs_release_value(cl);
            }

            if (!ErrorText)
            {
                AVS_Clip* abs{ avs_take_clip(inv, fi->env) };

                avs_release_value(inv);
                avs_release_value(cl);

                AVS_VideoFrame* abs_diff{ avs_get_frame(abs, frameNum) };
                AVS_VideoFrame* abs_diff1{ avs_get_frame(abs, (std::min)(fi->vi.num_frames - 1, frameNum + 1)) };
                sceneChange = get_sad_c(abs_diff, abs_diff1) > d->sc_threshold;

                avs_release_video_frame(abs_diff1);
                avs_release_video_frame(abs_diff);
                avs_release_clip(abs);
            }
        }

        if (!ErrorText)
        {
            if (d->skip)
            {
                AVS_Value cl{ avs_new_value_clip(fi->child) };
                AVS_Value args_[5]{ cl, avs_new_value_int((std::min)(fi->vi.width, 512)), avs_new_value_int((std::min)(fi->vi.height, 512)), avs_new_value_float(0.0), avs_new_value_float(0.5) };
                AVS_Value inv{ avs_invoke(fi->env, "BicubicResize", avs_new_value_array(args_, 5), 0) };
                if (avs_is_error(inv))
                {
                    ErrorText = "RIFE: cannot resize. (skip)";

                    avs_release_value(inv);
                    avs_release_value(cl);
                }

                if (!ErrorText)
                {
                    AVS_Clip* clip1{ avs_take_clip(inv, fi->env) };

                    avs_release_value(inv);
                    avs_release_value(cl);
                    //
                    cl = avs_new_value_clip(clip1);

                    avs_release_clip(clip1);

                    AVS_Value args1_[5]{ cl, avs_new_value_bool(false), avs_new_value_string("pc709"), avs_new_value_string("left"), avs_new_value_string("spline36") };
                    inv = avs_invoke(fi->env, "ConvertToYUV420", avs_new_value_array(args1_, 5), 0);
                    if (avs_is_error(inv))
                    {
                        ErrorText = "RIFE: cannot convert to YUV420. (skip)";

                        avs_release_value(inv);
                        avs_release_value(cl);
                    }

                    if (!ErrorText)
                    {
                        clip1 = avs_take_clip(inv, fi->env);

                        avs_release_value(inv);
                        avs_release_value(cl);
                        //
                        cl = avs_new_value_clip(clip1);

                        avs_release_clip(clip1);

                        AVS_Value args2_[7]{ cl, avs_new_value_int(8), avs_new_value_bool(false), avs_new_value_int(-1), avs_new_value_int(8), avs_new_value_bool(true), avs_new_value_bool(false) };
                        inv = avs_invoke(fi->env, "ConvertBits", avs_new_value_array(args2_, 7), 0);
                        if (avs_is_error(inv))
                        {
                            ErrorText = "RIFE: cannot ConvertBits. (skip)";

                            avs_release_value(inv);
                            avs_release_value(cl);
                        }

                        if (!ErrorText)
                        {
                            clip1 = avs_take_clip(inv, fi->env);

                            avs_release_value(inv);
                            avs_release_value(cl);
                            //
                            cl = avs_new_value_clip(clip1);
                            AVS_Value args3_[2]{ cl, avs_new_value_int(d->oldNumFrames - 1) };
                            inv = avs_invoke(fi->env, "DuplicateFrame", avs_new_value_array(args3_, 2), 0);
                            if (avs_is_error(inv))
                            {
                                ErrorText = "RIFE: cannot DuplicateFrame. (skip)";

                                avs_release_value(inv);
                                avs_release_value(cl);
                            }

                            if (!ErrorText)
                            {
                                AVS_Clip* clip2 = avs_take_clip(inv, fi->env);

                                avs_release_value(inv);
                                avs_release_value(cl);
                                //
                                cl = avs_new_value_clip(clip2);

                                avs_release_clip(clip2);

                                AVS_Value args4_[3]{ cl, avs_new_value_int(1), avs_new_value_int(0) };
                                inv = avs_invoke(fi->env, "Trim", avs_new_value_array(args4_, 3), 0);
                                if (avs_is_error(inv))
                                {
                                    ErrorText = "RIFE: cannot Trim. (skip)";

                                    avs_release_value(inv);
                                    avs_release_value(cl);
                                }

                                if (!ErrorText)
                                {
                                    clip2 = avs_take_clip(inv, fi->env);

                                    avs_release_value(inv);
                                    avs_release_value(cl);
                                    //
                                    cl = avs_new_value_clip(clip1);
                                    AVS_Value cl1{ avs_new_value_clip(clip2) };
                                    AVS_Value args5_[3]{ cl, cl1, avs_new_value_int(0) };
                                    inv = avs_invoke(fi->env, "VMAF2", avs_new_value_array(args5_, 3), 0);
                                    if (avs_is_error(inv))
                                    {
                                        ErrorText = "RIFE: VMAF2 is required. (skip)";

                                        avs_release_value(inv);
                                        avs_release_value(cl);
                                    }

                                    if (!ErrorText)
                                    {
                                        AVS_Clip* psnr_clip{ avs_take_clip(inv, fi->env) };

                                        avs_release_clip(clip1);
                                        avs_release_clip(clip2);
                                        avs_release_value(inv);
                                        avs_release_value(cl);
                                        avs_release_value(cl1);

                                        AVS_VideoFrame* psnr{ avs_get_frame(psnr_clip, frameNum) };
                                        psnrY = avs_prop_get_float(fi->env, avs_get_frame_props_ro(fi->env, psnr), "psnr_y", 0, nullptr);

                                        avs_release_video_frame(psnr);
                                        avs_release_clip(psnr_clip);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!ErrorText)
        {
            if (sceneChange || psnrY >= d->skipThreshold)
                copy_frame(src0, dst, fi->env);
            else
            {
                AVS_VideoFrame* src1{ avs_get_frame(fi->child, frameNum + 1) };
                filter(src0, src1, dst, static_cast<float>(remainder) / d->factorNum, d);

                avs_release_video_frame(src1);
            }
        }
    }
    else
        copy_frame(src0, dst, fi->env);

    if (!ErrorText)
    {
        auto props{ avs_get_frame_props_rw(fi->env, dst) };
        int errNum, errDen;
        unsigned durationNum{ static_cast<unsigned>(avs_prop_get_int(fi->env, props, "_DurationNum", 0, &errNum)) };
        unsigned durationDen{ static_cast<unsigned>(avs_prop_get_int(fi->env, props, "_DurationDen", 0, &errDen)) };
        if (!errNum && !errDen)
        {
            muldivRational(&durationNum, &durationDen, d->factorDen, d->factorNum);
            avs_prop_set_int(fi->env, props, "_DurationNum", durationNum, 0);
            avs_prop_set_int(fi->env, props, "_DurationDen", durationDen, 0);
        }

        avs_release_video_frame(src0);

        return dst;
    }
    else
    {
        avs_release_video_frame(dst);
        avs_release_video_frame(src0);

        fi->error = ErrorText;

        return nullptr;
    }
}

static void AVSC_CC free_RIFE(AVS_FilterInfo* fi)
{
    auto d{ static_cast<RIFEData*>(fi->user_data) };
    delete d;

    if (--numGPUInstances == 0)
        ncnn::destroy_gpu_instance();
}

static int AVSC_CC RIFE_set_cache_hints(AVS_FilterInfo* fi, int cachehints, int frame_range)
{
    return cachehints == AVS_CACHE_GET_MTMODE ? 2 : 0;
}

static AVS_Value AVSC_CC Create_RIFE(AVS_ScriptEnvironment* env, AVS_Value args, void* param)
{
    enum { Clip, Model, Factor_num, Factor_den, Fps_num, Fps_den, Model_path, Gpu_id, Gpu_thread, Tta, Uhd, Sc, Sc_threshold, Skip, Skip_threshold, List_gpu };

    auto d{ new RIFEData() };

    AVS_Clip* clip{ avs_new_c_filter(env, &d->fi, avs_array_elt(args, Clip), 1) };
    AVS_Value v{ avs_void };

    try
    {
        if (!avs_is_planar(&d->fi->vi) ||
            !avs_is_rgb(&d->fi->vi) ||
            avs_component_size(&d->fi->vi) < 4)
            throw "only RGB 32-bit planar format supported";

        if (ncnn::create_gpu_instance())
            throw "failed to create GPU instance";
        ++numGPUInstances;

        const auto model{ avs_defined(avs_array_elt(args, Model)) ? avs_as_int(avs_array_elt(args, Model)) : 5 };
        const auto factorNum{ avs_defined(avs_array_elt(args, Factor_num)) ? avs_as_int(avs_array_elt(args, Factor_num)) : 2 };
        const auto factorDen{ avs_defined(avs_array_elt(args, Factor_den)) ? avs_as_int(avs_array_elt(args, Factor_den)) : 1 };
        unsigned fpsNum{ static_cast<unsigned>(avs_defined(avs_array_elt(args, Fps_num)) ? avs_as_int(avs_array_elt(args, Fps_num)) : 0) };
        if (avs_defined(avs_array_elt(args, Fps_num)) && fpsNum < 1)
            throw "fps_num must be at least 1";

        unsigned fpsDen{ static_cast<unsigned>(avs_defined(avs_array_elt(args, Fps_den)) ? avs_as_int(avs_array_elt(args, Fps_den)) : 0) };
        if (avs_defined(avs_array_elt(args, Fps_den)) && fpsDen < 1)
            throw "fps_den must be at least 1";

        std::string modelPath{ avs_defined(avs_array_elt(args, Model_path)) ? avs_as_string(avs_array_elt(args, Model_path)) : "" };
        const auto gpuId{ avs_defined(avs_array_elt(args, Gpu_id)) ? avs_as_int(avs_array_elt(args, Gpu_id)) : ncnn::get_default_gpu_index() };
        const auto gpuThread{ avs_defined(avs_array_elt(args, Gpu_thread)) ? avs_as_int(avs_array_elt(args, Gpu_thread)) : 2 };
        const auto tta{ avs_defined(avs_array_elt(args, Tta)) ? avs_as_bool(avs_array_elt(args, Tta)) : 0 };
        const auto uhd{ avs_defined(avs_array_elt(args, Uhd)) ? avs_as_bool(avs_array_elt(args, Uhd)) : 0 };
        d->sceneChange = avs_defined(avs_array_elt(args, Sc)) ? avs_as_bool(avs_array_elt(args, Sc)) : 0;
        d->sc_threshold = avs_defined(avs_array_elt(args, Sc_threshold)) ? avs_as_float(avs_array_elt(args, Sc_threshold)) : 0.12;
        d->skip = avs_defined(avs_array_elt(args, Skip)) ? avs_as_bool(avs_array_elt(args, Skip)) : 0;
        d->skipThreshold = avs_defined(avs_array_elt(args, Skip_threshold)) ? avs_as_float(avs_array_elt(args, Skip_threshold)) : 60.0;

        if (model < 0 || model > 25)
            throw "model must be between 0 and 25 (inclusive)";
        if (factorNum < 1)
            throw "factor_num must be at least 1";
        if (factorDen < 1)
            throw "factor_den must be at least 1";
        if (fpsNum && fpsDen && !(d->fi->vi.fps_numerator && d->fi->vi.fps_denominator))
            throw "clip does not have a valid frame rate and hence fps_num and fps_den cannot be used";
        if (gpuId < 0 || gpuId >= ncnn::get_gpu_count())
            throw "invalid GPU device";
        if (auto queueCount{ ncnn::get_gpu_info(gpuId).compute_queue_count() }; gpuThread < 1 || static_cast<uint32_t>(gpuThread) > queueCount)
            throw ("gpu_thread must be between 1 and " + std::to_string(queueCount) + " (inclusive)").c_str();
        if (d->sc_threshold < 0 || d->sc_threshold > 1)
            throw "sc_threshold must be between 0.0 and 1.0 (inclusive)";
        if (d->skipThreshold < 0 || d->skipThreshold > 60)
            throw "skip_threshold must be between 0.0 and 60.0 (inclusive)";

        if (fpsNum && fpsDen)
        {
            muldivRational(&fpsNum, &fpsDen, d->fi->vi.fps_denominator, d->fi->vi.fps_numerator);
            d->factorNum = fpsNum;
            d->factorDen = fpsDen;
        }
        else
        {
            d->factorNum = factorNum;
            d->factorDen = factorDen;
        }

        muldivRational(&d->fi->vi.fps_numerator, &d->fi->vi.fps_denominator, d->factorNum, d->factorDen);

        if (d->fi->vi.num_frames < 2)
            throw "clip's number of frames must be at least 2";
        if (d->fi->vi.num_frames / d->factorDen > INT_MAX / d->factorNum)
            throw "resulting clip is too long";

        d->oldNumFrames = d->fi->vi.num_frames;
        d->fi->vi.num_frames = static_cast<int>(d->fi->vi.num_frames * d->factorNum / d->factorDen);

        d->factor = d->factorNum / d->factorDen;

        if (avs_defined(avs_array_elt(args, List_gpu)) ? avs_as_bool(avs_array_elt(args, List_gpu)) : 0)
        {
            std::string text;

            for (auto i{ 0 }; i < ncnn::get_gpu_count(); ++i)
                text += std::to_string(i) + ": " + ncnn::get_gpu_info(i).device_name() + "\n";

            d->msg = std::make_unique<char[]>(text.size() + 1);
            strcpy(d->msg.get(), text.c_str());

            AVS_Value cl{ avs_new_value_clip(clip) };
            AVS_Value args_[2]{ cl, avs_new_value_string(d->msg.get()) };
            AVS_Value inv{ avs_invoke(d->fi->env, "Text", avs_new_value_array(args_, 2), 0) };
            AVS_Clip* clip1{ avs_take_clip(inv, env) };

            v = avs_new_value_clip(clip1);

            avs_release_clip(clip1);
            avs_release_value(inv);
            avs_release_value(cl);
            avs_release_clip(clip);

            if (--numGPUInstances == 0)
                ncnn::destroy_gpu_instance();

            return v;
        }

        if (modelPath.empty())
        {
            modelPath = boost::dll::this_line_location().parent_path().generic_string() + "/models";

            switch (model)
            {
                case 0: modelPath += "/rife"; break;
                case 1: modelPath += "/rife-HD"; break;
                case 2: modelPath += "/rife-UHD"; break;
                case 3: modelPath += "/rife-anime"; break;
                case 4: modelPath += "/rife-v2"; break;
                case 5: modelPath += "/rife-v2.3"; break;
                case 6: modelPath += "/rife-v2.4"; break;
                case 7: modelPath += "/rife-v3.0"; break;
                case 8: modelPath += "/rife-v3.1"; break;
                case 9: modelPath += "/rife-v4_ensembleFalse_fastTrue"; break;
                case 10: modelPath += "/rife-v4_ensembleTrue_fastFalse"; break;
                case 11: modelPath += "/rife-v4.1_ensembleFalse_fastTrue"; break;
                case 12: modelPath += "/rife-v4.1_ensembleTrue_fastFalse"; break;
                case 13: modelPath += "/rife-v4.2_ensembleFalse_fastTrue"; break;
                case 14: modelPath += "/rife-v4.2_ensembleTrue_fastFalse"; break;
                case 15: modelPath += "/rife-v4.3_ensembleFalse_fastTrue"; break;
                case 16: modelPath += "/rife-v4.3_ensembleTrue_fastFalse"; break;
                case 17: modelPath += "/rife-v4.4_ensembleFalse_fastTrue"; break;
                case 18: modelPath += "/rife-v4.4_ensembleTrue_fastFalse"; break;
                case 19: modelPath += "/rife-v4.5_ensembleFalse"; break;
                case 20: modelPath += "/rife-v4.5_ensembleTrue"; break;
                case 21: modelPath += "/rife-v4.6_ensembleFalse"; break;
                case 22: modelPath += "/rife-v4.6_ensembleTrue"; break;
                case 23: modelPath += "/sudo_rife4_ensembleFalse_fastTrue"; break;
                case 24: modelPath += "/sudo_rife4_ensembleTrue_fastFalse"; break;
                case 25: modelPath += "/sudo_rife4_ensembleTrue_fastTrue"; break;
            }
        }

        std::ifstream ifs{ modelPath + "/flownet.param" };
        if (!ifs.is_open())
            throw "failed to load model";
        ifs.close();

        bool rife_v2{};
        bool rife_v4{};

        if (modelPath.find("rife-v2") != std::string::npos)
            rife_v2 = true;
        else if (modelPath.find("rife-v3") != std::string::npos)
            rife_v2 = true;
        else if (modelPath.find("rife-v4") != std::string::npos)
            rife_v4 = true;
        else if (modelPath.find("rife4") != std::string::npos)
            rife_v4 = true;
        else if (modelPath.find("rife") == std::string::npos)
            throw "unknown model dir type";

        if (!rife_v4 && (d->factorNum != 2 || d->factorDen != 1))
            throw "only rife-v4 model supports custom frame rate";

        if (rife_v4 && tta)
            throw "rife-v4 model does not support TTA mode";

        d->semaphore = std::make_unique<std::counting_semaphore<>>(gpuThread);
        d->rife = std::make_unique<RIFE>(gpuId, tta, uhd, 1, rife_v2, rife_v4);

#ifdef _WIN32
        auto bufferSize{ MultiByteToWideChar(CP_UTF8, 0, modelPath.c_str(), -1, nullptr, 0) };
        std::vector<wchar_t> wbuffer(bufferSize);
        MultiByteToWideChar(CP_UTF8, 0, modelPath.c_str(), -1, wbuffer.data(), bufferSize);
        d->rife->load(wbuffer.data());
#else
        d->rife->load(modelPath);
#endif
    }
    catch (const char* error)
    {
        const std::string err{ "RIFE: "s + error };
        d->msg = std::make_unique<char[]>(err.size() + 1);
        strcpy(d->msg.get(), err.c_str());
        v = avs_new_value_error(d->msg.get());

        if (--numGPUInstances == 0)
            ncnn::destroy_gpu_instance();
    }

    if (!avs_defined(v))
    {
        v = avs_new_value_clip(clip);

        d->fi->user_data = reinterpret_cast<void*>(d);
        d->fi->get_frame = RIFE_get_frame;
        d->fi->set_cache_hints = RIFE_set_cache_hints;
        d->fi->free_filter = free_RIFE;
    }

    avs_release_clip(clip);

    return v;
}

const char* AVSC_CC avisynth_c_plugin_init(AVS_ScriptEnvironment* env)
{
    avs_add_function(env, "RIFE", "c[model]i[factor_num]i[factor_den]i[fps_num]i[fps_den]i[model_path]s[gpu_id]i[gpu_thread]i[tta]b[uhd]b[sc]b[sc_threshold]f[skip]b[skip_threshold]f[list_gpu]b", Create_RIFE, 0);
    return "Real-Time Intermediate Flow Estimation for Video Frame Interpolation";
}
