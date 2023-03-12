#ifndef TILT_BENCH_INCLUDE_BDOPT_TILT_BASE_H_
#define TILT_BENCH_INCLUDE_BDOPT_TILT_BASE_H_

#include "tilt/builder/tilder.h"

using namespace tilt;
using namespace tilt::tilder;

/* Sum */

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

/* Avg */

Expr _BDOptAvg(_sym win)
{
    auto basic_acc = [](Expr s, Expr st, Expr et, Expr d) {
        auto sum = _get(s, 0);
        auto count = _get(s, 1);
        return _new(vector<Expr>{_add(sum, d), _add(count, _i64(1))});
    };
    return _block_red(win, _new(vector<Expr>{_i64(0), _i64(0)}), "vec_block_avg", basic_acc);
}

Op _WindowBDOptAvg(_sym in, int64_t w, int64_t p)
{
    auto window = in[_win(-w, 0)];
    auto window_sym = _sym("win", window);
    auto avg_state = _BDOptAvg(window_sym);
    auto avg_state_sym = _sym("avg_state", avg_state);

    auto avg = _div(_get(avg_state_sym, 0), _get(avg_state_sym, 1));
    auto avg_sym = _sym("avg", avg);

    auto wc_op = _op(
        _iter(0, p),
        Params{ in },
        SymTable{
            {window_sym, window},
            {avg_state_sym, avg_state},
            {avg_sym, avg}
        },
        _true(),
        avg_sym);
    return wc_op;
}

Op _WindowBDOptAvg(_sym in, int64_t w)
{
    return _WindowBDOptAvg(in, w, w);
}

/* StdDev */

Expr _BDOptStdDev(_sym win)
{
    auto basic_acc = [](Expr s, Expr st, Expr et, Expr d) {
        auto sum_sq = _get(s, 0);
        auto sum = _get(s, 1);
        auto count = _get(s, 2);
        return _new(vector<Expr>{ _add(sum_sq, _mul(d, d)),
                                  _add(sum, d),
                                  _add(count, _i64(1))});
    };
    return _block_red(win, _new(vector<Expr>{_i64(0), _i64(0), _i64(0)}),
                      "vec_block_stddev", basic_acc);
}

Op _WindowBDOptStdDev(_sym in, int64_t w, int64_t p)
{
    auto window = in[_win(-w, 0)];
    auto window_sym = _sym("win", window);
    auto stddev_state = _BDOptStdDev(window_sym);
    auto stddev_state_sym = _sym("stddev_state", stddev_state);

    // stddev = (sum_sq - (sum * sum)/n) / n
    /*
    auto stddev = _div(
        _sub(
            _get(stddev_state_sym, 0),
            _div(
                _mul(_get(stddev_state_sym, 1), _get(stddev_state_sym, 1)),
                _get(stddev_state_sym, 2)
            )
        ),
        _get(stddev_state_sym, 2)
    );
    */
    auto stddev = _get(stddev_state_sym, 2);
    auto stddev_sym = _sym("stddev", stddev);

    auto wc_op = _op(
        _iter(0, p),
        Params{ in },
        SymTable{
            {window_sym, window},
            {stddev_state_sym, stddev_state},
            {stddev_sym, stddev}
        },
        _true(),
        stddev_sym);
    return wc_op;
}

Op _WindowBDOptStdDev(_sym in, int64_t w)
{
    return _WindowBDOptStdDev(in, w, w);
}

#endif  // TILT_BENCH_INCLUDE_BDOPT_TILT_BASE_H_
