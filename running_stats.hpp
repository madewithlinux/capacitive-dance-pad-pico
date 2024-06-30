#pragma once

// #ifdef __cplusplus
// extern "C"
// {
// #endif

typedef int16_t value_t;
typedef uint32_t count_t;
typedef uint32_t sum_t;

class running_stats
{
public:
    value_t threshold = 0;
    sum_t sum = 0;
    count_t count_above_threshold = 0;
    count_t count_below_threshold = 0;

    inline void add_value(value_t v)
    {
        sum += v;
        if (v > threshold)
        {
            count_above_threshold++;
        }
        else
        {
            count_below_threshold++;
        }
    }

    inline count_t get_total_count() const
    {
        return count_above_threshold + count_below_threshold;
    }

    inline value_t get_mean() const
    {
        return sum / (count_above_threshold + count_below_threshold);
    }

    inline float get_mean_float() const
    {
        return (float)sum / (float)(count_above_threshold + count_below_threshold);
    }

    inline bool is_above_threshold() const
    {
        return count_above_threshold >= count_below_threshold;
    }

    inline void reset()
    {
        *this = running_stats();
    }
};

// #ifdef __cplusplus
// }
// #endif
