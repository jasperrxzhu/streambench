#ifndef TILT_BENCH_INCLUDE_TILT_AGGREGATE_H_
#define TILT_BENCH_INCLUDE_TILT_AGGREGATE_H_

#include "tilt/builder/tilder.h"
#include "tilt_base.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

class AggregateBench : public Benchmark {
public:
    AggregateBench(dur_t period, int64_t size, int64_t w) :
        period(period), size(size), w(w)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::FLOAT32, _iter(0, -1)));
        return _WindowSum(in_sym, w);
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
        print_reg<float>(&in_reg, "agg_in_reg.txt");
        print_reg<float>(&out_reg, "agg_out_reg.txt");
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

class ParallelAggregateBench : public ParallelBenchmark {
public:
    ParallelAggregateBench(int threads, dur_t period, int64_t size, int64_t w)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new AggregateBench(period, size, w));
        }
    }
};

// added additional benchmarks for ease of testing

class Sum64Bench : public Benchmark {
public:
    Sum64Bench(dur_t period, int64_t size, int64_t w) :
        period(period), size(size), w(w)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT64, _iter(0, -1)));
        return _WindowSum64(in_sym, w);
    }

    void init() final
    {
        in_reg = create_reg<int64_t>(size);
        float osize = (float)size / (float)w;
        out_reg = create_reg<int64_t>(ceil(osize));

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
        print_reg<int64_t>(&in_reg, "sum64_in_reg.txt");
        print_reg<int64_t>(&out_reg, "sum64_out_reg.txt");
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

class ParallelSum64Bench : public ParallelBenchmark {
public:
    ParallelSum64Bench(int threads, dur_t period, int64_t size, int64_t w)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new Sum64Bench(period, size, w));
        }
    }
};

class Sum8Bench : public Benchmark {
public:
    Sum8Bench(dur_t period, int64_t size, int64_t w) :
        period(period), size(size), w(w)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT8, _iter(0, -1)));
        return _WindowSum8(in_sym, w);
    }

    void init() final
    {
        in_reg = create_reg<int8_t>(size);
        float osize = (float)size / (float)w;
        out_reg = create_reg<int8_t>(ceil(osize));

        SynthData<int8_t> dataset(period, size);
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
        print_reg<int8_t>(&in_reg, "sum8_in_reg.txt");
        print_reg<int8_t>(&out_reg, "sum8_out_reg.txt");
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

class ParallelSum8Bench : public ParallelBenchmark {
public:
    ParallelSum8Bench(int threads, dur_t period, int64_t size, int64_t w)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new Sum8Bench(period, size, w));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_TILT_AGGREGATE_H_
