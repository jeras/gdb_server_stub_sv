target extended-remote gdb_server_stub_socket
monitor reset assert
load
monitor reset release
stepi 4
info registers
stepi 4
info registers
reverse-stepi 4
info registers
stepi 4
info registers
stepi 4
info registers
hbreak *0x40
continue
info registers
hbreak *0x10
reverse-continue
info registers
detach