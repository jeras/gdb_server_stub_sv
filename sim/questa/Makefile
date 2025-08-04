################################################################################
# DUT, TOP and source files
################################################################################

# design under test, must be provided as environment variable
DUT ?= nerv

# top level file (an alternative TOP can be provided as environment variable)
TOP ?= ${DUT}_tb

# include source file list in ${HDL} variable
include ../sources-${DUT}.mk

################################################################################
# tool specific flags
################################################################################

# TODO: add comment regarding this flag.
# I think it is suppressing errors on clock transition at time 0.
FLAGS  = -suppress 13178
# suppress error caused by mixing combinational/sequential elements in the same unpacked array
FLAGS += -suppress 7033
# disable optimizations
FLAGS += -voptargs=+acc

################################################################################
# Verilog define macros
################################################################################

# example
# DEF += -defineall macro
# DEF += -defineall macro=value

# define TOOL_* macro (used to handle tool quirks)
DEF = -defineall TOOL_QUESTA

################################################################################
# NERV specific define macros
################################################################################

DEF += -defineall STALL
DEF += -defineall NERV_DBGREGS
#DEF += -defineall NERV_RVFI

################################################################################
# Verilog toplevel parameter override
################################################################################

# example
# PAR += -Gparameter=value

# Unix/TCP socket
SOCKET ?= "gdb_server_stub_socket"
PORT   ?= 0
PAR += -GSOCKET=${SOCKET}

################################################################################
# Verilog plusargs
################################################################################

# example
# ARG += +key
# ARG += +key=value

################################################################################
# targets
################################################################################

# uncomment the GUI target for debugging purposes
#all: gui
all: sim

sim: ${HDL} ${SRC}
	qrun -makelib work -sv ${HDL} -c ${SRC} -end ${DEF} ${PAR} ${FLAGS} ${ARG} -top ${TOP}

gui: ${HDL} ${SRC}
	qrun -makelib work -sv ${HDL} -c ${SRC} -end ${DEF} ${PAR} ${FLAGS} ${ARG} -top ${TOP} -gui
