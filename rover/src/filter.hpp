#pragma once
#include <vector>
#include <cstdint>

namespace dsp
{

class averaging_filter
{
public:
    averaging_filter(uint32_t size_);
    double operator()(double);
    double last(int);
    void reset(uint32_t);
    bool is_initialized();
    uint32_t size();
private:
    std::vector<double> samples;
    std::vector<double> output;
    uint32_t samples_idx;
    uint32_t size_;
};

} // namespace dsp
