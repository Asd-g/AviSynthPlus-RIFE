// Copyright (c) 2021-2022 HolyWu
// Copyright (c) 2022-2025 Asd-g
// SPDX-License-Identifier: MIT

#include <array>
#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <semaphore>
#include <string>
#include <utility>
#include <vector>

#include "../avs_c_api_loader/avs_c_api_loader.hpp"
#include "boost/dll/runtime_symbol_info.hpp"
#include "rife.h"

static std::atomic<int> numGPUInstances{ 0 };

template <typename Key, typename Value1, typename Value2, std::size_t Size>
struct Map
{
    std::array<std::tuple<Key, Value1, Value2>, Size> data;
    constexpr std::pair<Value1, Value2> at(const Key& key) const
    {
        const auto itr{ std::find_if(begin(data), end(data), [&key](const auto& v) { return std::get<0>(v) == key; }) };

        if (itr != end(data))
            return std::make_pair(std::get<1>(*itr), std::get<2>(*itr));
        if constexpr (std::is_same_v<Key, int>)
            return std::make_pair("", -1);
        else
            return std::make_tuple(nullptr, -1.0f, -1.0f);
    }
};

// model number, model name, model padding
static constexpr std::array<std::tuple<int, std::string_view, int>, 74> models_num
{
    std::make_tuple(0, "/rife", 32),
    std::make_tuple(1, "/rife-HD", 32),
    std::make_tuple(2, "/rife-UHD", 32),
    std::make_tuple(3, "/rife-anime", 32),
    std::make_tuple(4, "/rife-v2", 32),
    std::make_tuple(5, "/rife-v2.3", 32),
    std::make_tuple(6, "/rife-v2.4", 32),
    std::make_tuple(7, "/rife-v3.0", 32),
    std::make_tuple(8, "/rife-v3.1", 32),
    std::make_tuple(9, "/rife-v3.9_ensembleFalse_fastTrue", 32),
    std::make_tuple(10, "/rife-v3.9_ensembleTrue_fastFalse", 32),
    std::make_tuple(11, "/rife-v4_ensembleFalse_fastTrue", 32),
    std::make_tuple(12, "/rife-v4_ensembleTrue_fastFalse", 32),
    std::make_tuple(13, "/rife-v4.1_ensembleFalse_fastTrue", 32),
    std::make_tuple(14, "/rife-v4.1_ensembleTrue_fastFalse", 32),
    std::make_tuple(15, "/rife-v4.2_ensembleFalse_fastTrue", 32),
    std::make_tuple(16, "/rife-v4.2_ensembleTrue_fastFalse", 32),
    std::make_tuple(17, "/rife-v4.3_ensembleFalse_fastTrue", 32),
    std::make_tuple(18, "/rife-v4.3_ensembleTrue_fastFalse", 32),
    std::make_tuple(19, "/rife-v4.4_ensembleFalse_fastTrue", 32),
    std::make_tuple(20, "/rife-v4.4_ensembleTrue_fastFalse", 32),
    std::make_tuple(21, "/rife-v4.5_ensembleFalse", 32),
    std::make_tuple(22, "/rife-v4.5_ensembleTrue", 32),
    std::make_tuple(23, "/rife-v4.6_ensembleFalse", 32),
    std::make_tuple(24, "/rife-v4.6_ensembleTrue", 32),
    std::make_tuple(25, "/rife-v4.7_ensembleFalse", 32),
    std::make_tuple(26, "/rife-v4.7_ensembleTrue", 32),
    std::make_tuple(27, "/rife-v4.8_ensembleFalse", 32),
    std::make_tuple(28, "/rife-v4.8_ensembleTrue", 32),
    std::make_tuple(29, "/rife-v4.9_ensembleFalse", 32),
    std::make_tuple(30, "/rife-v4.9_ensembleTrue", 32),
    std::make_tuple(31, "/rife-v4.10_ensembleFalse", 32),
    std::make_tuple(32, "/rife-v4.10_ensembleTrue", 32),
    std::make_tuple(33, "/rife-v4.11_ensembleFalse", 32),
    std::make_tuple(34, "/rife-v4.11_ensembleTrue", 32),
    std::make_tuple(35, "/rife-v4.12_ensembleFalse", 32),
    std::make_tuple(36, "/rife-v4.12_ensembleTrue", 32),
    std::make_tuple(37, "/rife-v4.12_lite_ensembleFalse", 32),
    std::make_tuple(38, "/rife-v4.12_lite_ensembleTrue", 32),
    std::make_tuple(39, "/rife-v4.13_ensembleFalse", 32),
    std::make_tuple(40, "/rife-v4.13_ensembleTrue", 32),
    std::make_tuple(41, "/rife-v4.13_lite_ensembleFalse", 32),
    std::make_tuple(42, "/rife-v4.13_lite_ensembleTrue", 32),
    std::make_tuple(43, "/rife-v4.14_ensembleFalse", 32),
    std::make_tuple(44, "/rife-v4.14_ensembleTrue", 32),
    std::make_tuple(45, "/rife-v4.14_lite_ensembleFalse", 32),
    std::make_tuple(46, "/rife-v4.14_lite_ensembleTrue", 32),
    std::make_tuple(47, "/rife-v4.15_ensembleFalse", 32),
    std::make_tuple(48, "/rife-v4.15_ensembleTrue", 32),
    std::make_tuple(49, "/rife-v4.15_lite_ensembleFalse", 32),
    std::make_tuple(50, "/rife-v4.15_lite_ensembleTrue", 32),
    std::make_tuple(51, "/rife-v4.16_lite_ensembleFalse", 32),
    std::make_tuple(52, "/rife-v4.16_lite_ensembleTrue", 32),
    std::make_tuple(53, "/rife-v4.17_ensembleFalse", 32),
    std::make_tuple(54, "/rife-v4.17_ensembleTrue", 32),
    std::make_tuple(55, "/rife-v4.17_lite_ensembleFalse", 32),
    std::make_tuple(56, "/rife-v4.17_lite_ensembleTrue", 32),
    std::make_tuple(57, "/rife-v4.18_ensembleFalse", 32),
    std::make_tuple(58, "/rife-v4.18_ensembleTrue", 32),
    std::make_tuple(59, "/rife-v4.19_beta_ensembleFalse", 32),
    std::make_tuple(60, "/rife-v4.19_beta_ensembleTrue", 32),
    std::make_tuple(61, "/rife-v4.20_ensembleFalse", 32),
    std::make_tuple(62, "/rife-v4.20_ensembleTrue", 32),
    std::make_tuple(63, "/rife-v4.21_ensembleFalse", 32),
    std::make_tuple(64, "/rife-v4.22_ensembleFalse", 32),
    std::make_tuple(65, "/rife-v4.22_lite_ensembleFalse", 32),
    std::make_tuple(66, "/rife-v4.23_beta_ensembleFalse", 32),
    std::make_tuple(67, "/rife-v4.24_ensembleFalse", 32),
    std::make_tuple(68, "/rife-v4.24_ensembleTrue", 32),
    std::make_tuple(69, "/rife-v4.25_ensembleFalse", 64),
    std::make_tuple(70, "/rife-v4.25-lite_ensembleFalse", 128),
    std::make_tuple(71, "/rife-v4.25_heavy_beta_ensembleFalse", 64),
    std::make_tuple(72, "/rife-v4.26_ensembleFalse", 64),
    std::make_tuple(73, "/rife-v4.26-large_ensembleFalse", 64)
};

