#include "filter.hpp"

#include <numeric>

namespace dsp
{

averaging_filter::averaging_filter(uint32_t size_)
    : samples_idx(0)
    , size_(size_)
{
}

double averaging_filter::operator()(double in)
{
    if (samples.empty())
    {
        samples.resize(size_, in);
        output.resize(size_, in);
    }

    samples[samples_idx] = in;
    output[samples_idx] = std::accumulate(samples.begin(), samples.end(), double(0))/samples.size();
    auto sample0 = output[samples_idx];
    samples_idx = (samples_idx+1) % samples.size();

    return sample0;
}

double averaging_filter::last(int rel_idx)
{
    auto idx = (int(samples_idx) - 1 + rel_idx) % size_;

    if (idx < 0)
    {
        idx += size_;
    }

    return output[idx];
}

bool averaging_filter::is_initialized()
{
    return samples.size();
}

uint32_t averaging_filter::size()
{
    return size_;
}

void averaging_filter::reset(uint32_t size__)
{
    samples.clear();
    output.clear();
    size_ = size__;
}

} // namespace dsp
