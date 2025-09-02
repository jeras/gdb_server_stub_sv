///////////////////////////////////////////////////////////////////////////////
// HDLDB shadow (DUT shadow copy)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// SystemVerilog DPI includes
#include "vpi_user.h"

// C includes
#include <csignal>

// C++ includes
#include <string>
#include <vector>

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// GDB shadow class
///////////////////////////////////////////////////////////////////////////////

// 32/64 bit CPU selection
template <typename XLEN, typename FLEN>

class hdldbShadow {

    ///////////////////////////////////////
    // type definitions
    ///////////////////////////////////////

    // point type
    enum Ptype : int {
        swbreak   = 0,  // software breakpoint
        hwbreak   = 1,  // hardware breakpoint
        watch     = 2,  // write  watchpoint
        rwatch    = 3,  // read   watchpoint
        awatch    = 4,  // access watchpoint
        replaylog = 5,  // reached replay log edge
        none      = -1  // no reason is given
    };

    // memory block
    struct MBLK {
        XLEN base;  
        XLEN size;
    };

    ///////////////////////////////////////
    // configuration
    ///////////////////////////////////////

    // number of all registers
    unsigned int gprn =   32;  // GPR number (use 16 for E extension)
    unsigned int fprn =   32;  // floating point registers number
    unsigned int csrn = 4096;  // CSR number  TODO: should be a list of indexes

    // memory map (shadow memory map)
    MBLK * mmap;

    ///////////////////////////////////////
    // shadow state
    ///////////////////////////////////////

    // registers
    XLEN   pc;          // PC  (program counter)
    XLEN * gpr [gprn];  // GPR (general purpose register file)
    FLEN * fpr [fprn];  // FPR (floating point register file)
    XLEN * csr [csrn];  // CSR (configuration status registers)

    // memories
    vector<char> mem;  // vector of address map regions

    // memory mapped I/O registers (covers address space not covered by memories)
        dictionary_t     i_o;

    // instruction counter
    size_t           cnt;

    // trace queue
    retired_t        trc [$];

    // current retired instruction
    retired_t        ret;

        // signal
        signal_t         sig;

        // reason (point type/kind)
        point_t          rsn;

        // associative array for hardware breakpoints/watchpoint
        point_t          bpt [SIZE_T];
        point_t          wpt [SIZE_T];



public:
    
    Rectangle(string name) : m_name(name) {}
  void set_values (int,int);
  int area () {return m_width*m_height;}
  const char* name () {return m_name.c_str();}
};


///////////////////////////////////////////////////////////////////////////////
// constructor
///////////////////////////////////////////////////////////////////////////////

hdldbShadow::hdldbShadow (
    // number of registers
    unsigned int gprn,  // GPR number (use 16 for E extension)
    unsigned int fprn,  // GPR number (use 16 for E extension)
    unsigned int csrn,  // CSR number  TODO: should be a list of indexes

) {

    // registers
    gpr = new XLEN [gprn];
    fpr = new XLEN [fprn];
    csr = new XLEN [csrn];

};
