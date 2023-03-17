#ifndef TILT_BENCH_INCLUDE_BDLOOP_TILT_SELECT_H_
#define TILT_BENCH_INCLUDE_BDLOOP_TILT_SELECT_H_

#include "tilt/builder/tilder.h"
#include "tilt_bench.h"
#include "tilt_select.h"

using namespace tilt;
using namespace tilt::tilder;

class BDLoopSelectBench : public Benchmark {
public:
    BDLoopSelectBench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::BASEDELTA<int64_t, int8_t, 64>(), _iter(0, -1)));
        return _Select(in_sym, [](_sym in) { return in + _i64(3); });
    }

    void init() final
    {
        in_reg = create_cmp_reg<int64_t, int8_t>(size, 64);
        out_reg = create_reg<int64_t>(size);

        SynthAllCmpBDData<int64_t, int8_t> dataset(size, 64);
        dataset.fill(&in_reg);
    }

    void execute(intptr_t addr) final
    {
        auto query = (region_t* (*)(ts_t, ts_t, region_t*, region_t*)) addr;
        query(0, period * size, &out_reg, &in_reg);
#ifdef _PRINT_REGION_
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdloopselect_in_reg.txt");
#endif
    }

    void release() final
    {
#ifdef _PRINT_REGION_
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdloopselect_in_reg.txt");
        print_reg<int64_t>(&out_reg, "bdloopselect_out_reg.txt");
#endif
        release_cmp_reg(&in_reg);
        release_reg(&out_reg);
    }

    cmp_region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelBDLoopSelectBench : public ParallelBenchmark {
public:
    ParallelBDLoopSelectBench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new BDLoopSelectBench(period, size));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_TILT_BDLOOP_SELECT_H_
