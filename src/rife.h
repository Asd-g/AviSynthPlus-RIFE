#pragma once

// rife implemented with ncnn library

#include <string>

// ncnn
#include "net.h"

class RIFE
{
public:
    RIFE(int gpuid, bool tta_mode, bool uhd_mode, int num_threads, bool rife_v2, bool rife_v4, int padding);
    ~RIFE();

    int load(const std::string& modeldir);

    int process(const float* src0R, const float* src0G, const float* src0B,
        const float* src1R, const float* src1G, const float* src1B,
        float* dstR, float* dstG, float* dstB,
        const int w, const int h, const ptrdiff_t src_stride, const ptrdiff_t dst_stride, const float timestep) const;

    int process_v4(const float* src0R, const float* src0G, const float* src0B,
        const float* src1R, const float* src1G, const float* src1B,
        float* dstR, float* dstG, float* dstB,
        const int w, const int h, const ptrdiff_t src_stride, const ptrdiff_t dst_stride, const float timestep) const;

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
