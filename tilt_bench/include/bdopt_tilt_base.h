#ifndef TILT_BENCH_INCLUDE_BDOPT_TILT_BASE_H_
#define TILT_BENCH_INCLUDE_BDOPT_TILT_BASE_H_

#include "tilt/builder/tilder.h"

using namespace tilt;
using namespace tilt::tilder;

Expr _BDOptSum(_sym win)
{
    auto basic_acc = [](Expr s, Expr st, Expr et, Expr d) { return _add(s, d); };
    return _block_red(win, _i64(0), "vec_block_sum", basic_acc);
}

Op _WindowBDOptSum(_sym in, int64_t w, int64_t p)
{
    auto window = in[_win(-w, 0)];
    auto window_sym = _sym("win", window);
    auto sum = _BDOptSum(window_sym);
    auto sum_sym = _sym("sum", sum);
    auto wc_op = _op(
        _iter(0, p),
        Params{ in },
        SymTable{ {window_sym, window}, {sum_sym, sum} },
        _true(),
        sum_sym);
    return wc_op;
}

Op _WindowBDOptSum(_sym in, int64_t w)
{
    return _WindowBDOptSum(in, w, w);
}

#endif  // TILT_BENCH_INCLUDE_BDOPT_TILT_BASE_H_
