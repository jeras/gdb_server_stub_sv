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
#include <array>
#include <map>
#include <bitset>
#include <bit>
#include <utility>

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// HDLDB registers class
///////////////////////////////////////////////////////////////////////////////

// 32/64 bit CPU selection
template <typename XLEN, typename FLEN>

class hdldbRegisters {

    ///////////////////////////////////////
    // DUT access
    ///////////////////////////////////////

//    dutSet 

    ///////////////////////////////////////
    // RSP access
    ///////////////////////////////////////

//    rsp

};

// 32/64 bit CPU selection
template <
    // register widths are defined as types
    typename XLEN,
    typename FLEN,
    typename VLEN,
    // extensions
    bool extE,  // 16 GPR register file
    bool extF,  // F extension (single precision floating point)
    bool extD,  // D extension (double precision floating point)
    bool extQ,  // Q extension (quad   precision floating point)
    bool extV   // V extension (vector)
>

class riscvRegisters {

    enum regSet : int {GPR, PC, FPR, VEC, CSR};

    // register files
    std::array<XLEN,   32> gpr;  // GPR (general purpose register file)
               XLEN        pc;   // PC  (program counter)
//  std::array<FLEN,   32> fpr;  // FPR (floating point register file)
//  std::array<VLEN,   32> vec;  // CSR (configuration status registers)
    std::array<XLEN, 4096> csr;  // CSR (configuration status registers)

    ///////////////////////////////////////
    // DUT access
    ///////////////////////////////////////

    XLEN write (
        unsigned int idx,  // register index
        regSet       set,  // register file
        XLEN         val   // value
    ) {
        switch (set) {
            case GPR:  return std::exchange(gpr[idx], val);
            case PC :  return std::exchange(pc      , val);
        //  case FPR:  return std::exchange(fpr[idx], val);
        //  case VEC:  return std::exchange(vec[idx], val);
            case CSR:  return std::exchange(csr[idx], val);
        }
    };

    XLEN read (
        unsigned int idx,  // register index
        regSet       set   // register file
    ) {
        switch (set) {
            case GPR:  return gpr[idx];
            case PC :  return pc      ;
        //  case FPR:  return fpr[idx];
        //  case VEC:  return vec[idx];
            case CSR:  return csr[idx];
        }
    };

    ///////////////////////////////////////
    // RSP access
    ///////////////////////////////////////

    void set (
        unsigned int idx,  // register index
        XLEN         val   // value
    ) {
             if (idx < 32             )  gpr[idx-0      ] = val;
        else if (idx < 32+1           )  pc               = val;
    //  else if (idx < 32+1+32        )  fpr[idx-32-1   ] = val;
    //  else if (idx < 32+1+32+32     )  vec[idx-32-1-32] = val;
        else if (idx < 32+1+     +2048)  csr[idx-32-1   ] = val;
    };

    XLEN get (
        unsigned int idx   // register index
    ) {
             if (idx < 32             )  return gpr[idx-0      ];
        else if (idx < 32+1           )  return pc              ;
    //  else if (idx < 32+1+32        )  return fpr[idx-32-1   ];
    //  else if (idx < 32+1+32+32     )  return vec[idx-32-1-32];
        else if (idx < 32+1+     +2048)  return csr[idx-32-1   ];
    };

};

///////////////////////////////////////////////////////////////////////////////
// HDLDB shadow class
///////////////////////////////////////////////////////////////////////////////

// 32/64 bit CPU selection
template <typename XLEN, typename FLEN, unsigned int CNUM>

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

    // CPU core architecture
    struct ArchitectureCore {
        // number of all registers
        unsigned int gpr =   32;  // GPR number (use 16 for E extension)
        unsigned int fpr =   32;  // floating point registers number
        unsigned int csr = 4096;  // CSR number  TODO: should be a list of indexes
        // memory map (shadow memory map)
        std::vector<MemoryBlock> map;
    };

    // SoC system architecture
    struct ArchitectureSystem {
        std::array<ArchitectureCore, CNUM> cpu;
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

    ////////////////////////////////////////
    // core shadow
    ////////////////////////////////////////

    // shadow state structure
    struct ShadowCore {
        // register files
        XLEN     pc;      // PC  (program counter)
        XLEN     gpr [];  // GPR (general purpose register file)
        FLEN     fpr [];  // FPR (floating point register file)
        XLEN     csr [];  // CSR (configuration status registers)
        // core local memories (array of address map regions)
        std::vector<Memory>  mem;
        // core local memory mapped I/O registers (covers address space not covered by memories)
        std::map<XLEN, byte> i_o;

        // associative array for per thread hardware breakpoints/watchpoint
        std::map<XLEN, PointTtype> bpt;
        std::map<XLEN, PointTtype> wpt;

        // instruction counter
        size_t     cnt;
        // current retired instruction
        Retired    ret;
        // signal
        int        sig;
        // reason (point type/kind)
        PointTtype rsn;
    };

    ////////////////////////////////////////
    // system shadow
    ////////////////////////////////////////

    struct ShadowSystem {
        // shadow of individual CPU cores
        std::array<ArchitectureCore, CNUM> archCore;

        // system shared memories (array of address map regions)
        std::vector<Memory>  mem;
        // system shared memory mapped I/O registers (covers address space not covered by memories)
        std::map<XLEN, byte> i_o;

        // associative array for all threads hardware breakpoints/watchpoint
        std::map<XLEN, PointTtype> bpt;
        std::map<XLEN, PointTtype> wpt;

        // time 

        // trace queue
        vector<Retired> trc;
    };

    ///////////////////////////////////////
    // architecture and shadow configuration array
    ///////////////////////////////////////

    ArchitectureSystem  arch;
    ShadowSystem        shadow;

public:
    // constructor/destructor
    hdldbShadow (const std::array<ArchitectureCore, CNUM>, ArchitectureSystem);
    ~hdldbShadow ();

//    Rectangle(string name) : m_name(name) {}
//  void set_values (int,int);
//  int area () {return m_width*m_height;}
//  const char* name () {return m_name.c_str();}
};


///////////////////////////////////////////////////////////////////////////////
// constructor/destructor
///////////////////////////////////////////////////////////////////////////////

template <typename XLEN, typename FLEN, unsigned int CNUM>
hdldbShadow<XLEN, FLEN, CNUM>::hdldbShadow (
    const std::array<ArchitectureCore, CNUM>  archCore,
    const            ArchitectureSystem archSystem
) {
    // architecture
    archCore = arch;

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

template <typename XLEN, typename FLEN, unsigned int CNUM>
hdldbShadow<XLEN, FLEN, CNUM>::~hdldbShadow () {
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
