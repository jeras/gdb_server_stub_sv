///////////////////////////////////////////////////////////////////////////////
// HDLDB shadow (DUT shadow copy)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// SystemVerilog DPI includes
//#include "vpi_user.h"

// C includes
#include <cstdint>
#include <csignal>

// C++ includes
#include <string>
#include <vector>
#include <map>
#include <bitset>
#include <bit>

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

    typedef std::byte Memory [];

    // point type
    enum PointTtype : int {
        swbreak   = 0,  // software breakpoint
        hwbreak   = 1,  // hardware breakpoint
        watch     = 2,  // write  watchpoint
        rwatch    = 3,  // read   watchpoint
        awatch    = 4,  // access watchpoint
        replaylog = 5,  // reached replay log edge
        none      = -1  // no reason is given
    };

    // memory block
    struct MemoryBlock {
        XLEN base;  
        XLEN size;
    };

    // DUT architecture
    struct Architecture {
        // number of all registers
        unsigned int gpr =   32;  // GPR number (use 16 for E extension)
        unsigned int fpr =   32;  // floating point registers number
        unsigned int csr = 4096;  // CSR number  TODO: should be a list of indexes
        // memory map (shadow memory map)
        std::vector<MemoryBlock> map;
    };

    ////////////////////////////////////////
    // retired instruction trace
    ////////////////////////////////////////

    // IFU (instruction fetch unit)
    struct RetiredIfu {
        XLEN      adr;     // instruction address (current PC)
        XLEN      pcn;     // next PC
        std::byte rdt [];  // read data (current instruction)
        bool      ill;     // illegal instruction (DUT behavior might not be defined)
        // TODO: add interrupts
    };

    // GPR (general purpose registers)
    struct RetiredGpr {
        uint8_t   idx;     // GPR index
        XLEN      rdt;     // read  data (current destination register value)
        XLEN      wdt;     // write data (next    destination register value)
    };

    // FPR (floating point registers)
    struct RetiredFpr {
        uint8_t   idx;     // FPR index
        FLEN      rdt;     // read  data (current destination register value)
        FLEN      wdt;     // write data (next    destination register value)
    };

    // CSR (control status registers)
    struct RetiredCsr {
        uint16_t  idx;     // CSR index
        XLEN      rdt;     // read  data
        XLEN      wdt;     // write data
    };

    // LSU (load store unit)
    // (memory/IO access size is encoded in array size)
    struct RetiredLsu {
        XLEN      adr;     // data address
        std::byte rdt [];  // read  data
        std::byte wdt [];  // write data
    };

    // instruction retirement history log entry
    struct Retired {
        RetiredIfu ifu;
        RetiredGpr gpr [];
        RetiredFpr fpr [];
        RetiredCsr csr [];
        RetiredLsu lsu;
    };

    // shadow state structure
    struct Shadow {
        // register files
        XLEN     pc;      // PC  (program counter)
        XLEN     gpr [];  // GPR (general purpose register file)
        FLEN     fpr [];  // FPR (floating point register file)
        XLEN     csr [];  // CSR (configuration status registers)
        // memories (array of address map regions)
        std::vector<Memory>  mem;
        // memory mapped I/O registers (covers address space not covered by memories)
        std::map<XLEN, byte> i_o;

        // associative array for hardware breakpoints/watchpoint
        std::map<XLEN, PointTtype> bpt;
        std::map<XLEN, PointTtype> wpt;

        // instruction counter
        size_t   cnt;
        // current retired instruction
        Retired  ret;
        // trace queue
        vector<Retired> trc;
        // signal
        int         sig;
        // reason (point type/kind)
        PointTtype          rsn;
    };

    ///////////////////////////////////////
    // architecture and shadow configuration array
    ///////////////////////////////////////

    int cores;

    std::vector<Architecture> arch;
    std::vector<Shadow> shadow;

public:
    // constructor/destructor
    hdldbShadow (const std::vector<Architecture>);
    ~hdldbShadow ();

//    Rectangle(string name) : m_name(name) {}
//  void set_values (int,int);
//  int area () {return m_width*m_height;}
//  const char* name () {return m_name.c_str();}
};


///////////////////////////////////////////////////////////////////////////////
// constructor/destructor
///////////////////////////////////////////////////////////////////////////////

template <typename XLEN, typename FLEN>
hdldbShadow<XLEN, FLEN>::hdldbShadow (
    const std::vector<Architecture> arch
) {
    // architecture
    arch = arch;

    // initialize DUT shadow copy
    for (unsigned int core=0; core<arch.size(); core++) {
        // register files
        shadow[core].gpr = new XLEN [arch[core].gpr];
        shadow[core].fpr = new FLEN [arch[core].fpr];
        shadow[core].csr = new XLEN [arch[core].csr];
        // memories
        for (unsigned int i=0; i<arch[core].map.size(); i++) {
            shadow[core].mem.push_back(new Memory [arch[core].mem[i].size]);
        }
        // instruction counter
        shadow[core].cnt = 0;
        // signal
        shadow[core].sig = SIGTRAP;
        // reason (point type/kind)
        // reached beginning of trace
        //rsn;
    };
};

template <typename XLEN, typename FLEN>
hdldbShadow<XLEN, FLEN>::~hdldbShadow () {
    // initialize DUT shadow copy
    for (unsigned int core=0; core<arch.size(); core++) {
        // memories
        for (unsigned int i=0; i<arch[core].map.size(); i++) {
            delete shadow[core].mem[i];
        }
        // register files
        delete shadow[core].gpr;
        delete shadow[core].fpr;
        delete shadow[core].csr;
    };
};