static constexpr auto map_models{ Map<int, std::string_view, int, 74>{ { models_num } } };

struct RIFEData
{
    AVS_FilterInfo* fi;
    double sc_threshold;
    double skipThreshold;
    int64_t factor;
    int64_t factorNum;
    int64_t factorDen;
    std::unique_ptr<RIFE> rife;
    std::unique_ptr<std::counting_semaphore<>> semaphore;
    std::string msg;
    int oldNumFrames;
    int tr;
};

static void filter(const AVS_VideoFrame* src0, const AVS_VideoFrame* src1, AVS_VideoFrame* dst, const float timestep, const RIFEData* const __restrict d) noexcept
{
    const auto width{ g_avs_api->avs_get_row_size_p(src0, AVS_DEFAULT_PLANE) / g_avs_api->avs_component_size(&d->fi->vi) };
    const auto height{ g_avs_api->avs_get_height_p(src0, AVS_DEFAULT_PLANE) };
    const auto src_stride{ g_avs_api->avs_get_pitch_p(src0, AVS_DEFAULT_PLANE) / g_avs_api->avs_component_size(&d->fi->vi) };
    const auto dst_stride{ g_avs_api->avs_get_pitch_p(dst, AVS_DEFAULT_PLANE) / g_avs_api->avs_component_size(&d->fi->vi) };
    auto src0R{ reinterpret_cast<const float*>(g_avs_api->avs_get_read_ptr_p(src0, AVS_PLANAR_R)) };
    auto src0G{ reinterpret_cast<const float*>(g_avs_api->avs_get_read_ptr_p(src0, AVS_PLANAR_G)) };
    auto src0B{ reinterpret_cast<const float*>(g_avs_api->avs_get_read_ptr_p(src0, AVS_PLANAR_B)) };
    auto src1R{ reinterpret_cast<const float*>(g_avs_api->avs_get_read_ptr_p(src1, AVS_PLANAR_R)) };
    auto src1G{ reinterpret_cast<const float*>(g_avs_api->avs_get_read_ptr_p(src1, AVS_PLANAR_G)) };
    auto src1B{ reinterpret_cast<const float*>(g_avs_api->avs_get_read_ptr_p(src1, AVS_PLANAR_B)) };
    auto dstR{ reinterpret_cast<float*>(g_avs_api->avs_get_write_ptr_p(dst, AVS_PLANAR_R)) };
    auto dstG{ reinterpret_cast<float*>(g_avs_api->avs_get_write_ptr_p(dst, AVS_PLANAR_G)) };
    auto dstB{ reinterpret_cast<float*>(g_avs_api->avs_get_write_ptr_p(dst, AVS_PLANAR_B)) };

    d->semaphore->acquire();
    d->rife->process(src0R, src0G, src0B, src1R, src1G, src1B, dstR, dstG, dstB, width, height, src_stride, dst_stride, timestep);
    d->semaphore->release();
}

