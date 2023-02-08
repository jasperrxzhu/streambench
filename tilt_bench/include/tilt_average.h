#ifndef TILT_BENCH_INCLUDE_TILT_AVERAGE_H_
#define TILT_BENCH_INCLUDE_TILT_AVERAGE_H_

#include "tilt/builder/tilder.h"
#include "tilt_base.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

// Average benchmark that uses two passes over the same window
class AverageBench : public Benchmark {
public:
    AverageBench(dur_t period, int64_t size, int64_t w) :
        period(period), size(size), w(w)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::FLOAT32, _iter(0, -1)));
        return _WindowAvg(in_sym, w);
    }

    void init() final
    {
        in_reg = create_reg<float>(size);
        float osize = (float)size / (float)w;
        out_reg = create_reg<float>(ceil(osize));

        SynthData<float> dataset(period, size);
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
        print_reg<float>(&in_reg, "avg_in_reg.txt");
        print_reg<float>(&out_reg, "avg_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
    int64_t w;
};

class ParallelAverageBench : public ParallelBenchmark {
public:
    ParallelAverageBench(int threads, dur_t period, int64_t size, int64_t w)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new AverageBench(period, size, w));
        }
    }
};

// Average that only requires one pass over a window
class AverageOnePassBench : public Benchmark {
public:
    AverageOnePassBench(dur_t period, int64_t size, int64_t w) :
        period(period), size(size), w(w)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::FLOAT32, _iter(0, -1)));
        return _WindowAvgOnePass(in_sym, w);
    }

    void init() final
    {
        in_reg = create_reg<float>(size);
        float osize = (float)size / (float)w;
        out_reg = create_reg<float>(ceil(osize));

        SynthData<float> dataset(period, size);
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
        print_reg<float>(&in_reg, "avgonepass_in_reg.txt");
        print_reg<float>(&out_reg, "avgonepass_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
    int64_t w;
};

class ParallelAverageOnePassBench : public ParallelBenchmark {
public:
    ParallelAverageOnePassBench(int threads, dur_t period, int64_t size, int64_t w)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new AverageOnePassBench(period, size, w));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_TILT_AGGREGATE_H_
