#ifndef TILT_BENCH_INCLUDE_TILT_AGGREGATE_H_
#define TILT_BENCH_INCLUDE_TILT_AGGREGATE_H_

#include "tilt/builder/tilder.h"
#include "tilt_base.h"
#include "tilt_bench.h"

using namespace tilt;
using namespace tilt::tilder;

/*** Uncompressed Aggregate Benchmark and related classes ***/

class AggregateBench : public Benchmark {
public:
    AggregateBench(dur_t period, int64_t size, int64_t w) :
        period(period), size(size), w(w)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT64, _iter(0, -1)));
        return _WindowSum(in_sym, w);
    }

    void init() final
    {
        in_reg = create_reg<int64_t>(size);
        float osize = (float)size / (float)w;
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
        print_reg<int64_t>(&in_reg, "agg_in_reg.txt");
        print_reg<int64_t>(&out_reg, "agg_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
    int64_t w;
};

class ParallelAggregateBench : public ParallelBenchmark {
public:
    ParallelAggregateBench(int threads, dur_t period, int64_t size, int64_t w)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new AggregateBench(period, size, w));
        }
    }
};

/*** Base-Delta compressed Aggregate Benchmark and related classes ***/

class BDAggregateBench : public Benchmark {
public:
    BDAggregateBench(dur_t period, int64_t size, int64_t w, uint32_t block_size) :
        period(period), size(size), w(w), block_size(block_size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::BASEDELTA<int64_t, int8_t>(), _iter(0, -1)));
        return _WindowSum(in_sym, w);
    }

    void init() final
    {
        in_reg = create_bd_reg<int64_t, int8_t>(size, block_size);
        float osize = (float)size / (float)w;
        out_reg = create_reg<int64_t>(ceil(osize));

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
        print_bd_reg<int64_t, int8_t>(&in_reg, "bdagg_in_reg.txt");
        print_reg<int64_t>(&out_reg, "bdagg_out_reg.txt");
#endif
        release_bd_reg(&in_reg);
        release_reg(&out_reg);
    }

    bd_region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
    int64_t w;
    uint32_t block_size;
};

class ParallelBDAggregateBench : public ParallelBenchmark {
public:
    ParallelBDAggregateBench(int threads, dur_t period, int64_t size, int64_t w, uint32_t block_size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new BDAggregateBench(period, size, w, block_size));
        }
    }
};


#endif  // TILT_BENCH_INCLUDE_TILT_AGGREGATE_H_
