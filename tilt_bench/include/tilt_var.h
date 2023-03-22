#ifndef TILT_BENCH_INCLUDE_TILT_VAR_H_
#define TILT_BENCH_INCLUDE_TILT_VAR_H_

#include "tilt/builder/tilder.h"
#include "tilt_base.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

// INT64 OnePass Benchmark

class Var64OnePassBench : public Benchmark {
public:
    Var64OnePassBench(dur_t period, int64_t window, int64_t size) :
        period(period), window(window), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT64, _iter(0, -1)));
        return _WindowVar64OnePass(in_sym, window);
    }

    void init() final
    {
        in_reg = create_reg<int64_t>(size);
        float osize = (float)size / (float)window;
        out_reg = create_reg<float>(ceil(osize));

        SynthData<int64_t> dataset(period, size);
        dataset.fill(&in_reg);
    }

    void execute(intptr_t addr) final
    {
        auto query = (region_t* (*)(ts_t, ts_t, region_t*, region_t*)) addr;
        query(0, period * size, &out_reg, &in_reg);
    }

    void release() final
    {
#ifdef _PRINT_REGION_
        print_reg<int64_t>(&in_reg, "var64onepass_in_reg.txt");
        print_reg<float>(&out_reg, "var64onepass_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    int64_t window;
    dur_t period;
    int64_t size;
    region_t in_reg;
    region_t out_reg;
};

class ParallelVar64OnePassBench : public ParallelBenchmark {
public:
    ParallelVar64OnePassBench(int threads, dur_t period, int64_t window, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new Var64OnePassBench(period, window, size));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_TILT_VAR_H_