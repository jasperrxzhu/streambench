#ifndef TILT_BENCH_INCLUDE_TILT_SELECT_H_
#define TILT_BENCH_INCLUDE_TILT_SELECT_H_

#include "tilt/builder/tilder.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

Op _Select(_sym in, function<Expr(_sym)> selector)
{
    auto e = in[_pt(0)];
    auto e_sym = _sym("e", e);
    auto res = selector(e_sym);
    auto res_sym = _sym("res", res);
    auto sel_op = _op(
        _iter(0, 1),
        Params{in},
        SymTable{{e_sym, e}, {res_sym, res}},
        _exists(e_sym),
        res_sym);
    return sel_op;
}

class SelectBench : public Benchmark {
public:
    SelectBench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::FLOAT32, _iter(0, -1)));
        return _Select(in_sym, [](_sym in) { return in + _f32(3); });
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
        print_reg<float>(&in_reg, "select_in_reg.txt");
        print_reg<float>(&out_reg, "select_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelSelectBench : public ParallelBenchmark {
public:
    ParallelSelectBench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new SelectBench(period, size));
        }
    }
};

// added additional benchmarks with different input payload sizes for ease of testing

class Select64Bench : public Benchmark {
public:
    Select64Bench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT64, _iter(0, -1)));
        return _Select(in_sym, [](_sym in) { return in + _i64(3); });
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
        print_reg<int64_t>(&in_reg, "select_in_reg.txt");
        print_reg<int64_t>(&out_reg, "select_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelSelect64Bench : public ParallelBenchmark {
public:
    ParallelSelect64Bench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new Select64Bench(period, size));
        }
    }
};

class Select8Bench : public Benchmark {
public:
    Select8Bench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT8, _iter(0, -1)));
        return _Select(in_sym, [](_sym in) { return in + _i8(3); });
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
        print_reg<int8_t>(&in_reg, "select_in_reg.txt");
        print_reg<int8_t>(&out_reg, "select_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelSelect8Bench : public ParallelBenchmark {
public:
    ParallelSelect8Bench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new Select8Bench(period, size));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_TILT_SELECT_H_
