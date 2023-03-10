#ifndef TILT_BENCH_INCLUDE_TILT_SUM_WHERE_H_
#define TILT_BENCH_INCLUDE_TILT_SUM_WHERE_H_

#include "tilt/builder/tilder.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

Expr _SumWhere64(_sym win, function<Expr(Expr)> filter)
{
    auto acc = [filter](Expr s, Expr st, Expr et, Expr d) {
        return _ifelse(filter(d), _add(s, d), s);
    };
    return _red(win, _i64(0), acc);
}

Op _WindowSumWhere64(_sym in, int64_t w, function<Expr(Expr)> filter)
{
    auto window = in[_win(-w, 0)];
    auto window_sym = _sym("win", window);

    auto sumwhere = _SumWhere64(window_sym, filter);
    auto sumwhere_sym = _sym("sumwhere", sumwhere);

    auto sumwhere_op = _op(
        _iter(0, w),
        Params{in},
        SymTable{
            {window_sym, window},
            {sumwhere_sym, sumwhere}
        },
        _true(),
        sumwhere_sym
    );
    return sumwhere_op;
}

class SumWhere64Bench : public Benchmark {
public:
    SumWhere64Bench(dur_t period, int64_t size, int64_t w) :
        period(period), size(size), w(w)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT64, _iter(0, -1)));
        return _WindowSumWhere64(in_sym, w, [](Expr e) { return _gt(e, _i64(0)); });
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
        print_reg<int64_t>(&in_reg, "sumwhere64_in_reg.txt");
        print_reg<int64_t>(&out_reg, "sumwhere64_out_reg.txt");
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

class ParallelSumWhere64Bench : public ParallelBenchmark {
public:
    ParallelSumWhere64Bench(int threads, dur_t period, int64_t size, int64_t w)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new SumWhere64Bench(period, size, w));
        }
    }
};

/* benchmark for int8 payloads */

Expr _SumWhere8(_sym win, function<Expr(Expr)> filter)
{
    auto acc = [filter](Expr s, Expr st, Expr et, Expr d) {
        return _ifelse(filter(d), _add(s, d), s);
    };
    return _red(win, _i8(0), acc);
}

Op _WindowSumWhere8(_sym in, int64_t w, function<Expr(Expr)> filter)
{
    auto window = in[_win(-w, 0)];
    auto window_sym = _sym("win", window);

    auto sumwhere = _SumWhere8(window_sym, filter);
    auto sumwhere_sym = _sym("sumwhere", sumwhere);

    auto sumwhere_op = _op(
        _iter(0, w),
        Params{in},
        SymTable{
            {window_sym, window},
            {sumwhere_sym, sumwhere}
        },
        _true(),
        sumwhere_sym
    );
    return sumwhere_op;
}

class SumWhere8Bench : public Benchmark {
public:
    SumWhere8Bench(dur_t period, int64_t size, int64_t w) :
        period(period), size(size), w(w)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT8, _iter(0, -1)));
        return _WindowSumWhere8(in_sym, w, [](Expr e) { return _gt(e, _i8(0)); });
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
        print_reg<int8_t>(&in_reg, "sumwhere8_in_reg.txt");
        print_reg<int8_t>(&out_reg, "sumwhere8_out_reg.txt");
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

class ParallelSumWhere8Bench : public ParallelBenchmark {
public:
    ParallelSumWhere8Bench(int threads, dur_t period, int64_t size, int64_t w)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new SumWhere8Bench(period, size, w));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_TILT_SUM_WHERE_H_
