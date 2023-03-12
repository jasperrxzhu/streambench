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

/* classes for datasets over uncompressed streams */

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

/* classes for datasets over base-delta compressed streams */

template<typename Tbase, typename Tdelta>
class BDDataset {
public:
    virtual void fill(cmp_region_t*) = 0;
};

template<typename Tbase, typename Tdelta>
class SynthAllCmpBDData : public BDDataset<Tbase, Tdelta>{
public:
    SynthAllCmpBDData(dur_t period, int64_t len, int64_t block_size) : 
        period(period), len(len), block_size(block_size)
    {}

    void fill(cmp_region_t* reg) final
    {
        double base_range = 100;
        double min_base = -100;
        int delta_range = 100;
        auto num_blocks = len / block_size;

        // populate full blocks
        for(int i = 0; i < num_blocks; i++){
            auto t = (i + 1) * period * block_size;
            commit_cmp_block(reg, t, block_size, true);
            auto b_mdata = reg->block_encodings + i;

            // populate bases for timestamps and data
            auto* base_tl = reg->tl + i;
            base_tl->t = i * period * block_size;
            base_tl->d = period;
            auto* base_data_ptr = reinterpret_cast<Tbase*>(reg->data + b_mdata->block_start_location);
            *base_data_ptr = static_cast<Tbase>(rand() / static_cast<double>(RAND_MAX / base_range)) + min_base;

            for(int j = 0; j < block_size; j++){
                auto* delta_tl = reg->delta_tl + ((i * block_size) + j);
                delta_tl->t = j * period;
                delta_tl->d = 0;

                /*
                auto* tl = reg->tl + ((i * block_size) + j);
                tl->t = (i * period * block_size) + (j * period);
                tl->d = period;
                */

                auto* delta_data_ptr = reinterpret_cast<Tdelta*>(reg->data + b_mdata->block_start_location
                                                                 + sizeof(Tbase) + j * sizeof(Tdelta));
                *delta_data_ptr = static_cast<Tdelta>(rand() % delta_range);
            }
        }

        // populate leftover block if applicable
        if(num_blocks * block_size < len){
            auto t = (num_blocks * block_size * period)
                     + ((len - (num_blocks * block_size)) * period);
            commit_cmp_block(reg, t, len - (num_blocks * block_size), true);
            auto b_mdata = reg->block_encodings + num_blocks;

            auto* base_tl = reg->tl + num_blocks;
            base_tl->t = num_blocks * period * block_size;
            base_tl->d = period;
            auto* base_data_ptr = reinterpret_cast<Tbase*>(reg->data + b_mdata->block_start_location);
            *base_data_ptr = static_cast<Tbase>(rand() / static_cast<double>(RAND_MAX / base_range)) + min_base;

            for(int j = 0; j < (len - num_blocks * block_size); j++){
                auto* delta_tl = reg->delta_tl + ((num_blocks * block_size) + j);
                delta_tl->t = j * period;
                delta_tl->d = 0;

                /*
                auto* tl = reg->tl + ((num_blocks * block_size) + j);
                tl->t = (num_blocks * period * block_size) + (j * period);
                tl->d = period;
                */

                auto* delta_data_ptr = reinterpret_cast<Tdelta*>(reg->data + b_mdata->block_start_location
                                                                 + sizeof(Tbase) + j * sizeof(Tdelta));
                *delta_data_ptr = static_cast<Tdelta>(rand() % delta_range);
            }
        }
    }

private:
    dur_t period;
    int64_t len;
    int64_t block_size;
};

