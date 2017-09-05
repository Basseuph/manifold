#ifndef SYSBUILDER_LLP_H
#define SYSBUILDER_LLP_H

#include <libconfig.h++>
#include <vector>
#include <set>
#include "kernel/manifold.h"
#include "kernel/component.h"
#include "uarch/DestMap.h"
#include "proc_builder.h"
#include "cache_builder.h"
#include "network_builder.h"
#include "mc_builder.h"
#include "qsim_builder.h"
#include "qsim.h"

#include "sysBuilder_llp.h"

#ifdef LIBKITFOX
#include "kitfox_builder.h"
#endif

// this data structure to hold a node's type and lp
enum {INVALID_NODE=0, EMPTY_NODE, CORE_NODE, MC_NODE, CORE_MC_NODE, L2_NODE};

struct Node_conf_llp {
    Node_conf_llp() : type(INVALID_NODE) {}

    int type;
    int lp;
};

class SysBuilder_llp_systemc : SysBuilder_llp {
public:
    enum FrontendType {FT_QSIMCLIENT, FT_QSIMLIB, FT_QSIMPROXY, FT_TRACE}; // front-end type: QSimClient, QSimLib, trace

    enum { PART_1, PART_2, PART_Y}; //torus partitioning

    SysBuilder_llp_systemc(const char* fname);
    ~SysBuilder_llp_systemc();

    void config_system();
    void build_system(FrontendType type, int n_lps, std::vector<std::string>& args, int part); //for QSim client and tracefile
    void build_system(Qsim::OSDomain* osd, std::vector<std::string>& args); //for QSimLib
    void build_system(std::vector<std::string>& args, const char* appFile, int n_lps, int part); // for QSimProxy

    void pre_simulation();
    void print_config(std::ostream& out);
    void print_stats(std::ostream& out);

    libconfig::Config m_config;

    ProcBuilder* get_proc_builder() { return m_proc_builder; }
    CacheBuilder* get_cache_builder() { return m_cache_builder; }
    NetworkBuilder* get_network_builder() { return m_network_builder; }
    MemControllerBuilder* get_mc_builder() { return m_mc_builder; }
    QsimBuilder* get_qsim_builder() { return m_qsim_builder; }

    manifold::kernel::Ticks_t get_stop_tick() { return STOP; }
    size_t get_proc_node_size() { return proc_node_idx_vec.size(); }
    size_t get_mc_node_size() { return mc_node_idx_vec.size(); }
    int get_y_dim() { return m_network_builder->get_y_dim(); }

    std::map<int, int>& get_proc_id_lp_map() { return proc_id_lp_map; }

    manifold::kernel::Clock* get_default_clock() { return m_default_clock; }

protected:
    //enum { PROC_ZESTO, PROC_SIMPLE, PROC_SPX }; //processor model type

    virtual void config_components();
    virtual void create_nodes(int type, int n_lps, int part);

    virtual void do_partitioning_1_part(int n_lps);
    virtual void do_partitioning_y_part(int n_lps);

    SystemCBuilder* sysC_builder;
 
    std::vector<int> sysC_node_idx_vec;

    std::set<int> sysC_node_idx_set; //set is used to ensure each index is unique

    std::map<int, int> sysC_id_lp_map; //maps mc's node id to its LP

    //int m_processor_type;
private:

    void connect_components();

    void dep_injection_for_sysC();

};





#endif // #ifndef SYSBUILDER_LLP_H
