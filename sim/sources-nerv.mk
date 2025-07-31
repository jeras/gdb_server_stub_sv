################################################################################
# HDL source files
################################################################################


# GDB stub files
PATH_GDB=../../hdl

# DPI-C code
SRC+=${PATH_GDB}/socket_dpi_pkg.c

# SystemVerilog bench (Test SV)
TSV+=${PATH_GDB}/socket_dpi_pkg.sv
TSV+=${PATH_GDB}/gdb_shadow_pkg.sv
TSV+=${PATH_GDB}/gdb_server_stub_pkg.sv

# NERV files
PATH_NERV=../../submodules/nerv

# SystemVerilog RTL
RTL+=${PATH_NERV}/nerv.sv

# SoC files
RTL+=${PATH_GDB}/nerv/nerv_soc.sv

# SystemVerilog bench (Test SV)
TSV+=${PATH_GDB}/nerv/nerv_gdb.sv
TSV+=${PATH_GDB}/nerv/nerv_tb.sv

# combined HDL sources
HDL =${RTL}
HDL+=${TSV}

