#include <iostream>
#include <iomanip>
#include <sys/resource.h>

#include "tilt_select.h"
#include "tilt_where.h"
#include "tilt_aggregate.h"
#include "tilt_sumwhere.h"
#include "tilt_average.h"
#include "tilt_var.h"
#include "tilt_alterdur.h"
#include "tilt_sliding_sum.h"
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

#include "bd_tilt_select.h"
#include "bd_tilt_where.h"

#include "bdopt_tilt_sum.h"
#include "bdopt_tilt_select.h"
#include "bdopt_tilt_where.h"
#include "bdopt_tilt_sumwhere.h"
#include "bdopt_tilt_avg.h"
#include "bdopt_tilt_stddev.h"


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
    } else if (testcase == "select64") {
        ParallelSelect64Bench bench(threads, period, size);
        time = bench.run();
    } else if (testcase == "select8") {
        ParallelSelect8Bench bench(threads, period, size);
        time = bench.run();
    } else if (testcase == "select_loopIR") {
        SelectBench bench(period, size);
        bench.print_loopIR("select_loopIR.txt");
    } else if (testcase == "select_llvmIR") {
        SelectBench bench(period, size);
        bench.print_llvmIR("select_llvmIR.txt");
    } else if (testcase == "where") {
        ParallelWhereBench bench(threads, period, size);
        time = bench.run();
    } else if (testcase == "where64") {
        ParallelWhere64Bench bench(threads, period, size);
        time = bench.run();
    } else if (testcase == "where8") {
        ParallelWhere8Bench bench(threads, period, size);
        time = bench.run();
    } else if (testcase == "where_loopIR") {
        WhereBench bench(period, size);
        bench.print_loopIR("where_loopIR.txt");
    } else if (testcase == "where_llvmIR") {
        WhereBench bench(period, size);
        bench.print_llvmIR("where_llvmIR.txt");
    } else if (testcase == "aggregate") {
        ParallelAggregateBench bench(threads, period, size, 1000 * period);
        time = bench.run();
    } else if (testcase == "aggregate_loopIR") {
        AggregateBench bench(period, size, 1000 * period);
        bench.print_loopIR("agg_loopIR.txt");
    } else if (testcase == "sum64") {
        ParallelSum64Bench bench(threads, period, size, 1000 * period);
        time = bench.run();
    } else if (testcase == "sum8") {
        ParallelSum8Bench bench(threads, period, size, 1000 * period);
        time = bench.run();
    } else if (testcase == "sumwhere64") {
        ParallelSumWhere64Bench bench(threads, period, size, 1000 * period);
        time = bench.run();
    } else if (testcase == "sumwhere64_loopIR") {
        SumWhere64Bench bench(period, size, 1000 * period);
        bench.print_loopIR("sumwhere64_loopIR.txt");
    } else if (testcase == "sumwhere8") {
        ParallelSumWhere8Bench bench(threads, period, size, 1000 * period);
        time = bench.run();
    } else if (testcase == "sumwhere8_loopIR") {
        SumWhere8Bench bench(period, size, 1000 * period);
        bench.print_loopIR("sumwhere8_loopIR.txt");
    } else if (testcase == "avg") {
        ParallelAverageBench bench(threads, period, size, 1000 * period);
        time = bench.run();
    } else if (testcase == "avg_loopIR") {
        AverageBench bench(period, size, 1000 * period);
        bench.print_loopIR("avg_loopIR.txt");
    } else if (testcase == "avgonepass") {
        ParallelAverageOnePassBench bench(threads, period, size, 1000 * period);
        time = bench.run();
    } else if (testcase == "avgonepass_loopIR") {
        AverageOnePassBench bench(period, size, 1000 * period);
        bench.print_loopIR("avgonepass_loopIR.txt");
    } else if (testcase == "aggregate_llvmIR") {
        AggregateBench bench(period, size, 1000 * period);
        bench.print_llvmIR("agg_llvmIR.txt");
    } else if (testcase == "avgonepass64") {
        ParallelAverageOnePass64Bench bench(threads, period, size, 1000 * period);
        time = bench.run();
    } else if (testcase == "avgonepass8") {
        ParallelAverageOnePass8Bench bench(threads, period, size, 1000 * period);
        time = bench.run();
    } else if (testcase == "var64") {
        ParallelVar64Bench bench(threads, period, 1000 * period, size);
        time = bench.run();
    } else if (testcase == "var64_loopIR") {
        Var64Bench bench(period, 1000 * period, size);
        bench.print_loopIR("var64_loopIR.txt");
    } else if (testcase == "var64onepass") {
        ParallelVar64OnePassBench bench(threads, period, 1000 * period, size);
        time = bench.run();
    } else if (testcase == "naivesum") {
        ParallelNaiveSlidingSumBench bench(threads, period, size);
        time = bench.run();
    } else if (testcase == "naivesum_loopIR") {
        NaiveSlidingSumBench bench(period, size);
        bench.print_loopIR("naivesum_loopIR.txt");
    } else if (testcase == "incsum") {
        ParallelIncSlidingSumBench bench(threads, period, size);
        time = bench.run();
    } else if (testcase == "incsum_loopIR") {
        IncSlidingSumBench bench(period, size);
        bench.print_loopIR("incsum_loopIR.txt");
    }  else if (testcase == "alterdur") {
        AlterDurBench bench(3, 2, size);
        time = bench.run();
    } else if (testcase == "alterdur_loopIR") {
        AlterDurBench bench(3, 2, size);
        bench.print_loopIR("alterdur_loopIR.txt");
    } else if (testcase == "innerjoin") {
        InnerJoinBench bench(period, period, size);
        time = bench.run();
    } else if (testcase == "innerjoin_loopIR") {
        InnerJoinBench bench(period, period, size);
        bench.print_loopIR("innerjoin_loopIR.txt");
    } else if (testcase == "innerjoin_llvmIR") {
        InnerJoinBench bench(period, period, size);
        bench.print_llvmIR("innerjoin_llvmIR.txt");
    } else if (testcase == "outerjoin") {
        OuterJoinBench bench(period, period, size);
        time = bench.run();
    } else if (testcase == "normalize") {
        ParallelNormBench bench(threads, period, 10000, size);
        time = bench.run();
    } else if (testcase == "norm_loopIR") {
        NormBench bench(period, 10000, size);
        bench.print_loopIR("norm_loopIR.txt");
    } else if (testcase == "fillmean") {
        ParallelImputeBench bench(threads, period, 10000, size);
        time = bench.run();
    } else if (testcase == "resample") {
        ParallelResampleBench bench(threads, 4, 5, 1000, size);
        time = bench.run();
    } else if (testcase == "algotrading") {
        ParallelMOCABench bench(threads, period, 20, 50, 100, size);
        time = bench.run();
    } else if (testcase == "moca_loopIR") {
        MOCABench bench(period, 20, 50, 100, size);
        bench.print_loopIR("moca_loopIR.txt");
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
    } else if (testcase == "eg1_loopIR") {
        Eg1Bench bench(period, size, 10, 20, size);
        bench.print_loopIR("eg1_loopIR.txt");
    } else if (testcase == "eg2_loopIR") {
        Eg2Bench bench(period, size, 10, 20, size);
        bench.print_loopIR("eg2_loopIR.txt");
    } else if (testcase == "eg7_loopIR") {
        Eg7Bench bench(period, size, 10, 20, size);
        bench.print_loopIR("eg7_loopIR.txt");
    } else if (testcase == "yahoo") {
        ParallelYahooBench bench(threads, period, 100 * period, size);
        time = bench.run();
    } else if (testcase == "bdselect") {
        ParallelBDSelectBench bench(threads, period, 100 * period, size);
        time = bench.run();
    } else if (testcase == "bdselect_loopIR") {
        BDSelectBench bench(period, 100 * period, size);
        bench.print_loopIR("bdselect_loopIR.txt");
    } else if (testcase == "bdwhere") {
        ParallelBDWhereBench bench(threads, period, 100 * period, size);
        time = bench.run();
    } else if (testcase == "bdwhere_loopIR") {
        BDWhereBench bench(period, 100 * period, size);
        bench.print_loopIR("bdwhere_loopIR.txt");
    } else if (testcase == "bdoptsum") {
        ParallelBDOptSumBench bench(threads, period, size, 1000 * period);
        time = bench.run();
    } else if (testcase == "bdoptsum_loopIR") {
        BDOptSumBench bench(period, size, 1000 * period);
        bench.print_loopIR("bdoptsum_loopIR.txt");
    } else if (testcase == "bdoptsum_llvmIR") {
        BDOptSumBench bench(period, size, 1000 * period);
        bench.print_llvmIR("bdoptsum_llvmIR.txt");
    } else if (testcase == "bdoptselect") {
        ParallelBDOptSelectBench bench(threads, period, size);
        time = bench.run();
    } else if (testcase == "bdoptselect_loopIR") {
        BDOptSelectBench bench(period, size);
        bench.print_loopIR("bdoptselect_loopIR.txt");
    } else if (testcase == "bdoptselect_llvmIR") {
        BDOptSelectBench bench(period, size);
        bench.print_llvmIR("bdoptselect_llvmIR.txt");
    } else if (testcase == "bdoptwhere") {
        ParallelBDOptWhereBench bench(threads, period, size);
        time = bench.run();
    } else if (testcase == "bdoptwhere_loopIR") {
        BDOptWhereBench bench(period, size);
        bench.print_loopIR("bdoptwhere_loopIR.txt");
    } else if (testcase == "bdoptwhere_llvmIR") {
        BDOptWhereBench bench(period, size);
        bench.print_llvmIR("bdoptwhere_llvmIR.txt");
    } else if (testcase == "bdoptsumwhere") {
        ParallelBDOptSumWhereBench bench(threads, period, size, 1000 * period);
        time = bench.run();
    } else if (testcase == "bdoptsumwhere_loopIR") {
        BDOptSumWhereBench bench(period, size, 100 * period);
        bench.print_loopIR("bdoptsumwhere_loopIR.txt");
    } else if (testcase == "bdoptsumwhere_llvmIR") {
        BDOptSumWhereBench bench(period, size, 100 * period);
        bench.print_llvmIR("bdoptsumwhere_llvmIR.txt");
    } else if (testcase == "bdoptavg") {
        ParallelBDOptAvgBench bench(threads, period, size, 100 * period);
        time = bench.run();
    } else if (testcase == "bdoptavg_loopIR") {
        BDOptAvgBench bench(period, size, 1000 * period);
        bench.print_loopIR("bdoptavg_loopIR.txt");
    } else if (testcase == "bdoptavg_llvmIR") {
        BDOptAvgBench bench(period, size, 1000 * period);
        bench.print_llvmIR("bdoptavg_llvmIR.txt");
    } else if (testcase == "bdoptstddev") {
        ParallelBDOptStdDevBench bench(threads, period, size, 100 * period);
        time = bench.run();
    } else if (testcase == "bdoptstddev_loopIR") {
        BDOptStdDevBench bench(period, size, 100 * period);
        bench.print_loopIR("bdoptstddev_loopIR.txt");
    } else if (testcase == "bdoptstddev_llvmIR") {
        BDOptStdDevBench bench(period, size, 100 * period);
        bench.print_llvmIR("bdoptstddev_llvmIR.txt");
    } else {
        throw runtime_error("Invalid testcase");
    }

    cout << "Throughput(M/s), " << testcase << ", " << threads << ", " << setprecision(3) << (size * threads) / time << endl;

    return 0;
}
