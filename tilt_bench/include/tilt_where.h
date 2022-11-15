#ifndef TILT_BENCH_INCLUDE_TILT_WHERE_H_
#define TILT_BENCH_INCLUDE_TILT_WHERE_H_

#include "tilt/builder/tilder.h"
#include "tilt_bench.h"
#include "tilt_base.h"

#include "iostream"

using namespace tilt;
using namespace tilt::tilder;

/*** Uncompressed Where Benchmark and related classes ***/

class WhereBench : public Benchmark {
public:
    WhereBench(dur_t period, int64_t size) :
        period(period), size(size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::INT64, _iter(0, -1)));
        return _Where(in_sym, [](_sym in) { return _gt(in, _i64(0)); });
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
        print_reg<int64_t>(&in_reg, "where_in_reg.txt");
        print_reg<int64_t>(&out_reg, "where_out_reg.txt");
#endif
        release_reg(&in_reg);
        release_reg(&out_reg);
    }

    region_t in_reg;
    region_t out_reg;

    int64_t size;
    dur_t period;
};

class ParallelWhereBench : public ParallelBenchmark {
public:
    ParallelWhereBench(int threads, dur_t period, int64_t size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new WhereBench(period, size));
        }
    }
};

/*** Base-Delta compressed Where Benchmark and related classes ***/

class BDWhereBench : public Benchmark {
public:
    BDWhereBench(dur_t period, int64_t size, uint32_t block_size) :
        period(period), size(size), block_size(block_size)
    {}

private:
    Op query() final
    {
        auto in_sym = _sym("in", tilt::Type(types::BASEDELTA<int64_t, int8_t>(), _iter(0, -1)));
        return _Where(in_sym, [](_sym in) { return _gt(in, _i64(0)); });
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
        print_bd_reg<int64_t, int8_t>(&in_reg, "bdwhere_in_reg.txt");
        print_reg<int64_t>(&out_reg, "bdwhere_out_reg.txt");
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

class ParallelBDWhereBench : public ParallelBenchmark {
public:
    ParallelBDWhereBench(int threads, dur_t period, int64_t size, uint32_t block_size)
    {
        for (int i = 0; i < threads; i++) {
            benchs.push_back(new BDWhereBench(period, size, block_size));
        }
    }
};

#endif  // TILT_BENCH_INCLUDE_TILT_WHERE_H_
