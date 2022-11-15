#ifndef TILT_BENCH_INCLUDE_TILT_BENCH_H_
#define TILT_BENCH_INCLUDE_TILT_BENCH_H_

#include <memory>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <fstream>

#include "tilt/codegen/loopgen.h"
#include "tilt/codegen/llvmgen.h"
#include "tilt/codegen/vinstr.h"
#include "tilt/engine/engine.h"
#include "tilt/codegen/printer.h"

using namespace std;
using namespace std::chrono;
using namespace tilt;
using namespace tilt::tilder;

/*** uncompressed Dataset and derived classes ***/

template<typename T>
class Dataset {
public:
    virtual void fill(region_t*) = 0;
};

template<typename T>
class SynthData : public Dataset<T> {
public:
    SynthData(dur_t period, int64_t len) : period(period), len(len) {}

    void fill(region_t* reg) final
    {
        double range = 100;

        auto data = reinterpret_cast<T*>(reg->data);
        for (int i = 0; i < len; i++) {
            auto t = period * (i + 1);
            commit_data(reg, t);
            auto* ptr = reinterpret_cast<T*>(fetch(reg, t, get_end_idx(reg), sizeof(T)));
            *ptr = static_cast<T>(rand() / static_cast<double>(RAND_MAX / range)) - (range / 2);
        }
    }

private:
    dur_t period;
    int64_t len;
};

struct Yahoo {
    long user_id;
    long camp_id;
    long event_type;
    long padding;

    Yahoo(int user_id, int camp_id, int event_type) :
        user_id(user_id), camp_id(camp_id), event_type(event_type)
    {}

    Yahoo() {}
};

class YahooData : public Dataset<Yahoo> {
public:
    YahooData(dur_t period, int64_t len) : period(period), len(len) {}

    void fill(region_t* reg) final
    {
        double range = 100;

        auto data = reinterpret_cast<Yahoo*>(reg->data);
        for (int i = 0; i < len; i++) {
            auto t = period * (i + 1);
            commit_data(reg, t);
            auto* ptr = reinterpret_cast<Yahoo*>(fetch(reg, t, get_end_idx(reg), sizeof(Yahoo)));
            *ptr = Yahoo(rand() % 5 + 1, rand() % 5 + 1, rand() % 5 + 1);
        }
    }

private:
    dur_t period;
    int64_t len;
};

/*** Base-Delta compressed Dataset and derived classes ***/

template<typename Tbase, typename Tdelta>
class BDDataset {
public:
    virtual void fill(bd_region_t*) = 0;
};

template<typename Tbase, typename Tdelta>
class SynthBDData : public BDDataset<Tbase, Tdelta>{
public:
    SynthBDData(dur_t period, int64_t len) : period(period), len(len) {}

    void fill(bd_region_t* bd_reg) final
    {
        double base_range = 100;
        double delta_range = 100;
        auto block_size = bd_reg->block_size;

        for(int i = 0; i < len; i++){
            auto t = period * (i + 1);
            if(i % block_size == 0) {
                commit_block(bd_reg, i, true);
                auto* base_ptr = reinterpret_cast<Tbase*>(fetch_base(bd_reg, i, sizeof(Tbase)));
                *base_ptr = static_cast<Tbase>(rand() / static_cast<double>(RAND_MAX / base_range)) - (base_range / 2);;
            }
            bd_commit_data(bd_reg, t);
            auto* ptr = reinterpret_cast<Tdelta*>(fetch_delta(bd_reg, t, i, sizeof(Tdelta)));
            *ptr = static_cast<Tdelta>(rand() / static_cast<double>(RAND_MAX / delta_range)) - (delta_range / 2);;
        }
        
    }

private:
    dur_t period;
    int64_t len;
};


/*** Benchmark and derived classes ***/

class Benchmark {
public:
    virtual void init() = 0;
    virtual void release() = 0;

    intptr_t compile()
    {
        auto query_op = query();
        auto query_op_sym = _sym("query", query_op);

        auto loop = LoopGen::Build(query_op_sym, query_op.get());

        auto jit = ExecEngine::Get();
        auto& llctx = jit->GetCtx();
        auto llmod = LLVMGen::Build(loop, llctx);
        jit->AddModule(move(llmod));
        auto addr = jit->Lookup(loop->get_name());

        return addr;
    }

    int64_t run()
    {
        auto addr = compile();

        init();

        auto start_time = high_resolution_clock::now();
        execute(addr);
        auto end_time = high_resolution_clock::now();

        release();

        return duration_cast<microseconds>(end_time - start_time).count();
    }

