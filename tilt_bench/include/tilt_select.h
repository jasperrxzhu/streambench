#ifndef TILT_BENCH_INCLUDE_TILT_SELECT_H_
#define TILT_BENCH_INCLUDE_TILT_SELECT_H_

#include "tilt/builder/tilder.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

/*** Select Operator ***/

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

/*** Uncompressed Select Benchmark and related classes ***/

class SelectBench : public Benchmark {
public:
    SelectBench(dur_t period, int64_t size) :
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

class ParallelSelectBench : public ParallelBenchmark {
public:
    ParallelSelectBench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new SelectBench(period, size));
        }
    }
};

/*** Base-Delta compressed Select Benchmark and related classes ***/

class BDSelectBench : public Benchmark {
public:
    BDSelectBench(dur_t period, int64_t size, uint32_t block_size) :
        period(period), size(size), block_size(block_size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::BASEDELTA<int64_t, int8_t>(), _iter(0, -1)));
        return _Select(in_sym, [](_sym in) { return in + _i64(3); });
    }

    void init() final
    {
        in_reg = create_bd_reg<int64_t, int8_t>(size, block_size);
        out_reg = create_reg<int64_t>(size);

        SynthBDData<int64_t, int8_t> dataset(period, size);
        dataset.fill(&in_reg);
    }

    void execute(intptr_t addr) final
    {
        auto query = (region_t* (*)(ts_t, ts_t, region_t*, bd_region_t*)) addr;
        query(0, period * size, &out_reg, &in_reg);
    }

    void release() final
    {
#ifdef _PRINT_REGION_
        print_bd_reg<int64_t, int8_t>(&in_reg, "bdselect_in_reg.txt");
        print_reg<int64_t>(&out_reg, "bdselect_out_reg.txt");
#endif
        release_bd_reg(&in_reg);
        release_reg(&out_reg);
    }

    bd_region_t in_reg;
    region_t out_reg;

    dur_t period;
    int64_t size;
    uint32_t block_size;
};

class ParallelBDSelectBench : public ParallelBenchmark {
public:
    ParallelBDSelectBench(int threads, dur_t period, int64_t size, uint32_t block_size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new BDSelectBench(period, size, block_size));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_TILT_SELECT_H_
