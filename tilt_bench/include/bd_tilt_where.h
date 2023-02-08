#ifndef TILT_BENCH_INCLUDE_TILT_WHERE_BD_H_
#define TILT_BENCH_INCLUDE_TILT_WHERE_BD_H_

#include <numeric>

#include "tilt/builder/tilder.h"
#include "tilt_bench.h"
#include "tilt_base.h"
#include "tilt_where.h"

using namespace tilt;
using namespace tilt::tilder;

Op _BDWhere(_sym base, _sym delta, function<Expr(_sym)> filter)
{
    auto e_base = base[_pt(0)];
    auto e_base_sym = _sym("e_base", e_base);
    auto e_delta = delta[_pt(0)];
    auto e_delta_sym = _sym("e_delta", e_delta);

    auto res = e_base_sym + _cast(types::INT64, e_delta_sym);
    auto res_sym = _sym("res", res);

    auto cond = _exists(e_delta_sym) && filter(res_sym);

    auto bdwhere_op = _op(
        _iter(0, 1),
        Params{base, delta},
        SymTable{
            {e_base_sym, e_base},
            {e_delta_sym, e_delta},
            {res_sym, res}
        },
        cond,
        res_sym
    );
    return bdwhere_op;
}

class BDWhereBench : public Benchmark
{
public:
    BDWhereBench(dur_t period, dur_t window, int64_t size) :
        period(period), window(window), size(size)
    {
        ASSERT(period <= window);
    }

private:
    Op query() final
    {
        auto base_sym = _sym("base", tilt::Type(types::INT64, _iter(0, -1)));
        auto delta_sym = _sym("delta", tilt::Type(types::INT8, _iter(0, -1)));
        return _BDWhere(base_sym, delta_sym,
                        [](_sym in) { return _gt(in, _i64(0)); });
    }

    void init() final
    {
        base_reg = create_reg<int64_t>(size/window + 1);
        delta_reg = create_reg<int8_t>(size);
        out_reg = create_reg<int64_t>(size);

        SynthData<int64_t> base_dataset(window, size/window + 1);
        base_dataset.fill(&base_reg);
        SynthData<int8_t> delta_dataset(period, size);
        delta_dataset.fill(&delta_reg);
    }

    void execute(intptr_t addr) final
    {
        auto query = (region_t* (*)(ts_t, ts_t, region_t*, region_t*, region_t*)) addr;
        query(0, period * size, &out_reg, &base_reg, &delta_reg);
    }

    void release() final
    {
#ifdef _PRINT_REGION_
        print_reg<int64_t>(&base_reg, "bdwhere_base_reg.txt");
        print_reg<int8_t>(&delta_reg, "bdwhere_delta_reg.txt");
        print_reg<int64_t>(&out_reg, "bdwhere_out_reg.txt");
#endif
        release_reg(&base_reg);
        release_reg(&delta_reg);
        release_reg(&out_reg);
    }

    region_t base_reg;
    region_t delta_reg;
    region_t out_reg;

    dur_t period; // period for the delta stream
    dur_t window; // period for the base stream
    int64_t size; // total number of elements in stream
};

class ParallelBDWhereBench : public ParallelBenchmark {
public:
    ParallelBDWhereBench(int threads, dur_t period, dur_t window, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new BDWhereBench(period, window, size));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_TILT_WHERE_BD_H_