#ifndef TILT_BENCH_INCLUDE_TILT_VAR_H_
#define TILT_BENCH_INCLUDE_TILT_VAR_H_

#include "tilt/builder/tilder.h"
#include "tilt_base.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

// INT64 Multiple-Pass Benchmark

Op _Var64(_sym in, int64_t window)
{
    auto inwin = in[_win(-window, 0)];
    auto inwin_sym = _sym("inwin", inwin);

    // avg state
    auto avg_state = _Average64(inwin_sym, [](Expr e) { return e; });
    auto avg_state_sym = _sym("avg_state", avg_state);

    // avg value
    auto avg = _div(_get(avg_state_sym, 0), _get(avg_state_sym, 1));
    auto avg_sym = _sym("avg", avg);

    // avg join
    auto avg_op = _Select(inwin_sym, avg_sym, [](_sym e, _sym avg) { return e - avg; });
    auto avg_op_sym = _sym("avgop", avg_op);

    // stddev state
    auto var_state = _Average64(avg_op_sym, [](Expr e) { return _mul(e, e); });
    auto var_state_sym = _sym("var_state", var_state);

    // var value
    auto var = _div(_get(var_state_sym, 0), _get(var_state_sym, 1));
    auto var_sym = _sym("var", var);

    // query operation
    auto query_op = _op(
        _iter(0, window),
        Params{ in },
        SymTable{
            {inwin_sym, inwin},
            {avg_state_sym, avg_state},
            {avg_sym, avg},
            {avg_op_sym, avg_op},
            {var_state_sym, var_state},
            {var_sym, var}
        },
        _true(),
        var_sym);

    return query_op;
}

class Var64Bench : public Benchmark {
public:
    Var64Bench(dur_t period, int64_t window, int64_t size) :
        period(period), window(window), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT64, _iter(0, -1)));
        return _Var64(in_sym, window);
    }

    void init() final
    {
        in_reg = create_reg<int64_t>(size);
        float osize = (float)size / (float)window;
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
        print_reg<int64_t>(&in_reg, "var64_in_reg.txt");
        print_reg<int64_t>(&out_reg, "var64_out_reg.txt");
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

class ParallelVar64Bench : public ParallelBenchmark {
public:
    ParallelVar64Bench(int threads, dur_t period, int64_t window, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new Var64Bench(period, window, size));
        }
    }
};

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
        print_reg<int64_t>(&in_reg, "var64onepass_in_reg.txt");
        print_reg<int64_t>(&out_reg, "var64onepass_out_reg.txt");
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
