// Copyright (c) 2021-2022 HolyWu
// Copyright (c) 2022-2026 Asd-g
// SPDX-License-Identifier: MIT

#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <semaphore>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "avs_c_api_loader.hpp"
#include "rife.h"

#if defined(__linux__) || defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

static std::atomic<int> numGPUInstances{ 0 };
static std::mutex g_global_mutex;
static std::unique_ptr<std::counting_semaphore<1024>> g_global_semaphore;

struct ModelKey {
    std::string modelPath;
    int gpuId;
    bool tta;
    bool uhd;
    bool rife_v2;
    bool rife_v4;
    int padding;

    auto operator<=>(const ModelKey&) const = default;
};

static std::map<ModelKey, std::weak_ptr<RIFE>> g_model_cache;

inline std::filesystem::path get_current_module_path()
{
#ifdef _WIN32
    HMODULE hModule{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&get_current_module_path), &hModule))
        return std::filesystem::path();

    wchar_t path_buf[MAX_PATH];
    DWORD result{ GetModuleFileNameW(hModule, path_buf, MAX_PATH) };
    if (!result)
        return std::filesystem::path();
    if (result == MAX_PATH)
        return std::filesystem::path();

    return std::filesystem::path(path_buf);
#elif defined(__linux__) || defined(__APPLE__)
    Dl_info dl_info;
    if (!dladdr(reinterpret_cast<void*>(&get_current_module_path), &dl_info))
        return std::filesystem::path();
    if (!dl_info.dli_fname)
        return std::filesystem::path();

    return std::filesystem::path(dl_info.dli_fname);
#endif
}

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
    std::make_tuple(0, "rife", 32),
    std::make_tuple(1, "rife-HD", 32),
    std::make_tuple(2, "rife-UHD", 32),
    std::make_tuple(3, "rife-anime", 32),
    std::make_tuple(4, "rife-v2", 32),
    std::make_tuple(5, "rife-v2.3", 32),
    std::make_tuple(6, "rife-v2.4", 32),
    std::make_tuple(7, "rife-v3.0", 32),
    std::make_tuple(8, "rife-v3.1", 32),
    std::make_tuple(9, "rife-v3.9_ensembleFalse_fastTrue", 32),
    std::make_tuple(10, "rife-v3.9_ensembleTrue_fastFalse", 32),
    std::make_tuple(11, "rife-v4_ensembleFalse_fastTrue", 32),
    std::make_tuple(12, "rife-v4_ensembleTrue_fastFalse", 32),
    std::make_tuple(13, "rife-v4.1_ensembleFalse_fastTrue", 32),
    std::make_tuple(14, "rife-v4.1_ensembleTrue_fastFalse", 32),
    std::make_tuple(15, "rife-v4.2_ensembleFalse_fastTrue", 32),
    std::make_tuple(16, "rife-v4.2_ensembleTrue_fastFalse", 32),
    std::make_tuple(17, "rife-v4.3_ensembleFalse_fastTrue", 32),
    std::make_tuple(18, "rife-v4.3_ensembleTrue_fastFalse", 32),
    std::make_tuple(19, "rife-v4.4_ensembleFalse_fastTrue", 32),
    std::make_tuple(20, "rife-v4.4_ensembleTrue_fastFalse", 32),
    std::make_tuple(21, "rife-v4.5_ensembleFalse", 32),
    std::make_tuple(22, "rife-v4.5_ensembleTrue", 32),
    std::make_tuple(23, "rife-v4.6_ensembleFalse", 32),
    std::make_tuple(24, "rife-v4.6_ensembleTrue", 32),
    std::make_tuple(25, "rife-v4.7_ensembleFalse", 32),
    std::make_tuple(26, "rife-v4.7_ensembleTrue", 32),
    std::make_tuple(27, "rife-v4.8_ensembleFalse", 32),
    std::make_tuple(28, "rife-v4.8_ensembleTrue", 32),
    std::make_tuple(29, "rife-v4.9_ensembleFalse", 32),
    std::make_tuple(30, "rife-v4.9_ensembleTrue", 32),
    std::make_tuple(31, "rife-v4.10_ensembleFalse", 32),
    std::make_tuple(32, "rife-v4.10_ensembleTrue", 32),
    std::make_tuple(33, "rife-v4.11_ensembleFalse", 32),
    std::make_tuple(34, "rife-v4.11_ensembleTrue", 32),
    std::make_tuple(35, "rife-v4.12_ensembleFalse", 32),
    std::make_tuple(36, "rife-v4.12_ensembleTrue", 32),
    std::make_tuple(37, "rife-v4.12_lite_ensembleFalse", 32),
    std::make_tuple(38, "rife-v4.12_lite_ensembleTrue", 32),
    std::make_tuple(39, "rife-v4.13_ensembleFalse", 32),
    std::make_tuple(40, "rife-v4.13_ensembleTrue", 32),
    std::make_tuple(41, "rife-v4.13_lite_ensembleFalse", 32),
    std::make_tuple(42, "rife-v4.13_lite_ensembleTrue", 32),
    std::make_tuple(43, "rife-v4.14_ensembleFalse", 32),
    std::make_tuple(44, "rife-v4.14_ensembleTrue", 32),
    std::make_tuple(45, "rife-v4.14_lite_ensembleFalse", 32),
    std::make_tuple(46, "rife-v4.14_lite_ensembleTrue", 32),
    std::make_tuple(47, "rife-v4.15_ensembleFalse", 32),
    std::make_tuple(48, "rife-v4.15_ensembleTrue", 32),
    std::make_tuple(49, "rife-v4.15_lite_ensembleFalse", 32),
    std::make_tuple(50, "rife-v4.15_lite_ensembleTrue", 32),
    std::make_tuple(51, "rife-v4.16_lite_ensembleFalse", 32),
    std::make_tuple(52, "rife-v4.16_lite_ensembleTrue", 32),
    std::make_tuple(53, "rife-v4.17_ensembleFalse", 32),
    std::make_tuple(54, "rife-v4.17_ensembleTrue", 32),
    std::make_tuple(55, "rife-v4.17_lite_ensembleFalse", 32),
    std::make_tuple(56, "rife-v4.17_lite_ensembleTrue", 32),
    std::make_tuple(57, "rife-v4.18_ensembleFalse", 32),
    std::make_tuple(58, "rife-v4.18_ensembleTrue", 32),
    std::make_tuple(59, "rife-v4.19_beta_ensembleFalse", 32),
    std::make_tuple(60, "rife-v4.19_beta_ensembleTrue", 32),
    std::make_tuple(61, "rife-v4.20_ensembleFalse", 32),
    std::make_tuple(62, "rife-v4.20_ensembleTrue", 32),
    std::make_tuple(63, "rife-v4.21_ensembleFalse", 32),
    std::make_tuple(64, "rife-v4.22_ensembleFalse", 32),
    std::make_tuple(65, "rife-v4.22_lite_ensembleFalse", 32),
    std::make_tuple(66, "rife-v4.23_beta_ensembleFalse", 32),
    std::make_tuple(67, "rife-v4.24_ensembleFalse", 32),
    std::make_tuple(68, "rife-v4.24_ensembleTrue", 32),
    std::make_tuple(69, "rife-v4.25_ensembleFalse", 64),
    std::make_tuple(70, "rife-v4.25-lite_ensembleFalse", 128),
    std::make_tuple(71, "rife-v4.25_heavy_beta_ensembleFalse", 64),
    std::make_tuple(72, "rife-v4.26_ensembleFalse", 64),
    std::make_tuple(73, "rife-v4.26-large_ensembleFalse", 64)
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
    std::shared_ptr<RIFE> rife;
    int oldNumFrames;
    int tr;
    std::array<int, 3> planes;
    int src_comp_size;
};

