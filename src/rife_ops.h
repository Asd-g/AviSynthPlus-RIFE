#pragma once

// rife implemented with ncnn library

#include <vector>

// ncnn
#include "ncnn/layer.h"
#include "ncnn/pipeline.h"

class Warp : public ncnn::Layer
{
public:
    Warp();
    virtual int create_pipeline(const ncnn::Option& opt);
    virtual int destroy_pipeline(const ncnn::Option& opt);
    virtual int forward(const std::vector<ncnn::Mat>& bottom_blobs, std::vector<ncnn::Mat>& top_blobs, const ncnn::Option& opt) const;
    virtual int forward(const std::vector<ncnn::VkMat>& bottom_blobs, std::vector<ncnn::VkMat>& top_blobs, ncnn::VkCompute& cmd, const ncnn::Option& opt) const;

private:
    ncnn::Pipeline* pipeline_warp;
    ncnn::Pipeline* pipeline_warp_pack4;
    ncnn::Pipeline* pipeline_warp_pack8;
};
