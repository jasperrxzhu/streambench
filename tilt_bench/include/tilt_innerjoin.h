#ifndef TILT_BENCH_INCLUDE_TILT_INNERJOIN_H_
#define TILT_BENCH_INCLUDE_TILT_INNERJOIN_H_

#include <numeric>

#include "tilt/builder/tilder.h"
#include "tilt_base.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

class InnerJoinBench : public Benchmark {
public:
    InnerJoinBench(dur_t lperiod, dur_t rperiod, int64_t size) :
        lperiod(lperiod), rperiod(rperiod), size(size)
    {}

private:
    Op query() final
    {
        auto left_sym = _sym("left", tilt::Type(types::BASEDELTA<int64_t, uint8_t, 64>(), _iter(0, -1)));
        auto right_sym = _sym("right", tilt::Type(types::BASEDELTA<int64_t, uint8_t, 64>(), _iter(0, -1)));
        return _Join(left_sym, right_sym, [](_sym left, _sym right) { return left + right; });
    }

    void init() final
    {
        left_reg = create_cmp_reg<int64_t, int8_t>(size, 64);
        right_reg = create_cmp_reg<int64_t, int8_t>(size, 64);
        float ratio = (float) max(lperiod, rperiod) / (float) min(lperiod, rperiod);
        int osize = size * ceil(ratio);
        out_reg = create_reg<int64_t>(osize);

        SynthAllCmpBDData<int64_t, int8_t> dataset_left(lperiod, size, 64);
        dataset_left.fill(&left_reg);
        SynthAllCmpBDData<int64_t, int8_t> dataset_right(rperiod, size, 64);
        dataset_right.fill(&right_reg);
    }

    void execute(intptr_t addr) final
    {
        auto query = (region_t* (*)(ts_t, ts_t, region_t*, region_t*, region_t*)) addr;
        query(0, min(lperiod, rperiod) * size, &out_reg, &left_reg, &right_reg);
    }

    void release() final
    {
#ifdef _PRINT_REGION_
        print_cmp_reg<int64_t, int8_t>(&left_reg, 64, "bdjoin_left_reg.txt");
        print_cmp_reg<int64_t, int8_t>(&right_reg, 64, "bdjoin_right_reg.txt");
        print_reg<int64_t>(&out_reg, "bdjoin_out_reg.txt");
#endif
        release_cmp_reg(&left_reg);
        release_cmp_reg(&right_reg);
        release_reg(&out_reg);
    }

    cmp_region_t left_reg;
    cmp_region_t right_reg;
    region_t out_reg;

    int64_t size;
    dur_t lperiod;
    dur_t rperiod;
};

#endif  // TILT_BENCH_INCLUDE_TILT_INNERJOIN_H_
