///////////////////////////////////////////////////////////////////////////////
// HDLDB shadow (DUT shadow copy)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

#include "Shadow.h"

namespace hdldb {

///////////////////////////////////////////////////////////////////////////////
// HDLDB registers class
///////////////////////////////////////////////////////////////////////////////

template <typename XLEN, typename FLEN, typename VLEN, bool extE, bool extF, bool extV>
XLEN RegistersRiscV<XLEN, FLEN, VLEN, extE, extF, extV>::write (
    const RegisterEnum set,  // register file
    const unsigned int idx,  // register index
    const XLEN         val   // value
) {
    switch (set) {
        case GPR:  return std::exchange(m_gpr[idx], val);
        case PC :  return std::exchange(m_pc      , val);
        case FPR:  return std::exchange(m_fpr[idx], val);
    //  case VEC:  return std::exchange(m_vec[idx], val);
        case CSR:  return std::exchange(m_csr[idx], val);
    };
};

template <typename XLEN, typename FLEN, typename VLEN, bool extE, bool extF, bool extV>
XLEN RegistersRiscV<XLEN, FLEN, VLEN, extE, extF, extV>::read (
    const RegisterEnum set,  // register file
    const unsigned int idx   // register index
) const {
    switch (set) {
        case GPR:  return m_gpr[idx];
        case PC :  return m_pc      ;
    //  case FPR:  return m_fpr[idx];
    //  case VEC:  return m_vec[idx];
        case CSR:  return m_csr[idx];
    }
};

template <typename XLEN, typename FLEN, typename VLEN, bool extE, bool extF, bool extV>
void RegistersRiscV<XLEN, FLEN, VLEN, extE, extF, extV>::set (
    const unsigned int idx,  // register index
    const XLEN         val   // value
) {
         if (idx < 32             )  m_gpr[idx-0      ] = val;
    else if (idx < 32+1           )  m_pc               = val;
//  else if (idx < 32+1+32        )  m_fpr[idx-32-1   ] = val;
//  else if (idx < 32+1+32+32     )  m_vec[idx-32-1-32] = val;
    else if (idx < 32+1+     +2048)  m_csr[idx-32-1   ] = val;
};

template <typename XLEN, typename FLEN, typename VLEN, bool extE, bool extF, bool extV>
XLEN RegistersRiscV<XLEN, FLEN, VLEN, extE, extF, extV>::get (
    const unsigned int idx   // register index
) const {
         if (idx < 32             )  return m_gpr[idx-0      ];
    else if (idx < 32+1           )  return m_pc              ;
//  else if (idx < 32+1+32        )  return m_fpr[idx-32-1   ];
//  else if (idx < 32+1+32+32     )  return m_vec[idx-32-1-32];
    else if (idx < 32+1+     +2048)  return m_csr[idx-32-1   ];
};

///////////////////////////////////////////////////////////////////////////////
// HDLDB memory map class
///////////////////////////////////////////////////////////////////////////////

// read from shadow memory map
template <typename XLEN>
std::vector<std::byte> MemoryMap<XLEN>::read (
    XLEN        addr,
    std::size_t size
) {
    std::vector<std::byte> tmp;
    // reading from an address map block
    for (int unsigned blk=0; blk<MMAP.size(); blk++) {
        if ((addr >= MMAP[blk].base) &&
            (addr <  MMAP[blk].base + MMAP[blk].size)) {
            tmp = {>>{mem[blk] with [adr - MMAP[blk].base +: siz]}};
            return tmp;
        };
    };
    // reading from an unmapped IO region (reads have higher priority)
    // TODO: handle access to nonexistent entries with a warning?
    // TODO: handle access with a size mismatch
    tmp = m_i_o[adr];
    return tmp;
};

// write to shadow memory map
template <typename XLEN>
void MemoryMap<XLEN>::write (
    XLEN                   addr,
    std::vector<std::byte> data
) {
    // writing to an address map block
    for (int unsigned blk=0; blk<MMAP.size; blk++) {
        if ((addr >= MMAP[blk].base) &&
            (addr <  MMAP[blk].base + MMAP[blk].size)) {
            {>>{mem[blk] with [adr - MMAP[blk].base +: dat.size()]}} = dat;
//            for (int unsigned i=0; i<dat.size(); i++) begin: byt
//              mem[blk][adr - MMAP[blk].base] = dat[i];
//            end: byt
            return;
        };
    };
    // writing to an unmapped IO region (reads have higher priority)
    m_i_o[adr] = dat;
};

///////////////////////////////////////////////////////////////////////////////
// HDLDB breakpoint/watchpoint/catchpoint class
///////////////////////////////////////////////////////////////////////////////

// 32/64 bit CPU selection
template <typename XLEN>

class hdldbPoints {

    ///////////////////////////////////////
    // type definitions
    ///////////////////////////////////////

    // point type
    enum class PointType : int {
        swbreak   = 0,  // software breakpoint
        hwbreak   = 1,  // hardware breakpoint
        watch     = 2,  // write  watchpoint
        rwatch    = 3,  // read   watchpoint
        awatch    = 4,  // access watchpoint
        replaylog = 5,  // reached replay log edge
        none      = -1  // no reason is given
    };

    using PointKind = unsigned int;

