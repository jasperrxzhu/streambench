#ifndef TILT_BENCH_INCLUDE_TILT_WHERE_H_
#define TILT_BENCH_INCLUDE_TILT_WHERE_H_

#include "tilt/builder/tilder.h"
#include "tilt_bench.h"
#include "tilt_base.h"

#include "iostream"

using namespace tilt;
using namespace tilt::tilder;

class WhereBench : public Benchmark {
public:
    WhereBench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::FLOAT32, _iter(0, -1)));
        return _Where(in_sym, [](_sym in) { return _gt(in, _f32(0)); });
    }

    void init() final
    {
        in_reg = create_reg<float>(size);
        out_reg = create_reg<float>(size);

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
        print_reg<float>(&in_reg, "where_in_reg.txt");
        print_reg<float>(&out_reg, "where_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelWhereBench : public ParallelBenchmark {
public:
    ParallelWhereBench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new WhereBench(period, size));
        }
    }
};

// added additional benchmarks with different input payload sizes for ease of testing

class Where64Bench : public Benchmark {
public:
    Where64Bench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT64, _iter(0, -1)));
        return _Where(in_sym, [](_sym in) { return _gt(in, _i64(0)); });
    }

    void init() final
    {
        in_reg = create_reg<int64_t>(size);
        out_reg = create_reg<int64_t>(size);

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
        print_reg<int64_t>(&in_reg, "where64_in_reg.txt");
        print_reg<int64_t>(&out_reg, "where64_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelWhere64Bench : public ParallelBenchmark {
public:
    ParallelWhere64Bench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new Where64Bench(period, size));
        }
    }
};

class Where8Bench : public Benchmark {
public:
    Where8Bench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT8, _iter(0, -1)));
        return _Where(in_sym, [](_sym in) { return _gt(in, _i8(0)); });
    }

    void init() final
    {
        in_reg = create_reg<int8_t>(size);
        out_reg = create_reg<int8_t>(size);

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
        print_reg<int8_t>(&in_reg, "where8_in_reg.txt");
        print_reg<int8_t>(&out_reg, "where8_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelWhere8Bench : public ParallelBenchmark {
public:
    ParallelWhere8Bench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new Where8Bench(period, size));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_TILT_WHERE_H_
