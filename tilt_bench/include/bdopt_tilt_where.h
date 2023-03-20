#ifndef TILT_BENCH_INCLUDE_BDOPT_TILT_WHERE_H_
#define TILT_BENCH_INCLUDE_BDOPT_TILT_WHERE_H_

#include "tilt/builder/tilder.h"
#include "bdopt_tilt_base.h"
#include "tilt_base.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

Op _BDOptWherev01(_sym in, int64_t w, function<Expr(_sym)> filter)
{
    auto window = in[_win(-w, 0)];
    auto window_sym = _sym("win", window);
    auto basic_where = _Where(window_sym, filter);
    auto block_where = _block_op(basic_where, "vec_block_where");
    auto block_where_sym = _sym("where", block_where);
    auto where_op = _op(
        _iter(0, w),
        Params{ in },
        SymTable{ {window_sym, window},
                  {block_where_sym, block_where} },
        _true(),
        block_where_sym
    );
    return where_op;
}

Op _BDOptWherev02(_sym in, int64_t w, function<Expr(_sym)> filter)
{
    auto window = in[_win(-w, 0)];
    auto window_sym = _sym("win", window);
    auto basic_where = _Where(window_sym, filter);
    auto block_where = _block_op(basic_where);
    auto block_where_sym = _sym("where", block_where);
    auto where_op = _op(
        _iter(0, w),
        Params{ in },
        SymTable{ {window_sym, window},
                  {block_where_sym, block_where} },
        _true(),
        block_where_sym
    );
    return where_op;
}

class BDOptWhereBench : public Benchmark {
public:
    BDOptWhereBench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in",
                           tilt::Type(types::BASEDELTA<int64_t, uint8_t, 64>(), _iter(0, -1)));
        return _BDOptWherev01(in_sym, period * size,
                              [](_sym in) { return _gt(in, _i64(0)); });
    }

    void init() final
    {
        in_reg = create_cmp_reg<int64_t, int8_t>(size, 64);
        out_reg = create_reg<int64_t>(size);

        SynthAllCmpBDData<int64_t, int8_t> dataset(period, size, 64);
        dataset.fill(&in_reg);
#ifdef _PRINT_REGION_
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdoptwhere_in_reg.txt");
#endif
    }

    void execute(intptr_t addr) final
    {
        auto query = (region_t* (*)(ts_t, ts_t, region_t*, cmp_region_t*)) addr;
        query(0, period * size, &out_reg, &in_reg);
    }

    void release() final
    {
#ifdef _PRINT_REGION_
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdoptwhere_in_reg.txt");
        print_reg<int64_t>(&out_reg, "bdoptwhere_out_reg.txt");
#endif
        release_cmp_reg(&in_reg);
        release_reg(&out_reg);
    }

    cmp_region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelBDOptWhereBench : public ParallelBenchmark {
public:
    ParallelBDOptWhereBench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new BDOptWhereBench(period, size));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_BDOPT_TILT_WHERE_H_
