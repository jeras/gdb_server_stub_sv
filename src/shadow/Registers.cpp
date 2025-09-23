///////////////////////////////////////////////////////////////////////////////
// HDLDB DUT shadow registers
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// HDLDB includes
#include "Registers.h"

namespace HdlDb {

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

};
