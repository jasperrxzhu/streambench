#ifndef TILT_BENCH_INCLUDE_BDOPT_TILT_NORM_H_
#define TILT_BENCH_INCLUDE_BDOPT_TILT_NORM_H_

#include "tilt/builder/tilder.h"
#include "bdopt_tilt_base.h"
#include "tilt_base.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

Op _WindowBDOptNormV01(_sym in, int64_t w)
/*****************************************
 * version that does not use a vectorized Select template
*****************************************/
{
    auto win = in[_win(-w, 0)];
    auto win_sym = _sym("win", win);

    auto sd_state = _BDOptStdDev(win_sym);
    auto sd_state_sym = _sym("sd_state", sd_state);

    auto sum_sq = _cast(types::FLOAT32, _get(sd_state_sym, 0));
    auto sum_sq_sym = _sym("sum_sq", sum_sq);
    auto sum = _cast(types::FLOAT32, _get(sd_state_sym, 1));
    auto sum_sym = _sym("sum", sum);
    auto count = _cast(types::FLOAT32, _get(sd_state_sym, 2));
    auto count_sym = _sym("count", count);

    auto avg = _div(sum_sym, count_sym);
    auto avg_sym = _sym("avg", avg);
    auto sd = _sqrt(
        _div(
            _sub(
                sum_sq_sym,
                _div(
                    _mul(sum_sym, sum_sym),
                    count_sym
                )
            ),
            count_sym
        )
    );
    auto sd_sym = _sym("sd", sd);

    auto basic_norm = _Select(win_sym, avg_sym, sd_sym,
                              [](_sym e, _sym avg, _sym sd){
                                    return _div(_sub(_cast(types::FLOAT32, e), avg), sd);
                              });
    auto block_norm = _block_op(basic_norm);
    auto block_norm_sym = _sym("block_norm", block_norm);

    auto query_op = _op(
        _iter(0, w),
        Params{ in },
        SymTable{
            {win_sym, win},
            {sd_state_sym, sd_state},
            {sum_sq_sym, sum_sq},
            {sum_sym, sum},
            {count_sym, count},
            {avg_sym, avg},
            {sd_sym, sd},
            {block_norm_sym, block_norm}
        },
        _true(),
        block_norm_sym
    );
    return query_op;
}

Op _WindowBDOptNormV02(_sym in, int64_t w)
/*****************************************
 * version that uses vectorized Select template
*****************************************/
{
    auto win = in[_win(-w, 0)];
    auto win_sym = _sym("win", win);

    auto sd_state = _BDOptStdDev(win_sym);
    auto sd_state_sym = _sym("sd_state", sd_state);

    auto sum_sq = _cast(types::FLOAT32, _get(sd_state_sym, 0));
    auto sum_sq_sym = _sym("sum_sq", sum_sq);
    auto sum = _cast(types::FLOAT32, _get(sd_state_sym, 1));
    auto sum_sym = _sym("sum", sum);
    auto count = _cast(types::FLOAT32, _get(sd_state_sym, 2));
    auto count_sym = _sym("count", count);

    auto avg = _div(sum_sym, count_sym);
    auto avg_sym = _sym("avg", avg);
    auto sd = _sqrt(
        _div(
            _sub(
                sum_sq_sym,
                _div(
                    _mul(sum_sym, sum_sym),
                    count_sym
                )
            ),
            count_sym
        )
    );
    auto sd_sym = _sym("sd", sd);

    auto basic_norm = _Select(win_sym, avg_sym, sd_sym,
                              [](_sym e, _sym avg, _sym sd){
                                    return _div(_sub(_cast(types::FLOAT32, e), avg), sd);
                              });
    auto block_norm = _block_op(basic_norm, "vec_block_select_norm");
    auto block_norm_sym = _sym("block_norm", block_norm);

    auto query_op = _op(
        _iter(0, w),
        Params{ in },
        SymTable{
            {win_sym, win},
            {sd_state_sym, sd_state},
            {sum_sq_sym, sum_sq},
            {sum_sym, sum},
            {count_sym, count},
            {avg_sym, avg},
            {sd_sym, sd},
            {block_norm_sym, block_norm}
        },
        _true(),
        block_norm_sym
    );
    return query_op;
}

class BDOptNormBench : public Benchmark {
public:
    BDOptNormBench(dur_t period, int64_t size, int64_t w) :
        period(period), size(size), w(w)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in",
                           tilt::Type(types::BASEDELTA<int64_t, uint8_t, 64>(), _iter(0, -1)));
        return _WindowBDOptNormV02(in_sym, w);
    }

    void init() final
    {
        in_reg = create_cmp_reg<int64_t, int8_t>(size, 64);
        out_reg = create_reg<float>(size);

        SynthAllCmpBDData<int64_t, int8_t> dataset(period, size, 64);
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
        print_cmp_reg<int64_t, int8_t>(&in_reg, 64, "bdoptnorm_in_reg.txt");
        print_reg<float>(&out_reg, "bdoptnorm_out_reg.txt");
#endif
        release_cmp_reg(&in_reg);
        release_reg(&out_reg);
    }

    int64_t w;
    dur_t period;
    int64_t size;
    cmp_region_t in_reg;
    region_t out_reg;
};


class ParallelBDOptNormBench : public ParallelBenchmark {
public:
    ParallelBDOptNormBench(int threads, dur_t period, int64_t size, int64_t w)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new BDOptNormBench(period, size, w));
        }
    }
};

#endif // TILT_BENCH_INCLUDE_BDOPT_TILT_NORM_H_