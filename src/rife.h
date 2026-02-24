#pragma once

// rife implemented with ncnn library

#include <string>

// ncnn
#include "net.h"

class RIFE
{
public:
    RIFE(int gpuid, bool tta_mode, bool uhd_mode, int num_threads, bool rife_v2, bool rife_v4, int padding, bool is_yuv,
        int chroma_subsampling, int matrix_in, int bytes_per_comp, bool full_range, int bit_depth);
    ~RIFE();

    int load(const std::string& modeldir);

    int process(const uint8_t* const src0_p[3], const uint8_t* const src1_p[3], float* dstR, float* dstG, float* dstB, const int w,
        const int h, const ptrdiff_t stride0[3], const ptrdiff_t stride1[3], const ptrdiff_t dst_stride, const float timestep) const;

    int process_v4(const uint8_t* const src0_p[3], const uint8_t* const src1_p[3], float* dstR, float* dstG, float* dstB, const int w,
        const int h, const ptrdiff_t stride0[3], const ptrdiff_t stride1[3], const ptrdiff_t dst_stride, const float timestep) const;

    int process_copy(const uint8_t* const src_p[3], float* dstR, float* dstG, float* dstB,
        const int w, const int h, const ptrdiff_t stride[3], const ptrdiff_t dst_stride) const;

    bool is_yuv;
    int chroma_subsampling; // 0=4:4:4, 1=4:2:0, 2=4:2:2
    int matrix_in; // 0=601, 1=709, 2=2020
    int bytes_per_comp; // 1=8b, 2=16b, 4=32f
    bool full_range; // 0=limited, 1=full
    int bit_depth;

private:
    ncnn::VulkanDevice* vkdev;
    ncnn::Net flownet;
    ncnn::Net contextnet;
    ncnn::Net fusionnet;
    ncnn::Pipeline* rife_preproc;
    ncnn::Pipeline* rife_postproc;
    ncnn::Pipeline* rife_flow_tta_avg;
    ncnn::Pipeline* rife_flow_tta_temporal_avg;
    ncnn::Pipeline* rife_out_tta_temporal_avg;
    ncnn::Pipeline* rife_v4_timestep;
    ncnn::Layer* rife_uhd_downscale_image;
    ncnn::Layer* rife_uhd_upscale_flow;
    ncnn::Layer* rife_uhd_double_flow;
    ncnn::Layer* rife_v2_slice_flow;
    bool tta_mode;
    bool tta_temporal_mode;
    bool uhd_mode;
    int num_threads;
    bool rife_v2;
    bool rife_v4;
    int padding;
};
