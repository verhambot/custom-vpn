# custom-vpn
A purely demonstrative custom VPN implementation in C

## Getting Started
1. Clone the repository : `$ git clone https://github.com/verhambot/custom-vpn.git`
2. Head into the `custom-vpn` directory : `$ cd custom-vpn/`
3. Compile using the `Makefile` : `$ make`
4. Give the binary privileges : `$ sudo setcap cap_net_admin+ep ./bin/vpn`
5. Run the setup scripts :
    - Server : `$ sudo ./server-setup.sh <interface>`
    - Client : `$ sudo ./client-setup.sh <interface> <client_ip>`
6. Clean : `make clean`

## References
- https://www.baeldung.com/linux/tun-interface-purpose
- https://piratelearner.com/en/bookmarks/tuntap-interface-tutorial/14/
- https://stackoverflow.com/questions/74538205/how-to-parse-an-ip-packet-stored-in-char-buffer-in-c-c
