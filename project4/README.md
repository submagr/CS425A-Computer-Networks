#Project-4: My own internet router

- To write a simple router with static forwarding table. My job is to write proper logic so that packet go to correct destination
- To things to handle in this assignment: ARP and IP forwarding

### Ethernet
- Link Layer Protocol
- Family of computer networking technologies commonly used in LAN 
- Raw ethernet frame may be an IP frame or ARP frame

### IP Forwarding
- Sanity check the packet(minimum length and checksum). If not matched, drop the packet
- Decrement the TTL by 1. Recompute the checksum 
- Find out which entry in the routing table has the longest prefix match with the destined IP address 
- Check the ARP cache for next hop mac address corresponding to next hop IP. 
	- If it's there, send it.
	- Else, send an ARP request for the next hop IP and add the packet to the ARPWaitingQueue

### ARP Protocol	
- Check if it is the request for the router's MAC Address. Send ARP Reply 
- Check if it a reply for the request, the router previously sent. Cache the reply and forward the IP packets waiting for this reply
- Cache entries should time out after 15 seconds

### ICMP - Internet Control Message Protocol
- It is used by network devices like routers to send error messages 
- Not used to exchange data. Also not very regularly employed.
- Echo Reply:
- Not entirely clear. Look Again

### About Mininet
- Mininet is a network emulator which create a network of virtual hosts

### Checksum
- It is a block of data used to detect corruption introduced while storage or transmission 
- Simple checksum algorithm: *Parity byte or parity word*
  -	Divide data into words of n bits each. Compute xor of these words and append it to data 
  - While checking, again divide data into words of n bits and take xor. If result is not 0, data is corrupted

### XOR function
- same => 0
- 0 0 => 0, 1 1 => 0
- 0 1 => 1, 1 0 => 1
