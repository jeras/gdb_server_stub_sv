# CC=clang CXX=clang++ CC_LD=lld CXX_LD=lld meson setup builddir

project('hdldb', 'cpp', default_options : ['c_std=c11', 'cpp_std=c++23'])

incdir = include_directories('src/rsp', 'src/shadow', 'src', 'submodules/cxxopts/include/')

executable('test-vector', 'src/tests/test-vector.cpp')

hdldb_sources = [
    'src/hdldb.cpp',
    'src/rsp/Socket.cpp',
    'src/rsp/Packet.cpp',
#    'src/rsp/Protocol.cpp',
#    'src/shadow/Registers.cpp',
#    'src/shadow/MemoryMap.cpp',
#    'src/shadow/Points.cpp',
#    'src/shadow/Core.cpp',
#    'src/shadow/System.cpp'
]

executable('hdldb', sources: hdldb_sources, include_directories : incdir)

test_packet_sources = [
    'src/tests/test-packet.cpp',
    'src/rsp/Socket.cpp',
    'src/rsp/Packet.cpp',
]

executable('test-packet', sources: test_packet_sources, include_directories : incdir)
