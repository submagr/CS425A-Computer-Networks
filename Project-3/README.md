#Things I learnt from this project

- I have to implement STCP(simplified tcp). MYSOCK is socket layer which is given already. Basic structure of STCP template is also given. 

### UDP (User datagram Protocol):
- Not reliable delievery. 
- No Connection setup delay.

### TCP (Transmission Control Protocol) Specifications:
- Stream of byte service
- Connection Oriented: Maintain some state at both ends. Connection established, Connection closed etc.
- Reliable/Guaranted and in-order delivery: Checksums, Detect loss/redelivery, Acknowledgement/Retransmission etc.
- Flow Control: Prevent receiver buffer overflow 
- Congestion Control 

### TCP Reliable Delivery Challenges: 
- Firstly, How receiver would know about lost packets, i.e. How it will identify this packet should not have come, some packet is missing
- Even if receiver is able to identify some desired packet has not arrived, is it simply lost or reordered ?

### TCP Acknowledgment Mechanism to ensure reliable delivery: 
- Each packet sent by sendor is acknowledged by client. Sender does not send another packet until previous packet is acknowledged. 
	- Acknowledgement from receiver: Ack- Packet received. NACK- Corrupted packet received, retransmit. 
	- Retransmission by sender: After not receiving ACK (based on timeout) or NACK received.
- Reliable Deleivery: 
	- Checksum: For data corruption detection: Send NACK
	- Sequence Numbers: For acknowledging sender about data received(uniquely identifies packages and is shared between peer and host).  Keep track of lost data
- Mechanism: 
	- Sender tells sequence number of first byte in data it is sending.
	- Receiver acknowledges sender by telling the next byte it is expecting (1+ sq no. of last byte received)

### TCP starting and ending a connection: 
- Starting 
	- A send SYN(open) to B with A's ISN
	- B send SYN-ACK to A with B's ISN and Acknowledgement-field=A'ISN+1
	- A send ACK to B with B's ISN+1
- Ending
	- (A)FIN/RST: send remaining bytes and close / reset to close and not receive remaining bytes
	- (B) Ack
	- (B) FIN
	- (A) Ack

### Problem of littile endian and big endian in network data transfer

- **Big Endian** : Normal Order, big end first b8a4 is stored as b8 followed by a4
- **Little Endian**: Devil, Little End first () b8a4 is stored as a4 followed by b8
- **Funda of little vs big end**: By little end, I think they mean lesser significant bits, i.e., bits comming towards right 
- **Problem in Networks**: When networks communicate, it is not neccessary that both machines share same endianess. It is the duty of transport layer to convert bytes in ***Network Byte Order*** *(nice big endian format)*. 
- While programming TCP layer, I don't have to worry about checking host's endianess. Always use a function that does this work accordingly
	- htons: host to network short(2 bytes)
	- htonl: host to network long(4 bytes)
	- Similarily, ntohl, ntohs
	- What is the funda of long vs short, I'm still not clear, When to use them?

### TCP headers:
- th\_off, offset: Offset in multiples of 32 bits at which data starts (should be 5 in our case as no options are being sent by packets) 
- th\_win:  advertised receiver window size in bytes

### Sequence Number: 
- Initial sequence number is chosen randomly. (0-255) 
- Transport layer manages two streams: incoming and outgoing which have different Seq. No.s 

### Advertising Window Concept:
- The amount of data host sending the packet is willing to accept.
- Set of seq number of which reciever is ready to accept data.

### Packet Acknowledgement: 
- 

### Problems I've to solve in this project:
- Packet formation of data coming from socket layer. Remember to use ntohs etc. to handle multibyte multi-byte fields of header.
- Parsing TCP header in packet

### Summary Project 3 Spec:
- STCP Packet Format: 
- Seq Numbers: 
- Data Packets: 
- ACK Packets: 
- Sliding Window: 
- Options 
- Retransmission 
- Network Initiation
- Network Termination 
- Transport Layer Interface:  
- Network Layer Interface: stcp\_api
- Application Layer Interface: stcp\_api

