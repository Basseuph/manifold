#include "kernel/component.h"
#include "kernel/manifold.h"
#include "manifold_systemc.h"
//#include "Transaction.h"

#include <iostream>

#include <systemc.h>

using namespace std;
using namespace manifold::kernel;

namespace manifold {
namespace manifold_systemc {

int ManifoldSystemc_sim:: MEM_MSG_TYPE = -1;
int ManifoldSystemc_sim :: CREDIT_MSG_TYPE = -1;
bool ManifoldSystemc_sim :: Msg_type_set = false;

// dummy providing the entry point for systemc lib
int sc_main(int argc, char **argv) {
  printf("This is a dummy");
}

ManifoldSystemc_sim::ManifoldSystemc_sim (int nid, const ManifoldSystemc_sim_settings& manifoldSystemc_settings, Clock& clk) :
    m_nid(nid)
{
    assert(Msg_type_set);
    assert(MEM_MSG_TYPE != CREDIT_MSG_TYPE);

    m_send_st_response = manifoldSystemc_settings.send_st_resp;
    downstream_credits = manifoldSystemc_settings.downstream_credits;
    frequency          = manifoldSystemc_settings.frequency;
    initiation_interval= manifoldSystemc_settings.initiation_interval;
    latency            = manifoldSystemc_settings.latency;
    
    /* instantiate the DRAMSim module */
//     mem = new MultiChannelMemorySystem(dram_settings.dev_filename, dram_settings.mem_sys_filename, ".", "res", dram_settings.size);

    mc_map = NULL;

    /* create and register DRAMSim callback functions */
//     read_cb = new Callback<ManifoldSystemc_sim, void, unsigned, uint64_t, uint64_t>(this, &ManifoldSystemc_sim::read_complete);
//     write_cb = new Callback<ManifoldSystemc_sim, void, unsigned, uint64_t, uint64_t>(this, &ManifoldSystemc_sim::write_complete);
//     mem->RegisterCallbacks(read_cb, write_cb, NULL);

    //register with clock
    Clock :: Register(clk, this, &ManifoldSystemc_sim::rising, &ManifoldSystemc_sim::falling);

    //stats
    stats_n_reads = 0;
    stats_n_writes = 0;
    stats_n_reads_sent = 0;
    stats_totalMemLat = 0;

#ifdef MANIFOLD_SYSTEMC_UTEST
    completed_writes = 0;
#endif
    
    // start systemc simulation part
    sc_start();
    
    cout << "startet SystemC environment" << endl;
    
    // open trace file
    waveform = sc_create_vcd_trace_file("test_trace");
    
    // dump the signals (currently only the clock is available)
    sc_trace(waveform, clock, "clock");
    sc_trace(waveform, reset, "reset");
    
    // assign initials
    reset = 0;
    
    
}

ManifoldSystemc_sim::~ManifoldSystemc_sim()
{
    sc_close_vcd_trace_file(ManifoldSystemc_sim::waveform);
}



void ManifoldSystemc_sim::read_complete(unsigned id, uint64_t address, uint64_t done_cycle)
{
    //cout << "@ " << m_clk->NowTicks() << " (local) " << Manifold::NowTicks() << " (default), read complete\n";
    map<uint64_t, list<Request> >::iterator it = m_pending_reqs.find(address);
    assert (it != m_pending_reqs.end());

    Request req = m_pending_reqs[address].front();
    m_pending_reqs[address].pop_front();
    if (it->second.size() == 0)
        m_pending_reqs.erase(it);

    assert(req.read);
    assert(req.addr == address);

    m_completed_reqs.push_back(req);
}


void ManifoldSystemc_sim::write_complete(unsigned id, uint64_t address, uint64_t done_cycle)
{
    map<uint64_t, list<Request> >::iterator it = m_pending_reqs.find(address);
    assert (it != m_pending_reqs.end());

    Request req = m_pending_reqs[address].front();
    m_pending_reqs[address].pop_front();
    if (it->second.size() == 0)
        m_pending_reqs.erase(it);

    assert(req.read == false);
    assert(req.addr == address);

    //move from pending buffer to completed buffer
    if (m_send_st_response) {
        m_completed_reqs.push_back(req);
    }

#ifdef MANIFOLD_SYSTEMC_UTEST
    completed_writes++;
#endif
}


void ManifoldSystemc_sim :: try_send_reply()
{
    if ( !m_completed_reqs.empty() && downstream_credits > 0) {
        //stats
      stats_n_reads_sent++;
      stats_totalMemLat += (m_clk->NowTicks() - m_completed_reqs.front().r_cycle);

      Request req = m_completed_reqs.front();
      m_completed_reqs.pop_front();

      uarch::Mem_msg mem_msg(req.gaddr, req.read);

      manifold::uarch::NetworkPacket * pkt = (manifold::uarch::NetworkPacket*)(req.extra);
      pkt->type = MEM_MSG_TYPE;
      *((uarch::Mem_msg*)(pkt->data)) = mem_msg;
      pkt->data_size = sizeof(uarch::Mem_msg);

#ifdef DBG_MANIFOLD_SYSTEMCSIM
      cout << "@ " << m_clk->NowTicks() << " MC " << m_nid << " sending reply: addr= " << hex << req.gaddr << dec << " destination= " << pkt->get_dst() << endl;
#endif
      Send(PORT0, pkt);
      downstream_credits--;
    }
}


void ManifoldSystemc_sim :: send_credit()
{
    manifold::uarch::NetworkPacket *credit_pkt = new manifold::uarch::NetworkPacket();
    credit_pkt->type = CREDIT_MSG_TYPE;
    Send(PORT0, credit_pkt);
}


bool ManifoldSystemc_sim::limitExceeds()
{
    //TODO this is the request reply dependency
    // PendingRequest has to be limited. Cannot exceed indefinitely
    return (m_completed_reqs.size() > 8); // some low threshold
}

// rising clock event
void ManifoldSystemc_sim::rising()
{
    // rising clock edge
    ManifoldSystemc_sim::clock = 1;
    
    //cout << "Dram sim tick(), t= " << m_clk->NowTicks() << endl;
    //start new transaction if there is any and the memory can accept
  
    cout << "@ " << m_clk->NowTicks() << "rising event" << endl;
    
// call to actual DRAMSim
//     if (!m_incoming_reqs.empty() && mem->willAcceptTransaction() && !limitExceeds()) {
    if (!m_incoming_reqs.empty() && !limitExceeds()) {
      // if limit exceeds, stop sending credits. interface will stop eventually
      Request req = m_incoming_reqs.front();
      m_incoming_reqs.pop_front();

// actual call to DRAMSim
//       mem->addTransaction(!req.read, req.addr);
#ifdef DBG_MANIFOLD_SYSTEMCSIM
      cout << "@ " << m_clk->NowTicks() << " MC " << m_nid << ": transaction of address " << hex << req.gaddr << dec << " is pushed to memory" << endl;
#endif
      //move from input buffer to pending buffer
          m_pending_reqs[req.addr].push_back(req);
          send_credit();
    }

// call to actual DRAMSim
//       mem->update();
      try_send_reply();

}

// falling clock event
void ManifoldSystemc_sim::falling()
{
  
    // falling clock edge
    ManifoldSystemc_sim::clock = 0;
    //cout << "Dram sim tick(), t= " << m_clk->NowTicks() << endl;
    //start new transaction if there is any and the memory can accept
  
    cout << "@ " << m_clk->NowTicks() << "falling event" << endl;
  
// call to actual DRAMSim
//     if (!m_incoming_reqs.empty() && mem->willAcceptTransaction() && !limitExceeds()) {
    if (!m_incoming_reqs.empty() && !limitExceeds()) {
      // if limit exceeds, stop sending credits. interface will stop eventually
      Request req = m_incoming_reqs.front();
      m_incoming_reqs.pop_front();

// actual call to DRAMSim
//       mem->addTransaction(!req.read, req.addr);
#ifdef DBG_MANIFOLD_SYSTEMCSIM
      cout << "@ " << m_clk->NowTicks() << " MC " << m_nid << ": transaction of address " << hex << req.gaddr << dec << " is pushed to memory" << endl;
#endif
      //move from input buffer to pending buffer
          m_pending_reqs[req.addr].push_back(req);
          send_credit();
    }

// call to actual DRAMSim
//       mem->update();
      try_send_reply();

}

void ManifoldSystemc_sim :: set_mc_map(manifold::uarch::DestMap *m)
{
    this->mc_map = m;
}

void ManifoldSystemc_sim :: print_stats(ostream& out)
{
    out << "***** MANIFOLD_SYSTEMC Accel Sim " << m_nid << " *****" << endl;
    out << "  Total Reads Received= " << stats_n_reads << endl;
    out << "  Total Writes Received= " << stats_n_writes << endl;
    out << "  Total Reads Sent Back= " << stats_n_reads_sent << endl;
    out << "  Avg Memory Latency= " << (double)stats_totalMemLat / stats_n_reads_sent << endl;
    out << "  Reads per source:" << endl;
    for(map<int, unsigned>::iterator it=stats_n_reads_per_source.begin(); it != stats_n_reads_per_source.end();
           ++it) {
        out << "    " << it->first << ": " << it->second << endl;
    }
    out << "  Writes per source:" << endl;
    for(map<int, unsigned>::iterator it=stats_n_writes_per_source.begin(); it != stats_n_writes_per_source.end();
        ++it) {
        out << "    " << it->first << ": " << it->second << endl;
    }

// call to actual DRAMSim
//     mem->printStats(true);
}

void ManifoldSystemc_sim :: print_config(ostream& out)
{
    out << "***** MANIFOLD_SYSTEMC Accel Sim " << m_nid << " *****" << endl;
    out << "MANIFOLD_SYSTEMC Accelerator configuration" << endl;
    out << "Clock frequency:     "  << frequency << endl;
    out << "Initiation interval: "  << initiation_interval / frequency << endl;
    out << "Latency:             "  << latency / frequency << endl;
}

} // namespace MANIFOLD_SYSTEMC
} //namespace manifold
