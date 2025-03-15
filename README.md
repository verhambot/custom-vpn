# custom-vpn
A purely demonstrative custom VPN implementation in C

## Setup
The following commands setup a new persistent TUN interface (not persistent across reboots) :
1. `sudo ip tuntap add dev tun0 mode tun`
2. `sudo ip addr add 10.0.0.1/24 dev tun0`
3. `sudo ip link set tun0 up`

## Run
1. Clone the repository : `git clone https://github.com/verhambot/custom-vpn.git`
2. Head into the `custom-vpn` directory : `cd custom-vpn/`
3. Run the `Makefile` : `make`
4. Run the compiled executable : `./vpn-tun`
5. Clean : `make clean`

## References
- https://www.baeldung.com/linux/tun-interface-purpose
- https://piratelearner.com/en/bookmarks/tuntap-interface-tutorial/14/
- https://stackoverflow.com/questions/74538205/how-to-parse-an-ip-packet-stored-in-char-buffer-in-c-c
