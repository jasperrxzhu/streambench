#include <iostream>
#include <iomanip>
#include <sys/resource.h>

#include "tilt_select.h"
#include "tilt_where.h"
#include "tilt_aggregate.h"
/*
#include "tilt_alterdur.h"
#include "tilt_innerjoin.h"
#include "tilt_outerjoin.h"
#include "tilt_norm.h"
#include "tilt_ma.h"
#include "tilt_rsi.h"
#include "tilt_qty.h"
#include "tilt_impute.h"
#include "tilt_peak.h"
#include "tilt_resample.h"
#include "tilt_kurt.h"
#include "tilt_eg.h"
#include "tilt_yahoo.h"
*/

using namespace std;

int main(int argc, char** argv)
{
    const rlim_t kStackSize = 2 * 1024 * 1024 * 1024;   // min stack size = 2 GB
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0) {
        if (rl.rlim_cur < kStackSize) {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0) {
                cerr << "setrlimit returned result = " << result << endl;
            }
        }
    }

    string testcase = (argc > 1) ? argv[1] : "select";
    int64_t size = (argc > 2) ? atoi(argv[2]) : 100000000;
    int threads = (argc > 3) ? atoi(argv[3]) : 1;
    int64_t period = 1;

    double time = 0;

    if (testcase == "select") {
        ParallelSelectBench bench(threads, period, size);
        time = bench.run();
    } else if (testcase == "select_loopIR") {
        SelectBench bench(period, size);
        bench.print_loopIR("select_loopIR.txt");
    } else if (testcase == "select_llvmIR") {
        SelectBench bench(period, size);
        bench.print_llvmIR("select_llvmIR.txt");
    } else if (testcase == "bdselect") {
        ParallelBDSelectBench bench(threads, period, size, 100);
        time = bench.run();
    } else if (testcase == "bdselect_loopIR") {
        BDSelectBench bench(period, size, 100);
        bench.print_loopIR("bdselect_loopIR.txt");
    } else if (testcase == "bdselect_llvmIR") {
        BDSelectBench bench(period, size, 100);
        bench.print_llvmIR("bdselect_llvmIR.txt");
    } else if (testcase == "where") {
        ParallelWhereBench bench(threads, period, size);
        time = bench.run();
    } else if (testcase == "where_loopIR") {
        WhereBench bench(period, size);
        bench.print_loopIR("where_loopIR.txt");
    } else if (testcase == "where_llvmIR") {
        WhereBench bench(period, size);
        bench.print_llvmIR("where_llvmIR.txt");
    } else if (testcase == "bdwhere") {
        ParallelBDWhereBench bench(threads, period, size, 100);
        time = bench.run();
    } else if (testcase == "bdwhere_loopIR") {
        BDWhereBench bench(period, size, 100);
        bench.print_loopIR("bdwhere_loopIR.txt");
    } else if (testcase == "bdwhere_llvmIR") {
        BDWhereBench bench(period, size, 100);
        bench.print_llvmIR("bdwhere_llvmIR.txt");
    } else if (testcase == "aggregate") {
        ParallelAggregateBench bench(threads, period, size, 1000 * period);
        time = bench.run();
    } else if (testcase == "aggregate_loopIR") {
        AggregateBench bench(period, size, 1000 * period);
        bench.print_loopIR("agg_loopIR.txt");
    } else if (testcase == "aggregate_llvmIR") {
        AggregateBench bench(period, size, 1000 * period);
        bench.print_llvmIR("agg_llvmIR.txt");
    } else if (testcase == "bdagg") {
        ParallelBDAggregateBench bench(threads, period, size, 1000 * period, 100);
        time = bench.run();
    } else if (testcase == "bdagg_loopIR") {
        BDAggregateBench bench(period, size, 1000 * period, 100);
        bench.print_loopIR("bdagg_loopIR.txt");
    } else if (testcase == "bdagg_llvmIR") {
        BDAggregateBench bench(period, size, 1000 * period, 100);
        bench.print_llvmIR("bdagg_llvmIR.txt");
    } /* else if (testcase == "alterdur") {
        AlterDurBench bench(3, 2, size);
        time = bench.run();
    } else if (testcase == "innerjoin") {
        InnerJoinBench bench(period, period, size);
        time = bench.run();
    } else if (testcase == "outerjoin") {
        OuterJoinBench bench(period, period, size);
        time = bench.run();
    } else if (testcase == "normalize") {
        ParallelNormBench bench(threads, period, 10000, size);
        time = bench.run();
    } else if (testcase == "fillmean") {
        ParallelImputeBench bench(threads, period, 10000, size);
        time = bench.run();
    } else if (testcase == "resample") {
        ParallelResampleBench bench(threads, 4, 5, 1000, size);
        time = bench.run();
    } else if (testcase == "algotrading") {
        ParallelMOCABench bench(threads, period, 20, 50, 100, size);
        time = bench.run();
    } else if (testcase == "rsi") {
        ParallelRSIBench bench(threads, period, 14, 100, size);
        time = bench.run();
    } else if (testcase == "largeqty") {
        ParallelLargeQtyBench bench(threads, period, 10, 100, size);
        time = bench.run();
    } else if (testcase == "pantom") {
        ParallelPeakBench bench(threads, period, 30, 100, size);
        time = bench.run();
    } else if (testcase == "kurtosis") {
        ParallelKurtBench bench(threads, period, 100, size);
        time = bench.run();
    } else if (testcase == "eg1") {
        Eg1Bench bench(period, size, 10, 20, size);
        time = bench.run();
    } else if (testcase == "eg2") {
        Eg2Bench bench(period, size, 10, 20, size);
        time = bench.run();
    } else if (testcase == "eg3") {
        Eg3Bench bench(period, size, 10, 20, size);
        time = bench.run();
    } else if (testcase == "eg4") {
        Eg4Bench bench(period, size, 10, 20, size);
        time = bench.run();
    } else if (testcase == "eg5") {
        Eg5Bench bench(period, size, 10, 20, size);
        time = bench.run();
    } else if (testcase == "eg6") {
        Eg6Bench bench(period, size, 10, 20, size);
        time = bench.run();
    } else if (testcase == "eg7") {
        Eg7Bench bench(period, size, 10, 20, size);
        time = bench.run();
    } else if (testcase == "yahoo") {
        ParallelYahooBench bench(threads, period, 100 * period, size);
        time = bench.run();
    } */ else {
        throw runtime_error("Invalid testcase");
    }

    cout << "Throughput(M/s), " << testcase << ", " << threads << ", " << setprecision(3) << (size * threads) / time << endl;

    return 0;
}
