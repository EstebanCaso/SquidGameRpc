sudo pacman -S libtirpc rpcsvc - proto gcc make rpcbind
sudo systemctl enable -- now rpcbind

# Generate stubs and header from service definition
rpcgen squid . x

# Compile server and client
make all

# Terminal 1 -- server
./ squid_server
# Terminal 2 , 3 , 4 -- clients
./ squid_client localhost 1 Alice
./ squid_client localhost 2 Bob
./ squid_client localhost 3 Carlos