static void filter(const AVS_VideoFrame* src0, const AVS_VideoFrame* src1, AVS_VideoFrame* dst, const float timestep,
    const RIFEData* const __restrict d) noexcept
{
    const auto& vi{ d->fi->vi };
    const auto width{ g_avs_api->avs_get_row_size_p(src0, AVS_DEFAULT_PLANE) / d->src_comp_size };
    const auto height{ g_avs_api->avs_get_height_p(src0, AVS_DEFAULT_PLANE) };
    const ptrdiff_t dst_stride{ g_avs_api->avs_get_pitch_p(dst, AVS_DEFAULT_PLANE) / sizeof(float) };

    auto [src0_stride, src1_stride, src0_p, src1_p] {[&]() {
        struct result
        {
            ptrdiff_t src0_stride[3];
            ptrdiff_t src1_stride[3];
            const uint8_t* src0_p[3];
            const uint8_t* src1_p[3];
        } res;

        for (int i{ 0 }; i < 3; ++i)
        {
            const auto& plane{ d->planes };
            res.src0_stride[i] = g_avs_api->avs_get_pitch_p(src0, plane[i]);
            res.src1_stride[i] = g_avs_api->avs_get_pitch_p(src1, plane[i]);
            res.src0_p[i] = g_avs_api->avs_get_read_ptr_p(src0, plane[i]);
            res.src1_p[i] = g_avs_api->avs_get_read_ptr_p(src1, plane[i]);
        }

        return res;
        }()
        };

    auto dstR{ reinterpret_cast<float*>(g_avs_api->avs_get_write_ptr_p(dst, AVS_PLANAR_R)) };
    auto dstG{ reinterpret_cast<float*>(g_avs_api->avs_get_write_ptr_p(dst, AVS_PLANAR_G)) };
    auto dstB{ reinterpret_cast<float*>(g_avs_api->avs_get_write_ptr_p(dst, AVS_PLANAR_B)) };

    if (g_global_semaphore)
        g_global_semaphore->acquire();

    d->rife->process(src0_p, src1_p, dstR, dstG, dstB, width, height, src0_stride, src1_stride, dst_stride, timestep);

    if (g_global_semaphore)
        g_global_semaphore->release();
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

static AVS_FORCEINLINE void copy_frame(const AVS_VideoFrame* src, AVS_VideoFrame* dst, const RIFEData* const __restrict d)
{
    auto [stride, src_p] {[&]() {
        struct result
        {
            ptrdiff_t stride[3];
            const uint8_t* src_p[3];
        }res;

        for (int i{ 0 }; i < 3; ++i)
        {
            const auto& plane{ d->planes };
            res.stride[i] = g_avs_api->avs_get_pitch_p(src, plane[i]);
            res.src_p[i] = g_avs_api->avs_get_read_ptr_p(src, plane[i]);
        }

        return res;
        }()
        };

    auto dstR{ reinterpret_cast<float*>(g_avs_api->avs_get_write_ptr_p(dst, AVS_PLANAR_R)) };
    auto dstG{ reinterpret_cast<float*>(g_avs_api->avs_get_write_ptr_p(dst, AVS_PLANAR_G)) };
    auto dstB{ reinterpret_cast<float*>(g_avs_api->avs_get_write_ptr_p(dst, AVS_PLANAR_B)) };

    const size_t width{ g_avs_api->avs_get_row_size_p(dst, AVS_PLANAR_R) / sizeof(float) };
    const int height{ g_avs_api->avs_get_height_p(dst, AVS_PLANAR_R) };
    const size_t dst_stride{ g_avs_api->avs_get_pitch_p(dst, AVS_PLANAR_R) / sizeof(float) };

    if (g_global_semaphore)
        g_global_semaphore->acquire();

    d->rife->process_copy(src_p, dstR, dstG, dstB, width, height, stride, dst_stride);

    if (g_global_semaphore)
        g_global_semaphore->release();
};

static AVS_FORCEINLINE void avg_frame(const AVS_VideoFrame* src0, const AVS_VideoFrame* src1, AVS_VideoFrame* dst,
    AVS_ScriptEnvironment* env, const RIFEData* const __restrict d)
{
    avs_helpers::avs_video_frame_ptr tmp0{ g_avs_api->avs_new_video_frame_p(env, &d->fi->vi, dst) };
    avs_helpers::avs_video_frame_ptr tmp1{ g_avs_api->avs_new_video_frame_p(env, &d->fi->vi, dst) };

    copy_frame(src0, tmp0.get(), d);
    copy_frame(src1, tmp1.get(), d);

    const size_t src_pitch0{ g_avs_api->avs_get_pitch_p(tmp0.get(), AVS_PLANAR_R) / sizeof(float) };
    const size_t src_pitch1{ g_avs_api->avs_get_pitch_p(tmp1.get(), AVS_PLANAR_R) / sizeof(float) };
    const size_t dst_pitch{ g_avs_api->avs_get_pitch_p(dst, AVS_PLANAR_R) / sizeof(float) };
    const size_t width{ g_avs_api->avs_get_row_size_p(tmp0.get(), AVS_PLANAR_R) / sizeof(float) };
    const int height{ g_avs_api->avs_get_height_p(tmp0.get(), AVS_PLANAR_R) };

    constexpr int planes[3]{ AVS_PLANAR_R, AVS_PLANAR_G, AVS_PLANAR_B };

    for (int i{ 0 }; i < 3; ++i)
    {
        const float* srcp0{ reinterpret_cast<const float*>(g_avs_api->avs_get_read_ptr_p(tmp0.get(), planes[i])) };
        const float* srcp1{ reinterpret_cast<const float*>(g_avs_api->avs_get_read_ptr_p(tmp1.get(), planes[i])) };
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

    const auto& env{ fi->env };
    const auto& child{ fi->child };
    const auto& vi{ fi->vi };

    const avs_helpers::avs_video_frame_ptr src0{ g_avs_api->avs_get_frame(child, (denoise) ? (std::max)(frameNum - d->tr, 0)
        : frameNum) };
    if (!src0)
        return nullptr;

    avs_helpers::avs_video_frame_ptr dst{ g_avs_api->avs_new_video_frame_p(env, &vi, src0.get()) };

    const auto set_error{ [&](std::string_view s) {
        fi->error = g_avs_api->avs_save_string(env, s.data(), static_cast<int>(s.size()));

        return nullptr;
            } };

    if constexpr (!denoise)
    {
        if (remainder != 0 && n < vi.num_frames - d->factor)
        {
            bool sceneChange{};
            double psnrY{ -1.0 };

            if constexpr (sc || sc1)
            {
                AVS_Value cl;
                g_avs_api->avs_set_to_clip(&cl, child);
                avs_helpers::avs_value_guard cl_guard(cl);
                AVS_Value args_[5]{ cl_guard.get(), avs_new_value_bool(false), avs_new_value_string("pc709"), avs_new_value_string("left"),
                    avs_new_value_string("spline36") };
                avs_helpers::avs_value_guard inv_guard{ g_avs_api->avs_invoke(env, "ConvertToYUV420", avs_new_value_array(args_, 5), 0) };
                if (avs_is_error(inv_guard.get()))
                    return set_error("RIFE: cannot convert to YUV420. (sc)");

                avs_helpers::avs_clip_ptr abs{ g_avs_api->avs_take_clip(inv_guard.get(), env) };

                avs_helpers::avs_video_frame_ptr abs_diff{ g_avs_api->avs_get_frame(abs.get(), frameNum) };
                avs_helpers::avs_video_frame_ptr abs_diff1{ g_avs_api->avs_get_frame(abs.get(), frameNum + 1) };
                sceneChange = get_sad_c(abs_diff.get(), abs_diff1.get()) > d->sc_threshold;
            }

            if constexpr (skip)
            {
                // resized clip
                AVS_Value cl;
                g_avs_api->avs_set_to_clip(&cl, child);
                avs_helpers::avs_value_guard cl_guard(cl);
                AVS_Value args_[5]{ cl_guard.get(), avs_new_value_int((std::min)(vi.width, 512)),
                    avs_new_value_int((std::min)(vi.height, 512)), avs_new_value_float(0.0), avs_new_value_float(0.5) };
                avs_helpers::avs_value_guard inv_guard{ g_avs_api->avs_invoke(env, "BicubicResize", avs_new_value_array(args_, 5), 0) };
                if (avs_is_error(inv_guard.get()))
                    return set_error("RIFE: cannot resize. (skip)");

                // yuv420
                AVS_Value args1_[5]{ inv_guard.get(), avs_new_value_bool(false), avs_new_value_string("pc709"),
                    avs_new_value_string("left"), avs_new_value_string("spline36") };
                avs_helpers::avs_value_guard inv1_guard{ g_avs_api->avs_invoke(env, "ConvertToYUV420", avs_new_value_array(args1_, 5), 0) };
                if (avs_is_error(inv1_guard.get()))
                    return set_error("RIFE: cannot convert to YUV420. (skip)");

                // 8-bit
                AVS_Value args2_[7]{ inv1_guard.get(), avs_new_value_int(8), avs_new_value_bool(false), avs_new_value_int(-1),
                    avs_new_value_int(8), avs_new_value_bool(true), avs_new_value_bool(false) };
                avs_helpers::avs_value_guard src_8bit_guard{ g_avs_api->avs_invoke(env, "ConvertBits", avs_new_value_array(args2_, 7), 0) };
                if (avs_is_error(src_8bit_guard.get()))
                    return set_error("RIFE: cannot ConvertBits. (skip)");

                // add frame at the end
                AVS_Value args3_[2]{ src_8bit_guard.get(), avs_new_value_int(d->oldNumFrames - 1) };
                inv_guard.reset(g_avs_api->avs_invoke(env, "DuplicateFrame", avs_new_value_array(args3_, 2), 0));
                if (avs_is_error(inv_guard.get()))
                    return set_error("RIFE: cannot DuplicateFrame. (skip)");

                // trim the first frme
                AVS_Value args4_[3]{ inv_guard.get(), avs_new_value_int(1), avs_new_value_int(0) };
                inv1_guard.reset(g_avs_api->avs_invoke(env, "Trim", avs_new_value_array(args4_, 3), 0));
                if (avs_is_error(inv1_guard.get()))
                    return set_error("RIFE: cannot Trim. (skip)");

                // vmaf with n and n+1
                AVS_Value args5_[3]{ src_8bit_guard.get(), inv1_guard.get(), avs_new_value_int(0) };
                inv_guard.reset(g_avs_api->avs_invoke(env, "VMAF2", avs_new_value_array(args5_, 3), 0));
                if (avs_is_error(inv_guard.get()))
                    return set_error("VMAF2 is required. (skip)");

                avs_helpers::avs_clip_ptr psnr_clip{ g_avs_api->avs_take_clip(inv_guard.get(), env) };

                avs_helpers::avs_video_frame_ptr psnr{ g_avs_api->avs_get_frame(psnr_clip.get(), frameNum) };
                psnrY = g_avs_api->avs_prop_get_float(env, g_avs_api->avs_get_frame_props_ro(env, psnr.get()), "psnr_y", 0, nullptr);
            }

            if (sceneChange || psnrY >= d->skipThreshold)
            {
                if constexpr (sc1)
                {
                    avs_helpers::avs_video_frame_ptr src1{ g_avs_api->avs_get_frame(child, frameNum + 1) };
                    avg_frame(src0.get(), src1.get(), dst.get(), env, d);
                }
                else
                    copy_frame(src0.get(), dst.get(), d);
            }
            else
            {
                avs_helpers::avs_video_frame_ptr src1{ g_avs_api->avs_get_frame(child, frameNum + 1) };
                filter(src0.get(), src1.get(), dst.get(), static_cast<float>(remainder) / d->factorNum, d);
            }
        }
        else
            copy_frame(src0.get(), dst.get(), d);
    }
    else
    {
        bool sceneChange{};
        double psnrY{ -1.0 };

        if constexpr (sc || sc1)
        {
            AVS_Value cl;
            g_avs_api->avs_set_to_clip(&cl, child);
            avs_helpers::avs_value_guard cl_guard(cl);
            AVS_Value args_[5]{ cl_guard.get(), avs_new_value_bool(false), avs_new_value_string("pc709"), avs_new_value_string("left"),
                avs_new_value_string("spline36") };
            avs_helpers::avs_value_guard inv_guard{ g_avs_api->avs_invoke(env, "ConvertToYUV420", avs_new_value_array(args_, 5), 0) };
            if (avs_is_error(inv_guard.get()))
                return set_error("cannot convert to YUV420. (sc)");

            avs_helpers::avs_clip_ptr abs{ g_avs_api->avs_take_clip(inv_guard.get(), env) };

            std::vector<avs_helpers::avs_video_frame_ptr> prev;
            prev.reserve(d->tr);
            std::vector<avs_helpers::avs_video_frame_ptr> next;
            next.reserve(d->tr);

            for (int i{ 1 }; i <= d->tr; ++i)
            {
                prev.emplace_back(g_avs_api->avs_get_frame(abs.get(), (std::max)(frameNum - i, 0)));
                next.emplace_back(g_avs_api->avs_get_frame(abs.get(), (std::min)((std::max)(vi.num_frames - 1, d->oldNumFrames - 1),
                    frameNum + i)));
            }

            for (int i{ 2 }; i <= d->tr && !sceneChange; ++i)
                sceneChange = get_sad_c(prev[i - 1].get(), prev[i - 2].get()) > d->sc_threshold;

            if (!sceneChange)
            {
                for (int i{ 2 }; i <= d->tr && !sceneChange; ++i)
                    sceneChange = get_sad_c(next[i - 2].get(), next[i - 1].get()) > d->sc_threshold;
            }

            if (!sceneChange)
            {
                avs_helpers::avs_video_frame_ptr abs_diff{ g_avs_api->avs_get_frame(abs.get(), frameNum) };

                sceneChange = get_sad_c(abs_diff.get(), next[0].get()) > d->sc_threshold || get_sad_c(prev[0].get(),
                    abs_diff.get()) > d->sc_threshold;
            }
        }

        if constexpr (skip)
        {
            // resized clip
            AVS_Value cl;
            g_avs_api->avs_set_to_clip(&cl, child);
            avs_helpers::avs_value_guard cl_guard(cl);
            AVS_Value args_[5]{ cl_guard.get(), avs_new_value_int((std::min)(vi.width, 512)), avs_new_value_int((std::min)(vi.height, 512)),
                avs_new_value_float(0.0), avs_new_value_float(0.5) };
            avs_helpers::avs_value_guard inv_guard{ g_avs_api->avs_invoke(env, "BicubicResize", avs_new_value_array(args_, 5), 0) };
            if (avs_is_error(inv_guard.get()))
                return set_error("RIFE: cannot resize. (skip)");

            // yuv420
            AVS_Value args1_[5]{ inv_guard.get(), avs_new_value_bool(false), avs_new_value_string("pc709"), avs_new_value_string("left"),
                avs_new_value_string("spline36") };
            avs_helpers::avs_value_guard inv1_guard{ g_avs_api->avs_invoke(env, "ConvertToYUV420", avs_new_value_array(args1_, 5), 0) };
            if (avs_is_error(inv1_guard.get()))
                return set_error("RIFE: cannot convert to YUV420. (skip)");

            // 8-bit
            AVS_Value args2_[7]{ inv1_guard.get(), avs_new_value_int(8), avs_new_value_bool(false), avs_new_value_int(-1),
                avs_new_value_int(8), avs_new_value_bool(true), avs_new_value_bool(false) };
            avs_helpers::avs_value_guard src_8bit_guard{ g_avs_api->avs_invoke(env, "ConvertBits", avs_new_value_array(args2_, 7), 0) };
            if (avs_is_error(src_8bit_guard.get()))
                return set_error("RIFE: cannot ConvertBits. (skip)");

            std::vector<avs_helpers::avs_value_guard> prev;
            prev.reserve(d->tr);
            std::vector<avs_helpers::avs_value_guard> next;
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

            // next
            for (int i{ 1 }; i <= d->tr; ++i)
            {
                // add frame at the end
                AVS_Value args3_[2]{ src_8bit_guard.get(), avs_new_value_array(end_frames.data(), i) };
                inv_guard.reset(g_avs_api->avs_invoke(env, "DuplicateFrame", avs_new_value_array(args3_, 2), 0));
                if (avs_is_error(inv_guard.get()))
                    return set_error("RIFE: cannot DuplicateFrame. (skip)");

                // trim frame at the beginning
                AVS_Value args4_[3]{ inv_guard.get(), avs_new_value_int(i), avs_new_value_int(0) };
                next.emplace_back(g_avs_api->avs_invoke(env, "Trim", avs_new_value_array(args4_, 3), 0));
                if (avs_is_error(next[i - 1].get()))
                    return set_error("RIFE: cannot Trim. (skip)");
            }

            // vmaf with n+x and n+(x+1)
            for (int i{ 1 }; i < d->tr && psnrY < d->skipThreshold; ++i)
            {
                AVS_Value args5_[3]{ next[i - 1].get(), next[i].get(), avs_new_value_int(0) };
                inv_guard.reset(g_avs_api->avs_invoke(env, "VMAF2", avs_new_value_array(args5_, 3), 0));
                if (avs_is_error(inv_guard.get()))
                    return set_error("RIFE: VMAF2 is required. (skip)");

                avs_helpers::avs_clip_ptr psnr_clip{ g_avs_api->avs_take_clip(inv_guard.get(), env) };

                avs_helpers::avs_video_frame_ptr psnr{ g_avs_api->avs_get_frame(psnr_clip.get(), frameNum) };
                psnrY = g_avs_api->avs_prop_get_float(env, g_avs_api->avs_get_frame_props_ro(env, psnr.get()), "psnr_y", 0, nullptr);
            }

            if (psnrY < d->skipThreshold)
            {
                // prev
                for (int i{ 1 }; i <= d->tr; ++i)
                {
                    // add frame at the beginning
                    AVS_Value args6_[2]{ src_8bit_guard.get(), avs_new_value_array(start_frames.data(), i) };
                    inv_guard.reset(g_avs_api->avs_invoke(env, "DuplicateFrame", avs_new_value_array(args6_, 2), 0));
                    if (avs_is_error(inv_guard.get()))
                        return set_error("RIFE: cannot DuplicateFrame. (skip)");

                    // trim the last frame
                    AVS_Value args7_[3]{ inv_guard.get(), avs_new_value_int(0), avs_new_value_int(d->oldNumFrames - 1) };
                    prev.emplace_back(g_avs_api->avs_invoke(env, "Trim", avs_new_value_array(args7_, 3), 0));
                    if (avs_is_error(prev[i - 1].get()))
                        return set_error("RIFE: cannot Trim. (skip)");
                }

                // vmaf with n-(x+1) and n-x
                for (int i{ 1 }; i < d->tr && psnrY < d->skipThreshold; ++i)
                {
                    AVS_Value args8_[3]{ prev[i].get(), prev[i - 1].get(), avs_new_value_int(0) };
                    inv_guard.reset(g_avs_api->avs_invoke(env, "VMAF2", avs_new_value_array(args8_, 3), 0));
                    if (avs_is_error(inv_guard.get()))
                        return set_error("RIFE: VMAF2 is required. (skip)");

                    avs_helpers::avs_clip_ptr psnr_clip{ g_avs_api->avs_take_clip(inv_guard.get(), env) };

                    avs_helpers::avs_video_frame_ptr psnr{ g_avs_api->avs_get_frame(psnr_clip.get(), frameNum) };
                    psnrY = g_avs_api->avs_prop_get_float(env, g_avs_api->avs_get_frame_props_ro(env, psnr.get()), "psnr_y", 0, nullptr);
                }
            }
            if (psnrY < d->skipThreshold)
            {
                // vmaf with n and n+1
                AVS_Value args5_[3]{ src_8bit_guard.get(), next[0].get(), avs_new_value_int(0) };
                inv_guard.reset(g_avs_api->avs_invoke(env, "VMAF2", avs_new_value_array(args5_, 3), 0));
                if (avs_is_error(inv_guard.get()))
                    return set_error("RIFE: VMAF2 is required. (skip)");

                avs_helpers::avs_clip_ptr psnr_clip{ g_avs_api->avs_take_clip(inv_guard.get(), env) };

                avs_helpers::avs_video_frame_ptr psnr{ g_avs_api->avs_get_frame(psnr_clip.get(), frameNum) };
                psnrY = g_avs_api->avs_prop_get_float(env, g_avs_api->avs_get_frame_props_ro(env, psnr.get()), "psnr_y", 0, nullptr);
            }
            if (psnrY < d->skipThreshold)
            {
                // vmaf with n-1 and n
                AVS_Value args8_[3]{ prev[0].get(), src_8bit_guard.get(), avs_new_value_int(0) };
                inv_guard.reset(g_avs_api->avs_invoke(env, "VMAF2", avs_new_value_array(args8_, 3), 0));
                if (avs_is_error(inv_guard.get()))
                    return set_error("RIFE: VMAF2 is required. (skip)");

                avs_helpers::avs_clip_ptr psnr_clip{ g_avs_api->avs_take_clip(inv_guard.get(), env) };

                avs_helpers::avs_video_frame_ptr psnr{ g_avs_api->avs_get_frame(psnr_clip.get(), frameNum) };
                psnrY = g_avs_api->avs_prop_get_float(env, g_avs_api->avs_get_frame_props_ro(env, psnr.get()), "psnr_y", 0, nullptr);
            }
        }

        if (sceneChange || psnrY >= d->skipThreshold)
        {
            if constexpr (sc1)
            {
                avs_helpers::avs_video_frame_ptr src1{ g_avs_api->avs_get_frame(child, (std::min)(frameNum + d->tr,
                    (std::max)(vi.num_frames - 1, d->oldNumFrames - 1))) };
                avg_frame(src0.get(), src1.get(), dst.get(), env, d);
            }
            else
                copy_frame(g_avs_api->avs_get_frame(child, frameNum), dst.get(), d);
        }
        else
        {
            avs_helpers::avs_video_frame_ptr src1{ g_avs_api->avs_get_frame(child, (std::min)(frameNum + d->tr,
                (std::max)(vi.num_frames - 1, d->oldNumFrames - 1))) };
            filter(src0.get(), src1.get(), dst.get(), 0.5f, d);
        }
    }

    auto props{ g_avs_api->avs_get_frame_props_rw(env, dst.get()) };
    int errNum, errDen;
    unsigned durationNum{ static_cast<unsigned>(g_avs_api->avs_prop_get_int(env, props, "_DurationNum", 0, &errNum)) };
    unsigned durationDen{ static_cast<unsigned>(g_avs_api->avs_prop_get_int(env, props, "_DurationDen", 0, &errDen)) };
    if (!errNum && !errDen)
    {
        muldivRational(&durationNum, &durationDen, d->factorDen, d->factorNum);
        g_avs_api->avs_prop_set_int(env, props, "_DurationNum", durationNum, 0);
        g_avs_api->avs_prop_set_int(env, props, "_DurationDen", durationDen, 0);
    }

    return dst.release();
}

static void AVSC_CC free_RIFE(AVS_FilterInfo* fi)
{
    auto d{ static_cast<RIFEData*>(fi->user_data) };
    delete d;

    if (--numGPUInstances == 0)
    {
        std::lock_guard lock(g_global_mutex);
        g_model_cache.clear();
        g_global_semaphore.reset();
        ncnn::destroy_gpu_instance();
    }
}

static int AVSC_CC RIFE_set_cache_hints(AVS_FilterInfo* fi, int cachehints, int frame_range)
{
    return cachehints == AVS_CACHE_GET_MTMODE ? 2 : 0;
}

static AVS_Value AVSC_CC Create_RIFE(AVS_ScriptEnvironment* env, AVS_Value args, void* param)
{
    enum {
        Clip, Model, Factor_num, Factor_den, Fps_num, Fps_den, Model_path, Gpu_id, Gpu_thread, Tta, Uhd, Sc, Sc1, Sc_threshold, Skip,
        Skip_threshold, List_gpu, Denoise, Denoise_tr, Matrinx_in, Full_range
    };

    auto d{ std::make_unique<RIFEData>() };
    auto& fi{ d->fi };
    AVS_Value v{};

    avs_helpers::avs_clip_ptr clip{ g_avs_api->avs_new_c_filter(env, &fi, avs_array_elt(args, Clip), 1) };

    if (const auto s{ avs_helpers::get_opt_arg<bool>(env, args, List_gpu) }; s && *s)
    {
        std::string msg;
        for (auto i{ 0 }; i < ncnn::get_gpu_count(); ++i)
            msg += std::to_string(i) + ": " + ncnn::get_gpu_info(i).device_name() + "\n";

        AVS_Value cl;
        g_avs_api->avs_set_to_clip(&cl, clip.get());
        avs_helpers::avs_value_guard cl_guard(cl);
        AVS_Value args_[2]{ cl_guard.get(), avs_new_value_string(msg.c_str()) };
        v = g_avs_api->avs_invoke(env, "Text", avs_new_value_array(args_, 2), 0);

        fi->user_data = reinterpret_cast<void*>(d.release());
        fi->free_filter = free_RIFE;

        return v;
    }

    auto& vi{ fi->vi };
    const int denoise{ avs_helpers::get_opt_arg<bool>(env, args, Denoise).value_or(0) };

    try
    {
        if (!avs_is_planar(&vi))
            throw "only planar formats supported";

        {
            std::lock_guard lock(g_global_mutex);
            if (numGPUInstances == 0)
            {
                if (ncnn::create_gpu_instance())
                    throw "failed to create GPU instance";
            }

            ++numGPUInstances;
        }

        const auto model{ avs_helpers::get_opt_arg<int>(env, args, Model).value_or(5) };
        const auto factorNum{ avs_helpers::get_opt_arg<int>(env, args, Factor_num).value_or(2) };
        const auto factorDen{ avs_helpers::get_opt_arg<int>(env, args, Factor_den).value_or(1) };
        const auto fpsNum{ avs_helpers::get_opt_arg<int>(env, args, Fps_num) };
        if (fpsNum && *fpsNum < 1)
            throw "fps_num must be at least 1";

        const auto fpsDen{ avs_helpers::get_opt_arg<int>(env, args, Fps_den) };
        if (fpsDen && *fpsDen < 1)
            throw "fps_den must be at least 1";

        std::string modelPath{ avs_helpers::get_opt_arg<std::string>(env, args, Model_path).value_or("") };

        const auto gpuId{ avs_helpers::get_opt_arg<int>(env, args, Gpu_id).value_or(ncnn::get_default_gpu_index()) };
        const auto gpuThread{ avs_helpers::get_opt_arg<int>(env, args, Gpu_thread).value_or(2) };

        const auto tta{ avs_helpers::get_opt_arg<bool>(env, args, Tta).value_or(0) };
        const auto uhd{ avs_helpers::get_opt_arg<bool>(env, args, Uhd).value_or(0) };

        const int sceneChange{ avs_helpers::get_opt_arg<bool>(env, args, Sc).value_or(0) };
        const int sceneChange1{ avs_helpers::get_opt_arg<bool>(env, args, Sc1).value_or(0) };
        d->sc_threshold = avs_helpers::get_opt_arg<float>(env, args, Sc_threshold).value_or(0.12);

        const int skip{ avs_helpers::get_opt_arg<bool>(env, args, Skip).value_or(0) };
        d->skipThreshold = avs_helpers::get_opt_arg<float>(env, args, Skip_threshold).value_or(60.0);

        d->tr = avs_helpers::get_opt_arg<int>(env, args, Denoise_tr).value_or(1);

        const auto matrix_in{ avs_helpers::get_opt_arg<int>(env, args, Matrinx_in) };
        const bool is_rgb{ static_cast<bool>(avs_is_rgb(&vi)) };
        const int full_range{
            avs_helpers::get_opt_arg<bool>(env, args, Full_range).value_or(g_avs_api->avs_component_size(&vi) == 4 || is_rgb) };

        if (model < 0 || model > models_num.size() - 1)
            throw std::format("model must be between 0 and {} (inclusive)", models_num.size() - 1);
        if (factorNum < 1)
            throw "factor_num must be at least 1";
        if (factorDen < 1)
            throw "factor_den must be at least 1";
        if (fpsNum && fpsDen && !(vi.fps_numerator && vi.fps_denominator))
            throw "clip does not have a valid frame rate and hence fps_num and fps_den cannot be used";
        if (gpuId < 0 || gpuId >= ncnn::get_gpu_count())
            throw "invalid GPU device";
        if (auto queueCount{ ncnn::get_gpu_info(gpuId).compute_queue_count() }; gpuThread < 1 ||
            static_cast<uint32_t>(gpuThread) > queueCount)
            throw std::format("gpu_thread must be between 1 and {} (inclusive)", queueCount);

        {
            std::lock_guard lock(g_global_mutex);
            if (!g_global_semaphore) {
                g_global_semaphore = std::make_unique<std::counting_semaphore<1024>>(gpuThread);
            }
        }

        if (sceneChange && sceneChange1)
            throw ("both sc and sc1 cannot be  true in the same time");
        if (d->sc_threshold < 0 || d->sc_threshold > 1)
            throw "sc_threshold must be between 0.0 and 1.0 (inclusive)";
        if (d->skipThreshold < 0 || d->skipThreshold > 60)
            throw "skip_threshold must be between 0.0 and 60.0 (inclusive)";
        if (d->tr < 1)
            throw "denoise_tr must be greater than or equal to 1.";
        if (!is_rgb && !matrix_in)
            throw "matrix_in must be specified for YUV formats.";
        if (matrix_in && (*matrix_in < 0 || *matrix_in > 2))
            throw "matrix_in must be between 0 and 2.";

        d->planes = is_rgb ? decltype(d->planes){AVS_PLANAR_R, AVS_PLANAR_G, AVS_PLANAR_B}
        : decltype(d->planes){AVS_PLANAR_Y, AVS_PLANAR_U, AVS_PLANAR_V};

        if (fpsNum && fpsDen)
        {
            auto fps_n{ static_cast<unsigned>(*fpsNum) };
            auto fps_d{ static_cast<unsigned>(*fpsDen) };

            muldivRational(&fps_n, &fps_d, vi.fps_denominator, vi.fps_numerator);
            d->factorNum = fps_n;
            d->factorDen = fps_d;
        }
        else
        {
            d->factorNum = factorNum;
            d->factorDen = factorDen;
        }

        if (!denoise)
            muldivRational(&vi.fps_numerator, &vi.fps_denominator, d->factorNum, d->factorDen);

        if (vi.num_frames < 2)
            throw "clip's number of frames must be at least 2";
        if (vi.num_frames / d->factorDen > INT_MAX / d->factorNum)
            throw "resulting clip is too long";

        d->oldNumFrames = vi.num_frames;

        if (!denoise)
            vi.num_frames = static_cast<int>(vi.num_frames * d->factorNum / d->factorDen);

        d->factor = d->factorNum / d->factorDen;

        if (modelPath.empty())
            modelPath = (get_current_module_path().parent_path() / "models" / (map_models.at(model)).first).generic_string();

        std::ifstream ifs{ modelPath + "/flownet.param" };
        if (!ifs.is_open())
            throw "failed to load model";
        ifs.close();

        const bool rife_v2{ (modelPath.find("rife-v2") != std::string::npos) || (modelPath.find("rife-v3") != std::string::npos) };
        const bool rife_v4{ (modelPath.find("rife-v4") != std::string::npos) || (modelPath.find("rife4") != std::string::npos) };

        if (modelPath.find("rife") == std::string::npos)
            throw "unknown model dir type";

        const int padding{ (map_models.at(model)).second };

        if (!rife_v4 && (d->factorNum != 2 || d->factorDen != 1))
            throw "only rife-v4 model supports custom frame rate";

        if (rife_v4 && tta)
            throw "rife-v4 model does not support TTA mode";

        const int chroma_subsampling{ !is_rgb ? !g_avs_api->avs_is_420(&vi) ? g_avs_api->avs_is_422(&vi) ? 2 : 0 : 1 : 0 };

        ModelKey key{ modelPath, gpuId, tta, uhd, rife_v2, rife_v4, padding };
        {
            std::lock_guard lock(g_global_mutex);
            auto& weak_ref{ g_model_cache[key] };
            d->rife = weak_ref.lock();
            if (!d->rife) {
                d->rife = std::make_shared<RIFE>(gpuId, tta, uhd, 1, rife_v2, rife_v4, padding, !is_rgb, chroma_subsampling,
                    matrix_in ? *matrix_in : 1, g_avs_api->avs_component_size(&vi), full_range, g_avs_api->avs_bits_per_component(&vi));
                d->rife->load(modelPath);
                weak_ref = d->rife;
            }
        }

        if (sceneChange)
        {
            if (skip)
                fi->get_frame = (denoise) ? RIFE_get_frame<true, false, true, true> : RIFE_get_frame<true, false, true, false>;
            else
                fi->get_frame = (denoise) ? RIFE_get_frame<true, false, false, true> : RIFE_get_frame<true, false, false, false>;
        }
        else
        {
            if (sceneChange1)
            {
                if (skip)
                    fi->get_frame = (denoise) ? RIFE_get_frame<false, true, true, true> : RIFE_get_frame<false, true, true, false>;
                else
                    fi->get_frame = (denoise) ? RIFE_get_frame<false, true, false, true> : RIFE_get_frame<false, true, false, false>;
            }
            else
            {
                if (skip)
                    fi->get_frame = (denoise) ? RIFE_get_frame<false, false, true, true> : RIFE_get_frame<false, false, true, false>;
                else
                    fi->get_frame = (denoise) ? RIFE_get_frame<false, false, false, true> : RIFE_get_frame<false, false, false, false>;
            }
        }

        d->src_comp_size = g_avs_api->avs_component_size(&vi);
        vi.pixel_type = AVS_CS_RGBPS;

        g_avs_api->avs_set_to_clip(&v, clip.get());
    }
    catch (std::string& error)
    {
        std::string msg{ "RIFE: " + error };
        v = avs_new_value_error(g_avs_api->avs_save_string(env, msg.c_str(), msg.size()));
    }
    catch (const char* error)
    {
        std::string msg{ std::format("RIFE: {}", error) };
        v = avs_new_value_error(g_avs_api->avs_save_string(env, msg.c_str(), msg.size()));
    }

    fi->user_data = reinterpret_cast<void*>(d.release());
    fi->set_cache_hints = RIFE_set_cache_hints;
    fi->free_filter = free_RIFE;

    return v;
}

const char* AVSC_CC avisynth_c_plugin_init(AVS_ScriptEnvironment* env)
{
    static constexpr int REQUIRED_INTERFACE_VERSION{ 9 };
    static constexpr int REQUIRED_BUGFIX_VERSION{ 2 };
    static constexpr std::string_view required_functions_storage[]
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
    static constexpr std::span<const std::string_view> required_functions{ required_functions_storage };

    if (!avisynth_c_api_loader::get_api(env, REQUIRED_INTERFACE_VERSION, REQUIRED_BUGFIX_VERSION, required_functions))
    {
        std::cerr << avisynth_c_api_loader::get_last_error() << std::endl;
        return avisynth_c_api_loader::get_last_error();
    }

    g_avs_api->avs_add_function(env, "RIFE",
        "c"
        "[model]i"
        "[factor_num]i"
        "[factor_den]i"
        "[fps_num]i"
        "[fps_den]i"
        "[model_path]s"
        "[gpu_id]i"
        "[gpu_thread]i"
        "[tta]b"
        "[uhd]b"
        "[sc]b"
        "[sc1]b"
        "[sc_threshold]f"
        "[skip]b"
        "[skip_threshold]f"
        "[list_gpu]b"
        "[denoise]b"
        "[denoise_tr]i"
        "[matrix_in]i"
        "[full_range]b",
        Create_RIFE, 0);
    return "Real-Time Intermediate Flow Estimation for Video Frame Interpolation";
}
