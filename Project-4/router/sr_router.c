/**********************************************************************
 * file:  sr_router.c
 * date:  Mon Feb 18 12:50:42 PST 2002
 * Contact: casado@stanford.edu
 *
 * Description:
 *
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"

/* TODO: Add constant definitions here... */

/* Helper functions */

/* Takes ip and if list. Returns interface ptr if ip matches
 * */
struct sr_if* ipInIfList(struct sr_instance* sr, uint32_t ip){
    struct sr_if* if_walker = 0;
    if(sr->if_list == 0){
        printf(" Interface list empty \n");
        return;
    }

    if_walker = sr->if_list;
    while(if_walker){
	  if(if_walker->ip == ip){
		return if_walker;
	  }
	  if_walker = if_walker->next; 
    }
	return 0;
}

int longestPrefixMatchHelper(struct sr_rt* rtEntry, uint32_t ip){
  uint32_t prefix = (rtEntry->mask.s_addr)&(rtEntry->dest.s_addr);
  int bitsMatched = 0;
  int bitPosition = 31;
  while(bitPosition+1){
	if((ip&((uint32_t)(1<<bitPosition))) == (prefix&((uint32_t)(1<<bitPosition)))){
	  bitsMatched+=1;
	  bitPosition-=1;
	}else{
	  break;
	}
  }
  return bitsMatched;
}

struct sr_rt* longestPrefixMatch(struct sr_rt* routing_table, uint32_t ip){ 
  int maxBytesMatched = 0;
  struct sr_rt* bestMatchedEntry;
  if(routing_table == 0){
	Debug("Routing Table empty\n");
	return 0;
  }
  struct sr_rt* rtEntry = routing_table;
  while(rtEntry){
	int bytesMatched = longestPrefixMatchHelper(rtEntry, ip);
	if(bytesMatched > maxBytesMatched){
	  maxBytesMatched = bytesMatched;
	  bestMatchedEntry = rtEntry;
	}
	rtEntry = rtEntry->next;
  }
  if(maxBytesMatched)
	return bestMatchedEntry;
  else
	return 0;
}

void fill_arpHdr(sr_arp_hdr_t* arpHdr, enum sr_arp_opcode op, uint8_t* buff_sha, uint8_t* buff_tha, uint32_t sip, uint32_t tip){
  arpHdr->ar_hrd = htons(arp_hrd_ethernet); 
  arpHdr->ar_pro = htons(arp_pro_IPV4);
  arpHdr->ar_hln = ETHER_ADDR_LEN;
  arpHdr->ar_pln = 4;  
  arpHdr->ar_op = htons(op);
  memcpy(arpHdr->ar_sha, buff_sha, ETHER_ADDR_LEN);
  memcpy(arpHdr->ar_tha, buff_tha, ETHER_ADDR_LEN);
  arpHdr->ar_sip = htonl(sip);
  arpHdr->ar_tip = htonl(tip);
  return;
}

void send_arpReq(struct sr_instance* sr, struct sr_arpreq* req){
  struct sr_rt* match = longestPrefixMatch(sr->routing_table, req->ip);
  struct sr_if* matchInterface = sr_get_interface(sr, match->interface) ;
  uint32_t tip = ntohl(req->ip);
  uint32_t sip = ntohl(matchInterface->ip);
  uint8_t* buff_sha = (uint8_t*)malloc(ETHER_ADDR_LEN*sizeof(uint8_t));/*Broadcast*/
  uint8_t* buff_tha = (uint8_t*)malloc(ETHER_ADDR_LEN*sizeof(uint8_t));/*Broadcast*/
  memcpy(buff_sha, matchInterface->addr, ETHER_ADDR_LEN);
  memset(buff_tha, 0xFF, ETHER_ADDR_LEN);

  /*Prepare packet*/
  int lenPacket = sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t);
  uint8_t* buff = (uint8_t*)malloc(lenPacket);
  /*Ethernet header*/
  sr_ethernet_hdr_t* ethHdr = (sr_ethernet_hdr_t*)buff;
  memcpy(ethHdr->ether_dhost, buff_tha, ETHER_ADDR_LEN);
  memcpy(ethHdr->ether_shost, buff_sha, ETHER_ADDR_LEN);
  ethHdr->ether_type = htons(ethertype_arp);
  /*arp header*/
  sr_arp_hdr_t* arpHdr = (sr_arp_hdr_t*)(buff+sizeof(sr_ethernet_hdr_t));
  fill_arpHdr(arpHdr, arp_op_request, buff_sha, buff_tha, sip, tip);
  /*send packet*/
  print_hdrs(buff, lenPacket);
  sr_send_packet(sr, buff, lenPacket, matchInterface->name);
  return;
}

