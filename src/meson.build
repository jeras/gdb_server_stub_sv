# CC=clang CXX=clang++ CC_LD=lld CXX_LD=lld meson setup build

project('hdldb', 'cpp', default_options : ['c_std=c11', 'cpp_std=c++23'])

executable('test-vector', 'tests/test-vector.cpp')

hdldb_sources = [
    'hdldb.cpp',
    'rsp/Socket.cpp',
    'rsp/Packet.cpp',
    'rsp/Protocol.cpp',
    'shadow/Registers.cpp',
    'shadow/MemoryMap.cpp',
    'shadow/Points.cpp',
    'shadow/Core.cpp',
    'shadow/System.cpp'
]
hdldb_incdir = include_directories('rsp', 'shadow')

executable('hdldb', sources: hdldb_sources, include_directories : hdldb_incdir)
