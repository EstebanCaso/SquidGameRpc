## Dependencias (Arch Linux)
```bash
sudo pacman -S libtirpc rpcsvc-proto gcc make
```

## Compilar
```bash
rpcgen squid.x
make all
```

## Correr servidor
```bash
./squid_server
```

## Conectarse como jugador
```bash
./squid_client <IP_del_servidor> <puente 1-3> <tu_nombre>
```

### Ejemplo
```bash
./squid_client 192.168.1.105 1 Alice
./squid_client 192.168.1.105 2 Bob
./squid_client 192.168.1.105 3 Carlos
```