/* See pseudo-code in sr_arpcache.h */
void handle_arpreq(struct sr_instance* sr, struct sr_arpreq* req){
  time_t now;
  time(&now);
  if(difftime(now, req->sent) > 1.0){
	if(req->times_sent >= 5){
	  Debug("Request sent more than 5 times\n");
	  /*icmp error => destroy req*/
	}else{
	  /**send arp request**/
	  Debug("Sending Arp request\n");
	  send_arpReq(sr, req);
	  req->sent = now;
	  req->times_sent += 1;
	}
  }
}

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{
    /* REQUIRES */
    assert(sr);

    /* Initialize cache and cache cleanup thread */
    sr_arpcache_init(&(sr->cache));

    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t thread;

    pthread_create(&thread, &(sr->attr), sr_arpcache_timeout, sr);
    
    /* TODO: (opt) Add initialization code here */
} /* -- sr_init -- */

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT free either (signified by "lent" comment).  
 * Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr,
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */){

  /* REQUIRES */
  assert(sr);
  assert(packet);
  assert(interface);
  uint8_t* cpyPacket = (uint8_t*)malloc(len);
  memcpy(cpyPacket, packet, len);
  printf("*** -> Received packet of length %d\n",len);
  sr_ethernet_hdr_t* ethHdr = packet;
  if(len < sizeof(sr_ethernet_hdr_t)){
	printf("Packet too small to be of ethernet type\n");
	return;
  }
  uint16_t packetType = ntohs(ethHdr->ether_type);
  if(packetType == ethertype_arp){ /*ARP*/
	Debug("Arp Packet Recieved\n");
	sr_arp_hdr_t* arpHdr = packet+sizeof(sr_ethernet_hdr_t);
	/*Packet Length Check*/
	if(len-sizeof(sr_ethernet_hdr_t) < sizeof(sr_arp_hdr_t)){
	  printf("Packet too small to be of ARP type\n");
	  return;
	}
	/*TODO: Checksum*/
	if(ntohs(arpHdr->ar_op) == arp_op_request){ /*ARP Request*/
	  Debug("Arp packet type: Request\n");
	  /*Check destination ip in ip list*/  
	  struct sr_if* matchInterface = ipInIfList(sr, arpHdr->ar_tip);
	  if(matchInterface){
		/*Send arp reply*/
		int ethReplyLen = sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t);
		sr_ethernet_hdr_t* ethReply = (sr_ethernet_hdr_t*)malloc(ethReplyLen);
		memcpy(ethReply, ethHdr, ethReplyLen); /*Copy received packet*/

		memcpy(ethReply->ether_dhost, ethHdr->ether_shost, ETHER_ADDR_LEN);
		memcpy(ethReply->ether_shost, matchInterface->addr, ETHER_ADDR_LEN);

		sr_arp_hdr_t* arpReply = (sr_arp_hdr_t*)((uint8_t*)ethReply + sizeof(sr_ethernet_hdr_t));
		arpReply->ar_op = ntohs(arp_op_reply);
		memcpy(arpReply->ar_tha, arpHdr->ar_sha, ETHER_ADDR_LEN);
		memcpy(arpReply->ar_sha, matchInterface->addr, ETHER_ADDR_LEN);
		arpReply->ar_tip = arpHdr->ar_sip;
		arpReply->ar_sip = matchInterface->ip;

		sr_send_packet(sr, ethReply, ethReplyLen, matchInterface->name);
		Debug("Arp Reply Sent\n");
	  }else{
		Debug("IP not found for arpr packet in if list\n");
		return;
	  }
	}else if(arpHdr->ar_op == arp_op_reply){ /*ARP Reply*/
	  Debug("ARP Reply\n");  
	}else{
	  Debug("Unknown arp packet type\n");  
	}
  }else if(packetType == ethertype_ip){ /*IP*/
	Debug("IP Packet Received\n");
	print_hdrs(packet, len);
	if(sizeof(sr_ip_hdr_t) > len-sizeof(sr_ethernet_hdr_t)){
	  printf("Size insufficient for packet to be IP packet\n");
	  return;
	}
	sr_ip_hdr_t* ipHdr = (sr_ip_hdr_t*)(cpyPacket+sizeof(sr_ethernet_hdr_t));
	/*TODO:Checksum*/
	ipHdr->ip_ttl -= 1;
	/*TODO: Recompute Checksum*/
	/*Check if packet directed for router*/
	struct sr_if* matchInterface = ipInIfList(sr, ipHdr->ip_dst);
	if(matchInterface){ /*Packet for router*/
	  Debug("Packet for router\n");
	}else{ /*Packet to be forwarded*/
	  Debug("Packet to be forwarded\n");
	  struct sr_rt* bestMatchedEntry = longestPrefixMatch(sr->routing_table, ipHdr->ip_dst);
	  if(bestMatchedEntry){ /*Matching entry found in routing table*/
		Debug("Matching entry found in routing table\n");
		uint32_t forwardIp = bestMatchedEntry->dest.s_addr;
		struct sr_arpentry* entry = sr_arpcache_lookup(&(sr->cache), forwardIp);
		if(entry){
		  Debug("IP found in arp cache\n");
		}else{
		  Debug("ip not found in arp cache\n");
		  char* bestMatchedEntryIface = (char*)malloc(sr_IFACE_NAMELEN);
		  memcpy(bestMatchedEntryIface, bestMatchedEntry->interface, sr_IFACE_NAMELEN);
		  struct sr_arpreq* req = sr_arpcache_queuereq(&(sr->cache), forwardIp, cpyPacket, len, bestMatchedEntryIface);
		  handle_arpreq(sr, req);
		}
	  }else{ /*No matching entry found in router*/
		/*TODO: send type 3 code 0 icmp*/
	  }
	}
  }else{ /*Unknown packet*/
	printf("Unknown ethertype\n");
	return;
  }
}/* -- sr_handlepacket -- */