/*
    Benchmark classes
    Functions for creating and releasing streams, compiling and executing queries
*/

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
    static cmp_region_t create_cmp_reg(int64_t size, uint32_t block_size)
    {
        cmp_region_t reg;
        auto num_blocks = size / block_size;

        ival_t* base_tl;
        if((num_blocks * block_size) < size){
            base_tl = new ival_t[num_blocks + 1];
        } else {
            base_tl = new ival_t[num_blocks];
        }

        auto delta_tl = new cmp_ival_t[size];

        cmp_mdata* block_encodings;
        if((num_blocks * block_size) < size){
            block_encodings = new cmp_mdata[num_blocks + 1];
        } else {
            block_encodings = new cmp_mdata[num_blocks];
        }

        char* data_blocks;
        int64_t data_size;
        data_size = (sizeof(Tbase) + block_size * sizeof(Tdelta)) * num_blocks;
        if((num_blocks * block_size) < size){
            data_size += sizeof(Tbase) + ((size - num_blocks * block_size) * sizeof(Tdelta));
        }
        data_blocks = new char[data_size];

        init_cmp_region(&reg, 0,
                        base_tl, delta_tl,
                        block_encodings, data_blocks);
        return reg;
    }

    static void release_reg(region_t* reg)
    {
        delete [] reg->tl;
        delete [] reg->data;
    }

    static void release_cmp_reg(cmp_region_t* reg)
    {
        delete [] reg->tl;
        delete [] reg->data;
        delete [] reg->delta_tl;
        delete [] reg->block_encodings;
    }

#ifdef _PRINT_REGION_
    template<typename T>
    void print_reg(region_t* reg, string fname)
    {
        ofstream f;
        f.open( fname );

        auto data = reinterpret_cast<T*>(reg->data);
        idx_t end = get_end_idx(reg);

        f << "Metadata:" << endl;
        f << "st: " << reg->st << endl;
        f << "et: " << reg->et << endl;
        f << "head: " << reg->head << endl;
        f << "count: " << reg->count << endl;

        for (int i = 0; i <= end; i++) {
            auto* ptr = data + i;
            if(ptr) {
                f << reg->tl[i].t << ' ' << reg->tl[i].d << ' ' << (int)*ptr << endl;
            }
        }

        f.close();
    }

    template<typename Tbase, typename Tdelta>
    void print_cmp_reg(cmp_region_t* reg, int block_size, string fname)
    {
        ofstream f;
        f.open(fname);

        auto num_blocks = reg->num_blocks;

        f << "Metadata:" << endl;
        f << "st: " << reg->st << endl;
        f << "et: " << reg->et << endl;
        f << "head: " << reg->head << endl;
        f << "count: " << reg->count << endl;

        for(int i = 0; i < num_blocks; i++){
            f << "Block " << i << ":" << endl;
            f << "Metadata:" << endl;

            cmp_mdata* i_mdata = reg->block_encodings + i;
            f << "block_start_location: " << i_mdata->block_start_location << endl;
            f << "next_block_start_location: " << i_mdata->next_block_start_location << endl;

            f << "Bases:" << endl;
            f << reg->tl[i].t << endl;
            f << (int) reg->tl[i].d << endl;

            if(i_mdata->cmp) {
                f << *((Tbase*)(reg->data + i_mdata->block_start_location)) << endl;
            }

            f << "Deltas:" << endl;
            auto loop_end = block_size;
            if((i * block_size) + block_size - 1 >= reg->count){
                loop_end = reg->count - (i * block_size);
            }
            for(int j = 0; j < loop_end; j++){
                f << (int)(reg->delta_tl[(i * block_size) + j].t) << " "
                  << (int)(reg->delta_tl[(i * block_size) + j].d) << " ";
                /*
                f << (int)(reg->tl[(i * block_size) + j].t) << " "
                  << (int)(reg->tl[(i * block_size) + j].d) << " ";
                */
                if(i_mdata->cmp){
                    f << (int)*((Tdelta*)
                         (reg->data + i_mdata->block_start_location+ sizeof(Tbase) + (j * sizeof(Tdelta)))) << endl;
                } else {
                    f << *((Tbase*)
                         (reg->data + i_mdata->block_start_location + (sizeof(Tbase) * j))) << endl;
                }
            } 
        }

        f.close();
    }

#endif // _PRINT_REGION_

    void print_loopIR( string fname )
    {
        auto query_op = query();
        auto query_op_sym = _sym("query", query_op);

        cout << "Starting LoopGen" << endl;
        auto loop = LoopGen::Build(query_op_sym, query_op.get());

        cout << "Starting printing" << endl;
        ofstream f;
        f.open(fname);
        f << IRPrinter::Build(loop);
        f.close();
        cout << "Finished printing" << endl;
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
