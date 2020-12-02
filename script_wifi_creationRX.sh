iwconfig wlp3s0 mode ad-hoc
iwconfig wlp3s0 channel 1
iwconfig wlp3s0 essid ifi1
iwconfig wlp3s0 key off
ifconfig wlp3s0 10.0.0.4 netmask 255.255.255.0
ifconfig wlp3s0 10.0.0.4 netmask 255.255.255.255
route add -host 10.0.0.3 dev wlp3s0
echo 0 > /proc/sys/net/ipv4/conf/wlp3s0/send_redirects
route add -host 10.0.0.6 gw 10.0.0.5
echo 1 >  /proc/sys/net/ipv4/ip_forward
route add -host 10.0.0.2 gw 10.0.0.3


