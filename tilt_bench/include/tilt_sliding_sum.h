#ifndef TILT_BENCH_INCLUDE_TILT_SLIDING_SUM_H_
#define TILT_BENCH_INCLUDE_TILT_SLIDING_SUM_H_

#include "tilt/builder/tilder.h"
#include "tilt_base.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

/* classes and operators relating to the naive sliding sum implementation */

class NaiveSlidingSumBench : public Benchmark {
public:
    NaiveSlidingSumBench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT64, _iter(0, -1)));
        return _WindowSum(in_sym, 100, 50);
    }

    void init() final
    {
        in_reg = create_reg<int64_t>(size);
        float osize = (float)size / 50;
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
        print_reg<int64_t>(&in_reg, "naivesum_in_reg.txt");
        print_reg<int64_t>(&out_reg, "naivesum_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelNaiveSlidingSumBench : public ParallelBenchmark {
public:
    ParallelNaiveSlidingSumBench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new NaiveSlidingSumBench(period, size));
        }
    }
};

/* classes and operators relating to the incremental sliding sum implementation */

Op _IncrementSums(_sym partial_sums)
{
    auto out = _out(types::INT64);

    auto o = out[_pt(-50)];
    auto o_sym = _sym("o", o);
    auto state = _ifelse(_exists(o_sym), o_sym, _i64(0));
    auto state_sym = _sym("state", state);

    auto head = partial_sums[_pt(0)];
    auto head_sym = _sym("head", head);
    auto tail = partial_sums[_pt(-100)];
    auto tail_sym = _sym("tail", tail);

    auto new_sum = state_sym
        + _ifelse(_exists(head_sym), head_sym, _i64(0))
        - _ifelse(_exists(tail_sym), tail_sym, _i64(0));
    auto new_sum_sym = _sym("new_sum", new_sum);

    auto isums_op = _op(
        _iter(0, 50),
        Params{partial_sums},
        SymTable{
            {o_sym, o},
            {state_sym, state},
            {head_sym, head},
            {tail_sym, tail},
            {new_sum_sym, new_sum}
        },
        _true(),
        new_sum_sym
    );
    return isums_op;
}

Op _SlidingSums(_sym in, dur_t period, int64_t size)
{
    auto win = in[_win(-(period * size), 0)];
    auto win_sym = _sym("win", win);
    auto partial_sums = _WindowSum(win_sym, 50);
    auto partial_sums_sym = _sym("partial_sums", partial_sums);

    auto res = _IncrementSums(partial_sums_sym);
    auto res_sym = _sym("res", res);

    auto ssums_op = _op(
        _iter(0, period * size),
        Params{in},
        SymTable{
            {win_sym, win},
            {partial_sums_sym, partial_sums},
            {res_sym, res}
        },
        _true(),
        res_sym
    );
    return ssums_op;
}

class IncSlidingSumBench : public Benchmark {
public:
    IncSlidingSumBench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT64, _iter(0, -1)));
        return _SlidingSums(in_sym, period, size);
    }

    void init() final
    {
        in_reg = create_reg<int64_t>(size);
        float osize = (float)size / 50;
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
        print_reg<int64_t>(&in_reg, "incsum_in_reg.txt");
        print_reg<int64_t>(&out_reg, "incsum_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelIncSlidingSumBench : public ParallelBenchmark {
public:
    ParallelIncSlidingSumBench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new IncSlidingSumBench(period, size));
        }
    }
};
#endif // TILT_BENCH_INCLUDE_TILT_SLIDING_SUM_H_