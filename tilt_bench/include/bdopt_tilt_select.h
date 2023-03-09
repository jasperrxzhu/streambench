#ifndef TILT_BENCH_INCLUDE_BDOPT_TILT_SELECT_H_
#define TILT_BENCH_INCLUDE_BDOPT_TILT_SELECT_H_

#include "tilt/builder/tilder.h"
#include "bdopt_tilt_base.h"
#include "tilt_bench.h"
#include "tilt_select.h"

using namespace tilt;
using namespace tilt::tilder;

Op _BDOptSelectv01(_sym in, int64_t w, function<Expr(_sym)> selector)
{
    auto window = in[_win(-w, 0)];
    auto window_sym = _sym("win", window);
    auto basic_select = _Select(window_sym, selector);
    auto block_select = _block_op(basic_select, "vec_block_select");
    auto block_select_sym = _sym("select", block_select);
    auto sel_op = _op(
        _iter(0, w),
        Params{ in },
        SymTable{ {window_sym, window},
                  {block_select_sym, block_select} },
        _true(),
        block_select_sym
    );
    return sel_op;
}

class BDOptSelectBench : public Benchmark {
public:
    BDOptSelectBench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in",
                           tilt::Type(types::BASEDELTA<int64_t, uint8_t, 64>(), _iter(0, -1)));
        return _BDOptSelectv01(in_sym, period * size,
                               [](_sym in) { return in + _i64(3); });
    }

    void init() final
    {
        in_reg = create_cmp_reg<int64_t, int8_t>(size, 64);
        out_reg = create_reg<int64_t>(size);

        SynthAllCmpBDData<int64_t, int8_t> dataset(period, size, 64);
        dataset.fill(&in_reg);
#ifdef _PRINT_REGION_
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdoptselect_in_reg.txt");
#endif
    }

    void execute(intptr_t addr) final
    {
        auto query = (region_t* (*)(ts_t, ts_t, region_t*, region_t*)) addr;
        query(0, period * size, &out_reg, &in_reg);
    }

    void release() final
    {
#ifdef _PRINT_REGION_
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdoptselect_in_reg.txt");
        print_reg<int64_t>(&out_reg, "bdoptselect_out_reg.txt");
#endif
        release_cmp_reg(&in_reg);
        release_reg(&out_reg);
    }

    cmp_region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelBDOptSelectBench : public ParallelBenchmark {
public:
    ParallelBDOptSelectBench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new BDOptSelectBench(period, size));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_BDOPT_TILT_SELECT_H_
