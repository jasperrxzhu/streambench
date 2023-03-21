#ifndef TILT_BENCH_INCLUDE_BDOPT_TILT_STDDEV_H_
#define TILT_BENCH_INCLUDE_BDOPT_TILT_STDDEV_H_

#include "tilt/builder/tilder.h"
#include "bdopt_tilt_base.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

class BDOptStdDevBench : public Benchmark {
public:
    BDOptStdDevBench(dur_t period, int64_t size, int64_t w) :
        period(period), size(size), w(w)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in",
                           tilt::Type(types::BASEDELTA<int64_t, uint8_t, 64>(), _iter(0, -1)));
        return _WindowBDOptStdDev(in_sym, w);
    }

    void init() final
    {
        in_reg = create_cmp_reg<int64_t, int8_t>(size, 64);
        float osize = (float)size / (float)w;
        out_reg = create_reg<float>(ceil(osize));

        SynthAllCmpBDData<int64_t, int8_t> dataset(period, size, 64);
        dataset.fill(&in_reg);
#ifdef _PRINT_REGION_
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdoptstddev_in_reg.txt");
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
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdoptstddev_in_reg.txt");
        print_reg<float>(&out_reg, "bdoptstddev_out_reg.txt");
#endif
        release_cmp_reg(&in_reg);
        release_reg(&out_reg);
    }

    cmp_region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
    int64_t w;
};

class ParallelBDOptStdDevBench : public ParallelBenchmark {
public:
    ParallelBDOptStdDevBench(int threads, dur_t period, int64_t size, int64_t w)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new BDOptStdDevBench(period, size, w));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_BDOPT_TILT_STDDEV_H_
