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
    while(if_walker->next){
		if(if_walker->ip == ip){
		  return if_walker;
		}
        if_walker = if_walker->next; 
    }
	return 0;
}

/* See pseudo-code in sr_arpcache.h */
void handle_arpreq(struct sr_instance* sr, struct sr_arpreq *req){
  /* TODO: Fill this in */
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

  printf("*** -> Received packet of length %d\n",len);
  sr_ethernet_hdr_t* ethHdr = packet;
  if(len < sizeof(sr_ethernet_hdr_t)){
	printf("Packet too small to be of ethernet type\n");
	return;
  }
  if(ethHdr->ether_type == ethertype_arp){ /*ARP*/
	sr_arp_hdr_t* arpHdr = ethHdr+sizeof(sr_ethernet_hdr_t);
	/*Packet Length Check*/
	if(len-sizeof(sr_ethernet_hdr_t) < sizeof(sr_arp_hdr_t)){
	  printf("Packet too small to be of ARP type\n");
	  return;
	}
	/*TODO: Checksum*/
	if(arpHdr->ar_op == arp_op_request){ /*ARP Request*/
	  /*Check destination ip in ip list*/  
	  struct sr_if* matchInterface = ip_in_if_list(sr, arpHdr->ar_tip);
	  if(matchInterface){
		/*Send arp reply*/
		int ethReplyLen = sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t);
		sr_ethernet_hdr_t* ethReply = (sr_ethernet_hdr_t*)malloc(ethReplyLen);
		memcpy(ethReply, ethHdr, ethReplyLen); /*Copy received packet*/
		printf("Copy of Packet received:\n");
		print_hdrs(ethReply, ethReplyLen);

		memcpy(ethReply->ether_dhost, ethHdr->ether_shost, ETHER_ADDR_LEN);
		memcpy(ethReply->ether_shost, matchInterface->addr, ETHER_ADDR_LEN);

		sr_arp_hdr_t* arpReply = ethReply + sizeof(sr_arp_hdr_t);
		arpReply->ar_op = arp_op_reply;
		memcpy(arpReply->ar_tha, arpHdr->ar_sha, ETHER_ADDR_LEN);
		memcpy(arpReply->ar_sha, matchInterface->addr, ETHER_ADDR_LEN);
		arpReply->ar_tip = arpHdr->ar_sip;
		arpReply->ar_sip = matchInterface->ip;

		printf("Sending Packet:\n");
		print_hdrs(ethReply, ethReplyLen);
		sr_send_packet(sr, ethReply, ethReplyLen, matchInterface->name);
	  }else{
		Debug("IP not found for arpr packet in if list");
		return;
	  }
	}else if(arpHdr->ar_op == arp_op_reply){ /*ARP Reply*/

	}
  }else if(ethHdr->ether_type == ethertype_ip){ /*IP*/

  }else{ /*Unknown packet*/
	printf("Unknown ethertype\n");
	return;
  }
 
  

}/* -- sr_handlepacket -- */

