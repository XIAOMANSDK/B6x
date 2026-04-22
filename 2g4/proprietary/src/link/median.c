#include "median.h"

static int median_partition(uint16_t *val, int begin, int end)
{
    uint16_t pivot = val[begin];
    int      idx   = begin;

    for (int i = begin + 1; i < end; i++)
    {
        if (val[i] < pivot)
        {
            val[idx++] = val[i];
            val[i]     = val[idx];
        }
    }

    val[idx] = pivot;

    return idx;
}

static int median_select(uint16_t *val, int begin, int end, int k)
{
    int idx = median_partition(val, begin, end);

    if (idx > k - 1)
    {
        idx = median_select(val, begin, idx, k);
    }
    else if (idx < k - 1)
    {
        idx = median_select(val, idx + 1, end, k);
    }

    return idx;
}

void median_find(uint16_t *val, int n, median_res_t *res)
{
    int idx     = median_select(val, 0, n, (n + 1) >> 1);
    res->median = val[idx];
    res->peak   = val[idx];
    for (int i = idx + 1; i < n; i++)
    {
        if (res->peak < val[i])
        {
            res->peak = val[i];
        }
    }
}

uint16_t median_calc(uint16_t *val, int n)
{
    return val[median_select(val, 0, n, (n + 1) >> 1)];
}