    struct Point {
        PointType type;
        PointKind kind;
    };

    std::map<XLEN, Point> breakpoints;
    std::map<XLEN, Point> watchpoints;

    ///////////////////////////////////////
    // DUT access
    ///////////////////////////////////////

    ///////////////////////////////////////
    // RSP access
    ///////////////////////////////////////

    unsigned int insert (
        PointType type,
        XLEN      addr,
        PointKind kind
    ) {
        // insert breakpoint/watchpoint into dictionary
        switch (type) {
            case swbreak:
            case hwbreak:
                breakpoints.insert(addr) = {type, kind};
                return breakpoints.size();
            case watch:
            case rwatch:
            case awatch:
                watchpoints.insert(addr) = {type, kind};
                return watchpoints.size();
        };
    };

    unsigned int remove (
        PointType type,
        XLEN      addr,
        PointKind kind
    ) {
        switch (type) {
            case swbreak:
            case hwbreak:
                breakpoints.erase(addr);
                return breakpoints.size();
            case watch:
            case rwatch:
            case awatch:
                watchpoints.erase(addr);
                return watchpoints.size();
        };
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
        RegistersRiscV<XLEN, FLEN> reg;
        // core local memories (array of address map regions)
        std::vector<Memory>  mem;
        // core local memory mapped I/O registers (covers address space not covered by memories)
        std::map<XLEN, byte> i_o;

        // associative array for per thread hardware breakpoints/watchpoint
        hdldbPoints<XLEN> breakpoints;
        hdldbPoints<XLEN> watchpoints;

        // instruction counter
        size_t     cnt;
        // current retired instruction
        Retired    ret;
        // signal
        int        signal;
        // reason (point type/kind)
//        hdldbPoints<XLEN>::PointType reason;
        int        reason;
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
        hdldbPoints<XLEN> breakpoints;
        hdldbPoints<XLEN> watchpoints;

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
    // memory read/write
    std::vector<std::byte> memRead (XLEN, int unsigned);
    void                   memWrite(XLEN, std::vector<std::byte>);
    // breakpoint/watchpoint/catchpoint
    bool matchPoint(Retired);
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
        // memories
        for (unsigned int i=0; i<arch[core].map.size(); i++) {
            shadow[core].mem.push_back(new Memory [arch[core].mem[i].size]);
        }
        // instruction counter
        shadow[core].cnt = 0;
        // signal
        shadow[core].signal = SIGTRAP;
        // reason (point type/kind)
        // TODO
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
    };
};

///////////////////////////////////////////////////////////////////////////////
// breakpoint/watchpoint/catchpoint
///////////////////////////////////////////////////////////////////////////////

template <typename XLEN, typename FLEN, unsigned int CNUM>
bool hdldbShadow<XLEN, FLEN, CNUM>::matchPoint (
    Retired ret
) {
    constexpr std::array<std::byte, 4>   ebreak = {std::byte{0x73}, std::byte{0x00}, std::byte{0x10}, std::byte{0x00}};  // 32'h00100073
    constexpr std::array<std::byte, 2> c_ebreak = {std::byte{0x02}, std::byte{0x90}};                                    // 16'h9002

    XLEN addr;

                             addr = ret.ifu.adr;
    std::array<std::byte, 4> inst = ret.ifu.rdt;
    // match illegal instruction
    if (ret.ifu.ill) {
        shadow.signal = SIGILL;
//            debug("DEBUG: Triggered illegal instruction at address %h.", addr);
        return true;
    }

    // match software breakpoint
    // match EBREAK/C.EBREAK instruction
    // TODO: there are also explicit SW breakpoints that depend on ILEN
    else if (std::equal(  ebreak.begin(),   ebreak.end(), inst) ||
             std::equal(c_ebreak.begin(), c_ebreak.end(), inst)) {              
        shadow.signal = SIGTRAP;
//            $display("DEBUG: Triggered SW breakpoint at address %h.", addr);
        return true;
    }

    // match hardware breakpoint
    else if (shadow.breakpoints.contains(addr)) {
        if (shadow.breakpoints[addr].type == shadow.breakpoints::hwbreak) {
            // signal
            shadow.signal = SIGTRAP;
            // reason
//            reason = shadow.breakpoints[addr].type;
            shadow.reason = 3;
//            $display("DEBUG: Triggered HW breakpoint at address %h.", addr);
            return true;
        };
    };

         addr = ret.lsu.adr;
    bool rena = ret.lsu.rdt.size() > 0;
    bool wena = ret.lsu.wdt.size() > 0;

    // match hardware breakpoint
    if (shadow.watchpoints.contains(addr)) {
        if (((shadow.watchpoints[addr].ptype == shadow.watchpoints::watch ) && wena) ||
            ((shadow.watchpoints[addr].ptype == shadow.watchpoints::rwatch) && rena) ||
            ((shadow.watchpoints[addr].ptype == shadow.watchpoints::awatch) )) {
            // TODO: check is transfer size matches
            // signal
            shadow.signal = SIGTRAP;
            // reason
            shadow.reason = shadow.watchpoints[addr].type;
//            $display("DEBUG: Triggered HW watchpoint at address %h.", addr);
            return true;
        };
    };
};

// end of hdldb namespace
};
