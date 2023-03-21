#ifndef TILT_BENCH_INCLUDE_TILT_WHERE_BD_H_
#define TILT_BENCH_INCLUDE_TILT_WHERE_BD_H_

#include <numeric>

#include "tilt/builder/tilder.h"
#include "tilt_bench.h"
#include "tilt_base.h"
#include "tilt_where.h"

using namespace tilt;
using namespace tilt::tilder;

class BDWhereBench : public Benchmark
{
public:
    BDWhereBench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in",
                           tilt::Type(types::BASEDELTA<int64_t, uint8_t, 64>(), _iter(0, -1)));
        return _Where(in_sym, [](_sym in) { return _gt(in, _i64(0)); });
    }

    void init() final
    {
        in_reg = create_cmp_reg<int64_t, int8_t>(size, 64);
        out_reg = create_reg<int64_t>(size);

        SynthAllCmpBDData<int64_t, int8_t> dataset(period, size, 64);
        dataset.fill(&in_reg);
#ifdef _PRINT_REGION_
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdwhere_in_reg.txt");
#endif
    }

    void execute(intptr_t addr) final
    {
        auto query = (region_t* (*)(ts_t, ts_t, region_t*, cmp_region_t*)) addr;
        query(0, period * size, &out_reg, &in_reg);
    }

    void release() final
    {
#ifdef _PRINT_REGION_
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdwhere_in_reg.txt");
        print_reg<int64_t>(&out_reg, "bdwhere_out_reg.txt");
#endif
        release_cmp_reg(&in_reg);
        release_reg(&out_reg);
    }

    cmp_region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelBDWhereBench : public ParallelBenchmark {
public:
    ParallelBDWhereBench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new BDWhereBench(period, size));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_TILT_WHERE_BD_H_