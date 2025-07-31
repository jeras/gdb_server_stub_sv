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
detach