    template<typename T>
    static region_t create_reg(int64_t size)
    {
        region_t reg;
        auto buf_size = get_buf_size(size);
        auto tl = new ival_t[buf_size];
        auto data = new T[buf_size];
        init_region(&reg, 0, buf_size, tl, reinterpret_cast<char*>(data));
        return reg;
    }

    template<typename Tbase, typename Tdelta>
    static bd_region_t create_bd_reg(int64_t size, uint32_t block_size)
    {
        bd_region_t bd_reg;
        auto buf_size = get_buf_size(size);
        auto tl = new ival_t[buf_size];
        auto data = new Tbase[buf_size];
        auto delta_data = new Tdelta[buf_size];
        auto cmp_map = new bool[buf_size / block_size];
        init_bd_region(&bd_reg, 0, buf_size, tl, reinterpret_cast<char*>(data),
                       block_size, reinterpret_cast<char*>(delta_data), cmp_map);
        return bd_reg;
    }

    static void release_reg(region_t* reg)
    {
        delete [] reg->tl;
        delete [] reg->data;
    }

    static void release_bd_reg(bd_region_t* bd_reg)
    {
        delete [] bd_reg->tl;
        delete [] bd_reg->data;
        delete [] bd_reg->delta_data;
        delete [] bd_reg->cmp_map;
    }

#ifdef _PRINT_REGION_
    template<typename T>
    void print_reg(region_t* reg, string fname)
    {
        ofstream f;
        f.open( fname );

        auto* data = reinterpret_cast<T*>(reg->data);
        idx_t end = get_end_idx(reg);

        for (int i = 0; i <= end; i++) {
            auto* ptr = data + i;
            if(ptr) {
                f << reg->tl[i].t << ' ' << reg->tl[i].d << ' ' << *ptr << endl;
            }
        }

        f.close();
    }

    template<typename Tbase, typename Tdelta>
    void print_bd_reg(bd_region_t* bd_reg, string fname)
    {
        ofstream f;
        f.open(fname);

        auto* base_data = reinterpret_cast<Tbase*>(bd_reg->data);
        auto* delta_data = reinterpret_cast<Tdelta*>(bd_reg->delta_data);
        auto end = get_end_idx(bd_reg);

        f << "Bases:" << endl;
        for(int i = 0; i <= end; i++){
            f << base_data[i] << endl;
        }

        f << "Deltas:" << endl;
        for (int i = 0; i <= end; i++) {
            auto* ptr = delta_data + i;
            if(ptr) {
                auto* base_ptr = reinterpret_cast<Tbase*>(fetch_base(bd_reg, i, sizeof(Tbase)));
                f << bd_reg->tl[i].t << ' ' << bd_reg->tl[i].d << ' ' 
                  << (bool)*fetch_cmp(bd_reg, bd_reg->tl[i].t+1, i) << ' '
                  << *base_ptr << ' ' << (int)*ptr << endl;
            }
        }

        f.close();
    }
#endif // _PRINT_REGION_

    void print_loopIR( string fname )
    {
        auto query_op = query();
        auto query_op_sym = _sym("query", query_op);

        auto loop = LoopGen::Build(query_op_sym, query_op.get());

        ofstream f;
        f.open(fname);
        f << IRPrinter::Build(loop);
        f.close();
    }

    void print_llvmIR( string fname )
    {
        auto query_op = query();
        auto query_op_sym = _sym("query", query_op);

        auto loop = LoopGen::Build(query_op_sym, query_op.get());

        auto jit = ExecEngine::Get();
        auto& llctx = jit->GetCtx();
        auto llmod = LLVMGen::Build(loop, llctx);

        ofstream f;
        f.open( fname );
        f << IRPrinter::Build( move( llmod ) );
        f.close();
    }

    virtual Op query() = 0;
    virtual void execute(intptr_t) = 0;
};

class ParallelBenchmark {
public:
    int64_t run()
    {
        auto addr = benchs[0]->compile();

        for (int i = 0; i < benchs.size(); i++) {
            benchs[i]->init();
        }

        vector<thread> splits;
        auto start_time = high_resolution_clock::now();
        for (int i = 0; i < benchs.size(); i++) {
            auto bench = benchs[i];
            splits.push_back(thread([bench, addr]() {
                bench->execute(addr);
            }));
        }
        for (int i = 0; i < benchs.size(); i++) {
            splits[i].join();
        }
        auto end_time = high_resolution_clock::now();

        for (int i = 0; i < benchs.size(); i++) {
            benchs[i]->release();
        }

        return duration_cast<microseconds>(end_time - start_time).count();
    }

    vector<Benchmark*> benchs;
};

#endif  // TILT_BENCH_INCLUDE_TILT_BENCH_H_
