#ifndef TILT_BENCH_INCLUDE_BDOPT_TILT_SUM_WHERE_H_
#define TILT_BENCH_INCLUDE_BDOPT_TILT_SUM_WHERE_H_

#include "tilt/builder/tilder.h"
#include "bdopt_tilt_base.h"
#include "tilt_base.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

Op _WindowBDOptSumWhere(_sym in, int64_t w, function<Expr(_sym)> filter)
{
    auto state = _sym("state", tilt::Type(types::INT64, _iter(0, -1)));

    auto window = in[_win(-w, 0)];
    auto window_sym = _sym("win", window);

    auto basic_where = _Where(window_sym, filter);
    auto filtered_win = _block_op(basic_where, "vec_block_where");
    auto filtered_win_sym = _sym("fwin", filtered_win);

    auto sum = _Sum64(filtered_win_sym);
    auto sum_sym = _sym("sum", sum);

    auto sumwhere_op = _op(
        _iter(0, w),
        Params{ in, state },
        SymTable{
            {window_sym, window},
            {filtered_win_sym, filtered_win},
            {sum_sym, sum}
        },
        _true(),
        sum_sym,
        Aux{
            {filtered_win_sym, state}
        }
    );
    return sumwhere_op;
}


class BDOptSumWhereBench : public Benchmark {
public:
    BDOptSumWhereBench(dur_t period, int64_t size, int64_t w) :
        period(period), size(size), w(w)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in",
                           tilt::Type(types::BASEDELTA<int64_t, uint8_t, 64>(), _iter(0, -1)));
        return _WindowBDOptSumWhere(in_sym, w, [](_sym in) { return _gt(in, _i64(0)); });
    }

    void init() final
    {
        in_reg = create_cmp_reg<int64_t, int8_t>(size, 64);
        state_reg = create_reg<int64_t>(w);
        float osize = (float)size / (float)w;
        out_reg = create_reg<int64_t>(ceil(osize));

        SynthAllCmpBDData<int64_t, int8_t> dataset(period, size, 64);
        dataset.fill(&in_reg);
#ifdef _PRINT_REGION_
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdoptsumwhere_in_reg.txt");
#endif
    }

    void execute(intptr_t addr) final
    {
        auto query = (region_t* (*)(ts_t, ts_t, region_t*, cmp_region_t*, region_t*)) addr;
        query(0, period * size, &out_reg, &in_reg, &state_reg);
    }

    void release() final
    {
#ifdef _PRINT_REGION_
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdoptsumwhere_in_reg.txt");
        print_reg<int64_t>(&out_reg, "bdoptsumwhere_out_reg.txt");
#endif
        release_cmp_reg(&in_reg);
        release_reg(&out_reg);
    }

    cmp_region_t in_reg;
    region_t state_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
    int64_t w;
};

class ParallelBDOptSumWhereBench : public ParallelBenchmark {
public:
    ParallelBDOptSumWhereBench(int threads, dur_t period, int64_t size, int64_t w)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new BDOptSumWhereBench(period, size, w));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_BDOPT_TILT_SUM_WHERE_H_