/* multiplies and divides a rational number, such as a frame duration, in place and reduces the result */
static AVS_FORCEINLINE void muldivRational(unsigned* num, unsigned* den, int64_t mul, int64_t div)
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
static AVS_FORCEINLINE const double get_sad_c(const AVS_VideoFrame* src, const AVS_VideoFrame* src1)
{
    const int c_pitch{ g_avs_api->avs_get_pitch_p(src, AVS_DEFAULT_PLANE) / 4 };
    const int t_pitch{ g_avs_api->avs_get_pitch_p(src1, AVS_DEFAULT_PLANE) / 4 };
    const int width{ g_avs_api->avs_get_row_size_p(src, AVS_DEFAULT_PLANE) / 4 };
    const int height{ g_avs_api->avs_get_height_p(src, AVS_DEFAULT_PLANE) };
    const float* c_plane{ reinterpret_cast<const float*>(g_avs_api->avs_get_read_ptr_p(src, AVS_DEFAULT_PLANE)) };
    const float* t_plane{ reinterpret_cast<const float*>(g_avs_api->avs_get_read_ptr_p(src1, AVS_DEFAULT_PLANE)) };

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

static AVS_FORCEINLINE AVS_VideoFrame* set_error(AVS_VideoFrame* src0, AVS_VideoFrame* dst, const std::string& error_message, AVS_Value& val1, AVS_Value& val2, AVS_Value& val3, AVS_FilterInfo* fi)
{
    RIFEData* d{ reinterpret_cast<RIFEData*>(fi->user_data) };

    g_avs_api->avs_release_value(val3);
    g_avs_api->avs_release_value(val2);
    g_avs_api->avs_release_value(val1);
    g_avs_api->avs_release_video_frame(dst);
    g_avs_api->avs_release_video_frame(src0);

    d->msg = "RIFE: " + error_message;
    fi->error = d->msg.c_str();

    return nullptr;
};

static AVS_FORCEINLINE void copy_frame(const AVS_VideoFrame* src, AVS_VideoFrame* dst, AVS_ScriptEnvironment* env)
{
    constexpr int planes[3]{ AVS_PLANAR_R, AVS_PLANAR_G, AVS_PLANAR_B };

    for (int i{ 0 }; i < 3; ++i)
        g_avs_api->avs_bit_blt(env, g_avs_api->avs_get_write_ptr_p(dst, planes[i]), g_avs_api->avs_get_pitch_p(dst, planes[i]),
            g_avs_api->avs_get_read_ptr_p(src, planes[i]), g_avs_api->avs_get_pitch_p(src, planes[i]), g_avs_api->avs_get_row_size_p(src, planes[i]),
            g_avs_api->avs_get_height_p(src, planes[i]));
};

static AVS_FORCEINLINE void avg_frame(const AVS_VideoFrame* src0, const AVS_VideoFrame* src1, AVS_VideoFrame* dst)
{
    const int src_pitch0{ g_avs_api->avs_get_pitch_p(src0, AVS_DEFAULT_PLANE) >> 2 };
    const int src_pitch1{ g_avs_api->avs_get_pitch_p(src1, AVS_DEFAULT_PLANE) >> 2 };
    const int dst_pitch{ g_avs_api->avs_get_pitch_p(dst, AVS_DEFAULT_PLANE) >> 2 };
    const int width{ g_avs_api->avs_get_row_size_p(src0, AVS_DEFAULT_PLANE) >> 2 };
    const int height{ g_avs_api->avs_get_height_p(src0, AVS_DEFAULT_PLANE) };

    constexpr int planes[3]{ AVS_PLANAR_R, AVS_PLANAR_G, AVS_PLANAR_B };

    for (int i{ 0 }; i < 3; ++i)
    {
        const float* srcp0{ reinterpret_cast<const float*>(g_avs_api->avs_get_read_ptr_p(src0, planes[i])) };
        const float* srcp1{ reinterpret_cast<const float*>(g_avs_api->avs_get_read_ptr_p(src1, planes[i])) };
        float* __restrict dstp{ reinterpret_cast<float*>(g_avs_api->avs_get_write_ptr_p(dst, planes[i])) };

        for (int y{ 0 }; y < height; ++y)
        {
            for (int x{ 0 }; x < width; ++x)
                dstp[x] = (srcp0[x] + srcp1[x]) * 0.5f;

            srcp0 += src_pitch0;
            srcp1 += src_pitch1;
            dstp += dst_pitch;
        }
    }
};

template <bool sc, bool sc1, bool skip, bool denoise>
static AVS_VideoFrame* AVSC_CC RIFE_get_frame(AVS_FilterInfo* fi, int n)
{
    RIFEData* d{ static_cast<RIFEData*>(fi->user_data) };

    auto frameNum{ (denoise) ? n : static_cast<int>(n * d->factorDen / d->factorNum) };
    auto remainder{ n * d->factorDen % d->factorNum };

    auto src0{ g_avs_api->avs_get_frame(fi->child, (denoise) ? (std::max)(frameNum - d->tr, 0) : frameNum) };
    if (!src0)
        return nullptr;

    AVS_VideoFrame* dst{ g_avs_api->avs_new_video_frame_p(fi->env, &fi->vi, src0) };

    if constexpr (!denoise)
    {
        if (remainder != 0 && n < fi->vi.num_frames - d->factor)
        {
            bool sceneChange{};
            double psnrY{ -1.0 };

            if constexpr (sc || sc1)
            {
                AVS_Value v{ avs_void };
                AVS_Value cl;
                g_avs_api->avs_set_to_clip(&cl, fi->child);
                AVS_Value args_[5]{ cl, avs_new_value_bool(false), avs_new_value_string("pc709"), avs_new_value_string("left"), avs_new_value_string("spline36") };
                AVS_Value inv{ g_avs_api->avs_invoke(fi->env, "ConvertToYUV420", avs_new_value_array(args_, 5), 0) };
                if (avs_is_error(inv))
                    return set_error(src0, dst, "cannot convert to YUV420. (sc)", inv, cl, v, fi);

                AVS_Clip* abs{ g_avs_api->avs_take_clip(inv, fi->env) };

                g_avs_api->avs_release_value(inv);
                g_avs_api->avs_release_value(cl);

                AVS_VideoFrame* abs_diff{ g_avs_api->avs_get_frame(abs, frameNum) };
                AVS_VideoFrame* abs_diff1{ g_avs_api->avs_get_frame(abs, frameNum + 1) };
                sceneChange = get_sad_c(abs_diff, abs_diff1) > d->sc_threshold;

                g_avs_api->avs_release_video_frame(abs_diff1);
                g_avs_api->avs_release_video_frame(abs_diff);
                g_avs_api->avs_release_clip(abs);
            }

            if constexpr (skip)
            {
                // resized clip
                AVS_Value v{ avs_void };
                AVS_Value cl;
                g_avs_api->avs_set_to_clip(&cl, fi->child);
                AVS_Value args_[5]{ cl, avs_new_value_int((std::min)(fi->vi.width, 512)), avs_new_value_int((std::min)(fi->vi.height, 512)), avs_new_value_float(0.0), avs_new_value_float(0.5) };
                AVS_Value inv{ g_avs_api->avs_invoke(fi->env, "BicubicResize", avs_new_value_array(args_, 5), 0) };
                if (avs_is_error(inv))
                    return set_error(src0, dst, "cannot resize. (skip)", inv, cl, v, fi);

                g_avs_api->avs_release_value(cl);

                // yuv420
                AVS_Value args1_[5]{ inv, avs_new_value_bool(false), avs_new_value_string("pc709"), avs_new_value_string("left"), avs_new_value_string("spline36") };
                AVS_Value inv1{ g_avs_api->avs_invoke(fi->env, "ConvertToYUV420", avs_new_value_array(args1_, 5), 0) };
                if (avs_is_error(inv1))
                    return set_error(src0, dst, "cannot convert to YUV420. (skip)", inv1, inv, v, fi);

                g_avs_api->avs_release_value(inv);

                // 8-bit
                AVS_Value args2_[7]{ inv1, avs_new_value_int(8), avs_new_value_bool(false), avs_new_value_int(-1), avs_new_value_int(8), avs_new_value_bool(true), avs_new_value_bool(false) };
                AVS_Value src_8bit{ g_avs_api->avs_invoke(fi->env, "ConvertBits", avs_new_value_array(args2_, 7), 0) };
                if (avs_is_error(src_8bit))
                    return set_error(src0, dst, "cannot ConvertBits. (skip)", src_8bit, inv1, v, fi);

                g_avs_api->avs_release_value(inv1);

                // add frame at the end
                AVS_Value args3_[2]{ src_8bit, avs_new_value_int(d->oldNumFrames - 1) };
                inv = g_avs_api->avs_invoke(fi->env, "DuplicateFrame", avs_new_value_array(args3_, 2), 0);
                if (avs_is_error(inv))
                    return set_error(src0, dst, "cannot DuplicateFrame. (skip)", inv, src_8bit, v, fi);

                // trim the first frme
                AVS_Value args4_[3]{ inv, avs_new_value_int(1), avs_new_value_int(0) };
                inv1 = g_avs_api->avs_invoke(fi->env, "Trim", avs_new_value_array(args4_, 3), 0);
                if (avs_is_error(inv1))
                    return set_error(src0, dst, "cannot Trim. (skip)", inv, inv1, src_8bit, fi);

                g_avs_api->avs_release_value(inv);

                // vmaf with n and n+1
                AVS_Value args5_[3]{ src_8bit, inv1, avs_new_value_int(0) };
                inv = g_avs_api->avs_invoke(fi->env, "VMAF2", avs_new_value_array(args5_, 3), 0);
                if (avs_is_error(inv))
                    return set_error(src0, dst, "VMAF2 is required. (skip)", inv, inv1, src_8bit, fi);

                AVS_Clip* psnr_clip{ g_avs_api->avs_take_clip(inv, fi->env) };

                g_avs_api->avs_release_value(inv);
                g_avs_api->avs_release_value(inv1);
                g_avs_api->avs_release_value(src_8bit);

                AVS_VideoFrame* psnr{ g_avs_api->avs_get_frame(psnr_clip, frameNum) };
                psnrY = g_avs_api->avs_prop_get_float(fi->env, g_avs_api->avs_get_frame_props_ro(fi->env, psnr), "psnr_y", 0, nullptr);

                g_avs_api->avs_release_video_frame(psnr);
                g_avs_api->avs_release_clip(psnr_clip);
            }

            if (sceneChange || psnrY >= d->skipThreshold)
            {
                if constexpr (sc1)
                {
                    AVS_VideoFrame* src1{ g_avs_api->avs_get_frame(fi->child, frameNum + 1) };
                    avg_frame(src0, src1, dst);

                    g_avs_api->avs_release_video_frame(src1);
                }
                else
                    copy_frame(src0, dst, fi->env);
            }
            else
            {
                AVS_VideoFrame* src1{ g_avs_api->avs_get_frame(fi->child, frameNum + 1) };
                filter(src0, src1, dst, static_cast<float>(remainder) / d->factorNum, d);

                g_avs_api->avs_release_video_frame(src1);
            }
        }
        else
            copy_frame(src0, dst, fi->env);
    }
    else
    {
        bool sceneChange{};
        double psnrY{ -1.0 };

        if constexpr (sc || sc1)
        {
            AVS_Value v{ avs_void };
            AVS_Value cl;
            g_avs_api->avs_set_to_clip(&cl, fi->child);
            AVS_Value args_[5]{ cl, avs_new_value_bool(false), avs_new_value_string("pc709"), avs_new_value_string("left"), avs_new_value_string("spline36") };
            AVS_Value inv{ g_avs_api->avs_invoke(fi->env, "ConvertToYUV420", avs_new_value_array(args_, 5), 0) };
            if (avs_is_error(inv))
                return set_error(src0, dst, "cannot convert to YUV420. (sc)", inv, cl, v, fi);

            AVS_Clip* abs{ g_avs_api->avs_take_clip(inv, fi->env) };

            g_avs_api->avs_release_value(inv);
            g_avs_api->avs_release_value(cl);

            std::vector<AVS_VideoFrame*> prev;
            prev.reserve(d->tr);
            std::vector<AVS_VideoFrame*> next;
            next.reserve(d->tr);

            for (int i{ 1 }; i <= d->tr; ++i)
            {
                prev.emplace_back(g_avs_api->avs_get_frame(abs, (std::max)(frameNum - i, 0)));
                next.emplace_back(g_avs_api->avs_get_frame(abs, (std::min)((std::max)(fi->vi.num_frames - 1, d->oldNumFrames - 1), frameNum + i)));
            }

            for (int i{ 2 }; i <= d->tr && !sceneChange; ++i)
                sceneChange = get_sad_c(prev[i - 1], prev[i - 2]) > d->sc_threshold;

            if (!sceneChange)
            {
                for (int i{ 2 }; i <= d->tr && !sceneChange; ++i)
                    sceneChange = get_sad_c(next[i - 2], next[i - 1]) > d->sc_threshold;
            }

            if (!sceneChange)
            {
                AVS_VideoFrame* abs_diff{ g_avs_api->avs_get_frame(abs, frameNum) };

                sceneChange = get_sad_c(abs_diff, next[0]) > d->sc_threshold || get_sad_c(prev[0], abs_diff) > d->sc_threshold;

                g_avs_api->avs_release_video_frame(abs_diff);
            }

            for (int i{ 0 }; i < d->tr; ++i)
            {
                g_avs_api->avs_release_video_frame(prev[i]);
                g_avs_api->avs_release_video_frame(next[i]);
            }

            g_avs_api->avs_release_clip(abs);
        }

        if constexpr (skip)
        {
            // resized clip
            AVS_Value v{ avs_void };
            AVS_Value cl;
            g_avs_api->avs_set_to_clip(&cl, fi->child);
            AVS_Value args_[5]{ cl, avs_new_value_int((std::min)(fi->vi.width, 512)), avs_new_value_int((std::min)(fi->vi.height, 512)), avs_new_value_float(0.0), avs_new_value_float(0.5) };
            AVS_Value inv{ g_avs_api->avs_invoke(fi->env, "BicubicResize", avs_new_value_array(args_, 5), 0) };
            if (avs_is_error(inv))
                return set_error(src0, dst, "cannot resize. (skip)", inv, cl, v, fi);

            g_avs_api->avs_release_value(cl);

            // yuv420
            AVS_Value args1_[5]{ inv, avs_new_value_bool(false), avs_new_value_string("pc709"), avs_new_value_string("left"), avs_new_value_string("spline36") };
            AVS_Value inv1{ g_avs_api->avs_invoke(fi->env, "ConvertToYUV420", avs_new_value_array(args1_, 5), 0) };
            if (avs_is_error(inv1))
                return set_error(src0, dst, "cannot convert to YUV420. (skip)", inv1, inv, v, fi);

            g_avs_api->avs_release_value(inv);

            // 8-bit
            AVS_Value args2_[7]{ inv1, avs_new_value_int(8), avs_new_value_bool(false), avs_new_value_int(-1), avs_new_value_int(8), avs_new_value_bool(true), avs_new_value_bool(false) };
            AVS_Value src_8bit{ g_avs_api->avs_invoke(fi->env, "ConvertBits", avs_new_value_array(args2_, 7), 0) };
            if (avs_is_error(src_8bit))
                return set_error(src0, dst, "cannot ConvertBits. (skip)", src_8bit, inv1, v, fi);

            g_avs_api->avs_release_value(inv1);

            std::vector<AVS_Value> prev;
            prev.reserve(d->tr);
            std::vector<AVS_Value> next;
            next.reserve(d->tr);
            std::vector<AVS_Value> start_frames;
            start_frames.reserve(d->tr);
            std::vector<AVS_Value> end_frames;
            end_frames.reserve(d->tr);

            for (int i{ 0 }; i < d->tr; ++i)
            {
                start_frames.emplace_back(avs_new_value_int(0));
                end_frames.emplace_back(avs_new_value_int(d->oldNumFrames - 1));
            }

            AVS_Value* arr_start{ start_frames.data() };
            AVS_Value* arr_end{ end_frames.data() };

            // next
            for (int i{ 1 }; i <= d->tr; ++i)
            {
                // add frame at the end
                AVS_Value args3_[2]{ src_8bit, avs_new_value_array(arr_end, i) };
                inv = g_avs_api->avs_invoke(fi->env, "DuplicateFrame", avs_new_value_array(args3_, 2), 0);
                if (avs_is_error(inv))
                {
                    for (int j{ 0 }; j < d->tr; ++j)
                        g_avs_api->avs_release_value(next[j]);

                    return set_error(src0, dst, "cannot DuplicateFrame. (skip)", inv, src_8bit, v, fi);
                }

                // trim frame at the beginning
                AVS_Value args4_[3]{ inv, avs_new_value_int(i), avs_new_value_int(0) };
                next.emplace_back(g_avs_api->avs_invoke(fi->env, "Trim", avs_new_value_array(args4_, 3), 0));
                if (avs_is_error(next[i - 1]))
                {
                    for (int j{ 0 }; j < d->tr; ++j)
                        g_avs_api->avs_release_value(next[j]);

                    return set_error(src0, dst, "cannot Trim. (skip)", inv, v, src_8bit, fi);
                }

                g_avs_api->avs_release_value(inv);
            }

            // vmaf with n+x and n+(x+1)
            for (int i{ 1 }; i < d->tr && psnrY < d->skipThreshold; ++i)
            {
                AVS_Value args5_[3]{ next[i - 1], next[i], avs_new_value_int(0) };
                inv = g_avs_api->avs_invoke(fi->env, "VMAF2", avs_new_value_array(args5_, 3), 0);
                if (avs_is_error(inv))
                {
                    for (int j{ 0 }; j < d->tr; ++j)
                        g_avs_api->avs_release_value(next[j]);

                    return set_error(src0, dst, "VMAF2 is required. (skip)", inv, v, src_8bit, fi);
                }

                AVS_Clip* psnr_clip{ g_avs_api->avs_take_clip(inv, fi->env) };

                g_avs_api->avs_release_value(inv);

                AVS_VideoFrame* psnr{ g_avs_api->avs_get_frame(psnr_clip, frameNum) };
                psnrY = g_avs_api->avs_prop_get_float(fi->env, g_avs_api->avs_get_frame_props_ro(fi->env, psnr), "psnr_y", 0, nullptr);

                g_avs_api->avs_release_video_frame(psnr);
                g_avs_api->avs_release_clip(psnr_clip);
            }

            if (psnrY < d->skipThreshold)
            {
                // prev
                for (int i{ 1 }; i <= d->tr; ++i)
                {
                    // add frame at the beginning
                    AVS_Value args6_[2]{ src_8bit, avs_new_value_array(arr_start, i) };
                    inv = g_avs_api->avs_invoke(fi->env, "DuplicateFrame", avs_new_value_array(args6_, 2), 0);
                    if (avs_is_error(inv))
                    {
                        for (int j{ 0 }; j < d->tr; ++j)
                        {
                            g_avs_api->avs_release_value(next[j]);
                            g_avs_api->avs_release_value(prev[j]);
                        }

                        return set_error(src0, dst, "cannot DuplicateFrame. (skip)", inv, src_8bit, v, fi);
                    }

                    // trim the last frame
                    AVS_Value args7_[3]{ inv, avs_new_value_int(0), avs_new_value_int(d->oldNumFrames - 1) };
                    prev.emplace_back(g_avs_api->avs_invoke(fi->env, "Trim", avs_new_value_array(args7_, 3), 0));
                    if (avs_is_error(prev[i - 1]))
                    {
                        for (int j{ 0 }; j < d->tr; ++j)
                        {
                            g_avs_api->avs_release_value(next[j]);
                            g_avs_api->avs_release_value(prev[j]);
                        }

                        return set_error(src0, dst, "cannot Trim. (skip)", inv, v, src_8bit, fi);
                    }

                    g_avs_api->avs_release_value(inv);
                }

                // vmaf with n-(x+1) and n-x
                for (int i{ 1 }; i < d->tr && psnrY < d->skipThreshold; ++i)
                {
                    AVS_Value args8_[3]{ prev[i], prev[i - 1], avs_new_value_int(0) };
                    inv = g_avs_api->avs_invoke(fi->env, "VMAF2", avs_new_value_array(args8_, 3), 0);
                    if (avs_is_error(inv))
                    {
                        for (int j{ 0 }; j < d->tr; ++j)
                        {
                            g_avs_api->avs_release_value(next[j]);
                            g_avs_api->avs_release_value(prev[j]);
                        }

                        return set_error(src0, dst, "VMAF2 is required. (skip)", inv, v, src_8bit, fi);
                    }

                    AVS_Clip* psnr_clip{ g_avs_api->avs_take_clip(inv, fi->env) };

                    g_avs_api->avs_release_value(inv);

                    AVS_VideoFrame* psnr{ g_avs_api->avs_get_frame(psnr_clip, frameNum) };
                    psnrY = g_avs_api->avs_prop_get_float(fi->env, g_avs_api->avs_get_frame_props_ro(fi->env, psnr), "psnr_y", 0, nullptr);

                    g_avs_api->avs_release_video_frame(psnr);
                    g_avs_api->avs_release_clip(psnr_clip);
                }
            }
            if (psnrY < d->skipThreshold)
            {
                // vmaf with n and n+1
                AVS_Value args5_[3]{ src_8bit, next[0], avs_new_value_int(0) };
                inv = g_avs_api->avs_invoke(fi->env, "VMAF2", avs_new_value_array(args5_, 3), 0);
                if (avs_is_error(inv))
                {
                    for (int j{ 0 }; j < d->tr; ++j)
                    {
                        g_avs_api->avs_release_value(next[j]);
                        g_avs_api->avs_release_value(prev[j]);
                    }

                    return set_error(src0, dst, "VMAF2 is required. (skip)", inv, v, src_8bit, fi);
                }

                AVS_Clip* psnr_clip{ g_avs_api->avs_take_clip(inv, fi->env) };

                g_avs_api->avs_release_value(inv);

                AVS_VideoFrame* psnr{ g_avs_api->avs_get_frame(psnr_clip, frameNum) };
                psnrY = g_avs_api->avs_prop_get_float(fi->env, g_avs_api->avs_get_frame_props_ro(fi->env, psnr), "psnr_y", 0, nullptr);

                g_avs_api->avs_release_video_frame(psnr);
                g_avs_api->avs_release_clip(psnr_clip);
            }
            if (psnrY < d->skipThreshold)
            {
                // vmaf with n-1 and n
                AVS_Value args8_[3]{ prev[0], src_8bit, avs_new_value_int(0) };
                inv = g_avs_api->avs_invoke(fi->env, "VMAF2", avs_new_value_array(args8_, 3), 0);
                if (avs_is_error(inv))
                {
                    for (int j{ 0 }; j < d->tr; ++j)
                    {
                        g_avs_api->avs_release_value(next[j]);
                        g_avs_api->avs_release_value(prev[j]);
                    }

                    return set_error(src0, dst, "VMAF2 is required. (skip)", inv, v, src_8bit, fi);
                }

                AVS_Clip* psnr_clip{ g_avs_api->avs_take_clip(inv, fi->env) };

                g_avs_api->avs_release_value(inv);

                AVS_VideoFrame* psnr{ g_avs_api->avs_get_frame(psnr_clip, frameNum) };
                psnrY = g_avs_api->avs_prop_get_float(fi->env, g_avs_api->avs_get_frame_props_ro(fi->env, psnr), "psnr_y", 0, nullptr);

                g_avs_api->avs_release_video_frame(psnr);
                g_avs_api->avs_release_clip(psnr_clip);
            }

            for (int j{ 0 }; j < d->tr; ++j)
            {
                g_avs_api->avs_release_value(next[j]);
                g_avs_api->avs_release_value(prev[j]);
            }
            g_avs_api->avs_release_value(src_8bit);
        }

        if (sceneChange || psnrY >= d->skipThreshold)
        {
            if constexpr (sc1)
            {
                AVS_VideoFrame* src1{ g_avs_api->avs_get_frame(fi->child, (std::min)(frameNum + d->tr, (std::max)(fi->vi.num_frames - 1, d->oldNumFrames - 1))) };
                avg_frame(src0, src1, dst);

                g_avs_api->avs_release_video_frame(src1);
            }
            else
                copy_frame(g_avs_api->avs_get_frame(fi->child, frameNum), dst, fi->env);
        }
        else
        {
            AVS_VideoFrame* src1{ g_avs_api->avs_get_frame(fi->child, (std::min)(frameNum + d->tr, (std::max)(fi->vi.num_frames - 1, d->oldNumFrames - 1))) };
            filter(src0, src1, dst, 0.5f, d);

            g_avs_api->avs_release_video_frame(src1);
        }
    }

    auto props{ g_avs_api->avs_get_frame_props_rw(fi->env, dst) };
    int errNum, errDen;
    unsigned durationNum{ static_cast<unsigned>(g_avs_api->avs_prop_get_int(fi->env, props, "_DurationNum", 0, &errNum)) };
    unsigned durationDen{ static_cast<unsigned>(g_avs_api->avs_prop_get_int(fi->env, props, "_DurationDen", 0, &errDen)) };
    if (!errNum && !errDen)
    {
        muldivRational(&durationNum, &durationDen, d->factorDen, d->factorNum);
        g_avs_api->avs_prop_set_int(fi->env, props, "_DurationNum", durationNum, 0);
        g_avs_api->avs_prop_set_int(fi->env, props, "_DurationDen", durationDen, 0);
    }

    g_avs_api->avs_release_video_frame(src0);

    return dst;
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
    enum { Clip, Model, Factor_num, Factor_den, Fps_num, Fps_den, Model_path, Gpu_id, Gpu_thread, Tta, Uhd, Sc, Sc1, Sc_threshold, Skip, Skip_threshold, List_gpu, Denoise, Denoise_tr };

    auto d{ new RIFEData() };

    AVS_Clip* clip{ g_avs_api->avs_new_c_filter(env, &d->fi, avs_array_elt(args, Clip), 1) };
    AVS_Value v{ avs_void };

    const int denoise{ avs_defined(avs_array_elt(args, Denoise)) ? avs_as_bool(avs_array_elt(args, Denoise)) : 0 };

    try
    {
        if (!avs_is_planar(&d->fi->vi) ||
            !avs_is_rgb(&d->fi->vi) ||
            g_avs_api->avs_component_size(&d->fi->vi) < 4)
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
        const int sceneChange{ avs_defined(avs_array_elt(args, Sc)) ? avs_as_bool(avs_array_elt(args, Sc)) : 0 };
        const int sceneChange1{ avs_defined(avs_array_elt(args, Sc1)) ? avs_as_bool(avs_array_elt(args, Sc1)) : 0 };
        d->sc_threshold = avs_defined(avs_array_elt(args, Sc_threshold)) ? avs_as_float(avs_array_elt(args, Sc_threshold)) : 0.12;
        const int skip{ avs_defined(avs_array_elt(args, Skip)) ? avs_as_bool(avs_array_elt(args, Skip)) : 0 };
        d->skipThreshold = avs_defined(avs_array_elt(args, Skip_threshold)) ? avs_as_float(avs_array_elt(args, Skip_threshold)) : 60.0;
        d->tr = avs_defined(avs_array_elt(args, Denoise_tr)) ? avs_as_float(avs_array_elt(args, Denoise_tr)) : 1;

        if (model < 0 || model > models_num.size() - 1)
            throw "model must be between 0 and " + std::to_string(models_num.size() - 1) + " (inclusive)";
        if (factorNum < 1)
            throw "factor_num must be at least 1";
        if (factorDen < 1)
            throw "factor_den must be at least 1";
        if (fpsNum && fpsDen && !(d->fi->vi.fps_numerator && d->fi->vi.fps_denominator))
            throw "clip does not have a valid frame rate and hence fps_num and fps_den cannot be used";
        if (gpuId < 0 || gpuId >= ncnn::get_gpu_count())
            throw "invalid GPU device";
        if (auto queueCount{ ncnn::get_gpu_info(gpuId).compute_queue_count() }; gpuThread < 1 || static_cast<uint32_t>(gpuThread) > queueCount)
            throw "gpu_thread must be between 1 and " + std::to_string(queueCount) + " (inclusive)";
        if (sceneChange && sceneChange1)
            throw ("both sc and sc1 cannot be  true in the same time");
        if (d->sc_threshold < 0 || d->sc_threshold > 1)
            throw "sc_threshold must be between 0.0 and 1.0 (inclusive)";
        if (d->skipThreshold < 0 || d->skipThreshold > 60)
            throw "skip_threshold must be between 0.0 and 60.0 (inclusive)";
        if (d->tr < 1)
            throw "denoise_tr must be greater than or equal to 1.";

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

        if (!denoise)
            muldivRational(&d->fi->vi.fps_numerator, &d->fi->vi.fps_denominator, d->factorNum, d->factorDen);

        if (d->fi->vi.num_frames < 2)
            throw "clip's number of frames must be at least 2";
        if (d->fi->vi.num_frames / d->factorDen > INT_MAX / d->factorNum)
            throw "resulting clip is too long";

        d->oldNumFrames = d->fi->vi.num_frames;

        if (!denoise)
            d->fi->vi.num_frames = static_cast<int>(d->fi->vi.num_frames * d->factorNum / d->factorDen);

        d->factor = d->factorNum / d->factorDen;

        if (avs_defined(avs_array_elt(args, List_gpu)) ? avs_as_bool(avs_array_elt(args, List_gpu)) : 0)
        {
            for (auto i{ 0 }; i < ncnn::get_gpu_count(); ++i)
                d->msg += std::to_string(i) + ": " + ncnn::get_gpu_info(i).device_name() + "\n";

            AVS_Value cl;
            g_avs_api->avs_set_to_clip(&cl, clip);
            AVS_Value args_[2]{ cl, avs_new_value_string(d->msg.c_str()) };
            v = g_avs_api->avs_invoke(env, "Text", avs_new_value_array(args_, 2), 0);

            g_avs_api->avs_release_value(cl);
            g_avs_api->avs_release_clip(clip);

            if (--numGPUInstances == 0)
                ncnn::destroy_gpu_instance();

            return v;
        }

        if (modelPath.empty())
            modelPath = boost::dll::this_line_location().parent_path().generic_string() + "/models" + std::string{ (map_models.at(model)).first };

        std::ifstream ifs{ modelPath + "/flownet.param" };
        if (!ifs.is_open())
            throw "failed to load model";
        ifs.close();

        const bool rife_v2{ [&]()
        {
            if (modelPath.find("rife-v2") != std::string::npos)
                return true;
            else if (modelPath.find("rife-v3") != std::string::npos)
                return true;
            else
                return false;
        }() };
        const bool rife_v4{ [&]()
        {
            if (modelPath.find("rife-v4") != std::string::npos)
                return true;
            else if (modelPath.find("rife4") != std::string::npos)
                return true;
            else
                return false;
        }() };

        if (modelPath.find("rife") == std::string::npos)
            throw "unknown model dir type";

        if (!rife_v4 && (d->factorNum != 2 || d->factorDen != 1))
            throw "only rife-v4 model supports custom frame rate";

        if (rife_v4 && tta)
            throw "rife-v4 model does not support TTA mode";

        d->semaphore = std::make_unique<std::counting_semaphore<>>(gpuThread);
        d->rife = std::make_unique<RIFE>(gpuId, tta, uhd, 1, rife_v2, rife_v4, (map_models.at(model)).second);
        d->rife->load(modelPath);

        g_avs_api->avs_set_to_clip(&v, clip);

        d->fi->user_data = reinterpret_cast<void*>(d);
        d->fi->set_cache_hints = RIFE_set_cache_hints;
        d->fi->free_filter = free_RIFE;

        if (sceneChange)
        {
            if (skip)
                d->fi->get_frame = (denoise) ? RIFE_get_frame<true, false, true, true> : RIFE_get_frame<true, false, true, false>;
            else
                d->fi->get_frame = (denoise) ? RIFE_get_frame<true, false, false, true> : RIFE_get_frame<true, false, false, false>;
        }
        else
        {
            if (sceneChange1)
            {
                if (skip)
                    d->fi->get_frame = (denoise) ? RIFE_get_frame<false, true, true, true> : RIFE_get_frame<false, true, true, false>;
                else
                    d->fi->get_frame = (denoise) ? RIFE_get_frame<false, true, false, true> : RIFE_get_frame<false, true, false, false>;
            }
            else
            {
                if (skip)
                    d->fi->get_frame = (denoise) ? RIFE_get_frame<false, false, true, true> : RIFE_get_frame<false, false, true, false>;
                else
                    d->fi->get_frame = (denoise) ? RIFE_get_frame<false, false, false, true> : RIFE_get_frame<false, false, false, false>;
            }
        }
    }
    catch (std::string& error)
    {
        d->msg = "RIFE: " + error;
        v = avs_new_value_error(d->msg.c_str());

        if (--numGPUInstances == 0)
            ncnn::destroy_gpu_instance();
    }
    catch (const char* error)
    {
        d->msg = "RIFE: " + std::string{ error };
        v = avs_new_value_error(d->msg.c_str());

        if (--numGPUInstances == 0)
            ncnn::destroy_gpu_instance();
    }

    g_avs_api->avs_release_clip(clip);

    return v;
}

const char* AVSC_CC avisynth_c_plugin_init(AVS_ScriptEnvironment* env)
{
    static constexpr int REQUIRED_INTERFACE_VERSION{ 9 };
    static constexpr int REQUIRED_BUGFIX_VERSION{ 2 };
    static constexpr std::initializer_list<std::string_view> required_functions
    {
        "avs_get_frame",
        "avs_get_pitch_p",
        "avs_get_row_size_p",
        "avs_get_height_p",
        "avs_get_write_ptr_p",
        "avs_get_read_ptr_p",
        "avs_get_frame_props_rw",
        "avs_release_video_frame",
        "avs_release_clip",
        "avs_new_c_filter",
        "avs_add_function",
        "avs_release_value",
        "avs_bit_blt",
        "avs_set_to_clip",
        "avs_take_clip",
        "avs_invoke",
        "avs_prop_set_int",
        "avs_prop_get_int",
        "avs_prop_get_float",
        "avs_new_video_frame_p",
        "avs_get_frame_props_ro"
    };

    if (!avisynth_c_api_loader::get_api(env, REQUIRED_INTERFACE_VERSION, REQUIRED_BUGFIX_VERSION, required_functions))
    {
        std::cerr << avisynth_c_api_loader::get_last_error() << std::endl;
        return avisynth_c_api_loader::get_last_error();
    }

    g_avs_api->avs_add_function(env, "RIFE", "c[model]i[factor_num]i[factor_den]i[fps_num]i[fps_den]i[model_path]s[gpu_id]i[gpu_thread]i[tta]b[uhd]b[sc]b[sc1]b[sc_threshold]f[skip]b[skip_threshold]f[list_gpu]b[denoise]b[denoise_tr]i", Create_RIFE, 0);
    return "Real-Time Intermediate Flow Estimation for Video Frame Interpolation";
}
