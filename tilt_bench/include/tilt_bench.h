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
        for (int i = 0; i < period * len; i++) {
            commit_data(reg);
            auto* ptr = reinterpret_cast<T*>(fetch(reg, get_end_idx(reg), sizeof(T)));
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
        for (int i = 0; i < period * len; i++) {
            commit_data(reg);
            auto* ptr = reinterpret_cast<Yahoo*>(fetch(reg, get_end_idx(reg), sizeof(Yahoo)));
            *ptr = Yahoo(rand() % 5 + 1, rand() % 5 + 1, rand() % 5 + 1);
        }
    }

private:
    dur_t period;
    int64_t len;
};


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
        auto data = new T[buf_size];
        init_region(&reg, 0, buf_size, nullptr, reinterpret_cast<char*>(data));
        return reg;
    }

    static void release_reg(region_t* reg)
    {
        // delete [] reg->tl;
        delete [] reg->data;
    }

#ifdef _PRINT_REGION_
    template<typename T>
    void print_reg(region_t* reg, string fname)
    {
        ofstream f;
        f.open( fname );

        auto data = reinterpret_cast<T*>(reg->data);
        idx_t end = get_end_idx(reg);

        for (int i = 0; i <= end; i++) {
            auto* ptr = data + i;
            if(ptr) {
                f << *ptr << endl;
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
