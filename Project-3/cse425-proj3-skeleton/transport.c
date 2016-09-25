/*
 * transport.c 
 *
 *	Project 3		
 *
 * This file implements the STCP layer that sits between the
 * mysocket and network layers. You are required to fill in the STCP
 * functionality in this file. 
 *
 */


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>
#include "mysock.h"
#include "stcp_api.h"
#include "transport.h"
#include "network_io.h"
#include <limits.h>


enum { 
	CSTATE_ESTABLISHED, 
	CSTATE_FIN, 
	CSTATE_FINACK
};    /* you should have more states */


/* this structure is global to a mysocket descriptor */
typedef struct
{
    bool_t done;    /* TRUE once connection is closed */

    int connection_state;   /* state of the connection (established, etc.) */
    tcp_seq initial_sequence_num;

    /* any other connection-wide global variables go here */
	tcp_seq y_init_seq; // sender's initial sequence number

	int rem_win_size;
	int unack_bytes;
	tcp_seq sent_seq;
	int fin;
} context_t;


static void generate_initial_seq_num(context_t *ctx);
static void control_loop(mysocket_t sd, context_t *ctx);

void our_dprintf(const char *format,...);

/* initialise the transport layer, and start the main loop, handling
 * any data from the peer or the application.  this function should not
 * return until the connection is closed.
 */
void transport_init(mysocket_t sd, bool_t is_active)
{
    context_t *ctx;

    ctx = (context_t *) calloc(1, sizeof(context_t));
    assert(ctx);

    generate_initial_seq_num(ctx);
	ctx->sent_seq = ctx->initial_sequence_num - 1;
	ctx->rem_win_size = TH_Initial_Win; // Set Initial window size
	ctx->fin = 0; // 0 fin exchanged

    /* XXX: you should send a SYN packet here if is_active, or wait for one
     * to arrive if !is_active.  after the handshake completes, unblock the
     * application with stcp_unblock_application(sd).  you may also use
     * this to communicate an error condition back to the application, e.g.
     * if connection fails; to do so, just set errno appropriately (e.g. to
     * ECONNREFUSED, etc.) before calling the function.
     */
	if(is_active){
		// create a tcp syn packet
		our_dprintf("ACTIVE: \n");
		STCPHeader* initHeader = (STCPHeader*) malloc(sizeof(STCPHeader));
		initHeader->th_seq	= ctx->initial_sequence_num;
		initHeader->th_flags = TH_SYN;
		initHeader->th_off = 5;
		initHeader->th_win = TH_Initial_Win;
		// send packet 
		stcp_network_send(sd, initHeader, sizeof(STCPHeader), NULL );
		our_dprintf("SYN packet sent, wait for synAck to arrive");
		// wait for syn-ack to arrive
		unsigned int flag = stcp_wait_for_event(sd, NETWORK_DATA, NULL);
		our_dprintf("SYN-ACK arrived");
		if(flag == NETWORK_DATA){
			// Read headers
			// Set initial seq number of sender in context
			char buffer[maxBufferSize];
			stcp_network_recv(sd, buffer, maxBufferSize);
			STCPHeader* synAckHeader = (STCPHeader*)buffer;
			//assert(synAckHeader->th_flags == (TH_SYN|TH_ACK));
			//assert(ctx->initial_sequence_num+1 == synAckHeader->th_ack);
			ctx->y_init_seq = synAckHeader->th_seq;
			//ctx->y_ack_seq = synAckHeader->th_seq+1;
			
			// #send ack back 
			// ##create a tcp ack packet
			our_dprintf("Sending ack packet\n");
			STCPHeader* ackHeader = (STCPHeader*) malloc(sizeof(STCPHeader));
			ackHeader->th_seq	= ctx->initial_sequence_num;
			ackHeader->th_ack	= ctx->y_init_seq+1;
			ackHeader->th_flags = TH_ACK;
			ackHeader->th_off = 5;
			ackHeader->th_win = TH_Initial_Win;
			// ##send packet 
			our_dprintf("Sending Ack\n");
			int sent = stcp_network_send(sd, ackHeader, sizeof(STCPHeader), NULL);
			our_dprintf("Ack Sent\n");
			//TODO check sent here
			// unblock the application 
		}else{
			//TODO Handle errors
		}
	}else{
		// wait for syn packet to arrive 
		our_dprintf("PASSIVE\n");
		unsigned int flag = stcp_wait_for_event(sd, NETWORK_DATA, NULL );
		our_dprintf("SYN Packet arrived\n");
		if(flag == NETWORK_DATA){
			// Read headers
			// Set initial seq number of sender in context
			our_dprintf("flag = Network data\n");
			char buffer[maxBufferSize];
			stcp_network_recv(sd, buffer, maxBufferSize);
			our_dprintf("Read data from network layer\n");
			STCPHeader* synHeader = (STCPHeader*)buffer;
			assert(synHeader->th_flags == TH_SYN);
			ctx->y_init_seq = synHeader->th_seq;	
			our_dprintf("Syn Received\n");
			// #send syn-ack back 
			// ##create a tcp syn-ack packet
			our_dprintf("SENDING SYN ACK\n");
			STCPHeader* synAckHeader = (STCPHeader*) malloc(sizeof(STCPHeader));
			synAckHeader->th_seq	= ctx->initial_sequence_num;
			synAckHeader->th_ack = ctx->y_init_seq+1;
			synAckHeader->th_flags = TH_SYN|TH_ACK;
			synAckHeader->th_off = 5;
			synAckHeader->th_win = TH_Initial_Win;
			// ##send synack packet 
			int sent = stcp_network_send(sd, synAckHeader, sizeof(STCPHeader), NULL );
			//TODO check sent here
			// #receive ack packet
			our_dprintf("WAITING FOR ACK PACKET TO ARRIVE");
			flag = stcp_wait_for_event(sd, NETWORK_DATA, NULL );
			if(flag == NETWORK_DATA){
				// Read headers
				memset(buffer, '\0', sizeof(buffer));
				stcp_network_recv(sd, buffer, maxBufferSize);
				STCPHeader* ackHeader = (STCPHeader*)buffer;
				our_dprintf("ACK ARRIVED\n");
				//assert(ackHeader->th_flags == TH_ACK);
				//assert(ackHeader->th_seq == ctx->initial_sequence_num+1);
			}else{
				our_dprintf("SOME ERROR INSTEAD OF ACK ARRIVED\n");
				//TODO handle ack packet not received here.
			}
			// unblock the application 
		}else{
			our_dprintf("SOME ERROR INSTEAD OF SYN\n");
			//TODO Handle errors
		}
	}
    ctx->connection_state = CSTATE_ESTABLISHED;
    stcp_unblock_application(sd);

    control_loop(sd, ctx);

    /* do any cleanup here */
    free(ctx);
}


/* generate random initial sequence number for an STCP connection */
static void generate_initial_seq_num(context_t *ctx)
{
    assert(ctx);

#ifdef FIXED_INITNUM
    /* please don't change this! */
    ctx->initial_sequence_num = 1;
#else
    /* you have to fill this up */
    /*ctx->initial_sequence_num =;*/
#endif
}


/* control_loop() is the main STCP loop; it repeatedly waits for one of the
 * following to happen:
 *   - incoming data from the peer
 *   - new data from the application (via mywrite())
 *   - the socket to be closed (via myclose())
 *   - a timeout
 */

static void control_loop(mysocket_t sd, context_t *ctx)
{
    assert(ctx);
    assert(!ctx->done);

	char buffer[maxBufferSize];
	unsigned int desired_event=NETWORK_DATA|APP_DATA|APP_CLOSE_REQUESTED;
    while (!ctx->done)
    {
        unsigned int event;

        /* see stcp_api.h or stcp_api.c for details of this function */
        /* XXX: you will need to change some of these arguments! */
		our_dprintf("waiting for some event \n");
        event = stcp_wait_for_event(sd, desired_event, NULL);

        /* check whether it was the network, app, or a close request */
		bzero(buffer, maxBufferSize);
        if (event & APP_DATA)
        {
            /* the application has requested that data be sent */
            /* see stcp_app_recv() */
			//Get data from application	
			//Prepare Header for data to be sent
			//Check available window. If data to be sent > window size
			//send data
			STCPHeader* dataPacket = (STCPHeader*) malloc(sizeof(STCPHeader));
			dataPacket->th_flags = 0; // Payload packet, no flags
			dataPacket->th_ack	= 0; // No acknowledgement, payload packet
			dataPacket->th_seq	= ctx->sent_seq+1;
			dataPacket->th_off = 5;
			dataPacket->th_win = 0; // No window size, payload packet
			assert(ctx->rem_win_size >= ctx->unack_bytes);
			our_dprintf("stcp_app_recv: %d %d",ctx->rem_win_size, ctx->unack_bytes);
			int appData = stcp_app_recv(sd, buffer, MIN(MAX_IP_PAYLOAD_LEN - sizeof(STCPHeader),ctx->rem_win_size - ctx->unack_bytes));
			our_dprintf("Data from app received, Size-%d\n------------\n%s\n-----------\n", appData, buffer);
			if(appData>0){
				int sent = stcp_network_send(sd, dataPacket, sizeof(STCPHeader), buffer, appData, NULL);
				assert(sent == appData + sizeof(STCPHeader)); // Assuming network layer is reliable
				our_dprintf("Data %d sent to network\n", sent);
				ctx->sent_seq = ctx->sent_seq + appData;
				ctx->unack_bytes = ctx->unack_bytes + appData;
			}else{
				desired_event = NETWORK_DATA;
				continue;
			}
        }else if(event & NETWORK_DATA){
			int networkData = stcp_network_recv(sd, buffer, maxBufferSize);
			our_dprintf("Data(%d) from network received\n", networkData, buffer);
			STCPHeader* packet = (STCPHeader *)buffer;
			//TODO: Handle this below other flags may come with TH_ACK
			if(packet->th_flags == TH_ACK){
				ctx->rem_win_size = packet->th_win;
				ctx->unack_bytes = ctx->sent_seq - packet->th_ack + 1;
				our_dprintf("\n::::::::::ACK Received: th_win = %d, ctx->sent_seq = %d, th_ack = %d, rem_win_size = %d, unack_bytes = %d\n", packet->th_win, ctx->sent_seq, packet->th_ack, ctx->rem_win_size);
			}else if(packet->th_flags == TH_FIN){
				our_dprintf("Fin Packet Received\n");
				// send FIN-ACK
				STCPHeader* finAckPacket = (STCPHeader*)malloc(sizeof(STCPHeader));
				finAckPacket->th_flags   = TH_FIN|TH_ACK; 
				finAckPacket->th_ack	 = 0; 
				finAckPacket->th_seq	 = 0;
				finAckPacket->th_off = 5;
				finAckPacket->th_win = 0; 
				stcp_network_send(sd, finAckPacket, sizeof(STCPHeader), NULL);
				our_dprintf("Fin-Ack Packet Sent\n");
				ctx->fin++;
				if(ctx->fin>=2){
					ctx->done = TRUE;
					break;
				}
				stcp_fin_received(sd);
				continue;
			}else if(packet->th_flags == (TH_FIN|TH_ACK)){
				our_dprintf("Fin Ack received\n");
				ctx->fin++;
				if(ctx->fin>=2){
					ctx->done = TRUE;
					break;
				}
				continue;
			}
			int offSet = packet->th_off * 4; 
			if(strlen(buffer+offSet)>0){
				stcp_app_send(sd, buffer+offSet, strlen(buffer+offSet));
				our_dprintf("payload data(%d) sent to app \n---------\n%s\n--------\n", strlen(buffer+offSet), buffer+offSet);
				// send acknowledgement of this data for other side
				STCPHeader* ackPacket = (STCPHeader*)malloc(sizeof(STCPHeader));
				ackPacket->th_flags = TH_ACK; 
				ackPacket->th_ack	= packet->th_seq + strlen(buffer+offSet) + 1;
				ackPacket->th_seq	= ctx->sent_seq+1; // No use, should send or not
				ackPacket->th_off = 5;
				ackPacket->th_win = TH_Initial_Win; 
				int sent = stcp_network_send(sd, ackPacket, sizeof(STCPHeader), NULL);
				our_dprintf("Ack sent\n");
			}
		}else if(event & APP_CLOSE_REQUESTED){
			our_dprintf("App close demanded\n");
			// send FIN
			STCPHeader* finPacket = (STCPHeader*)malloc(sizeof(STCPHeader));
			finPacket->th_flags = TH_FIN; 
			finPacket->th_ack	= 0; 
			finPacket->th_seq	= 0;
			finPacket->th_off = 5;
			finPacket->th_win = 0; 
			stcp_network_send(sd, finPacket, sizeof(STCPHeader), NULL);
			printf("Fin Packet Sent");
		}
		our_dprintf("++++++++++++++++++++One Event Completed++++++++++++++++++++\n");
		desired_event=NETWORK_DATA|APP_DATA|APP_CLOSE_REQUESTED;
        /* etc. */
    }
	our_dprintf("Connection Closed\n");
}


/**********************************************************************/
/* our_dprintf
 *
 * Send a formatted message to stdout.
 * 
 * format               A printf-style format string.
 *
 * This function is equivalent to a printf, but may be
 * changed to log errors to a file if desired.
 *
 * Calls to this function are generated by the dprintf amd
 * dperror macros in transport.h
 */
void our_dprintf(const char *format,...)
{
    va_list argptr;
    char buffer[1024];

    assert(format);
    va_start(argptr, format);
    vsnprintf(buffer, sizeof(buffer), format, argptr);
    va_end(argptr);
    fputs(buffer, stdout);
    fflush(stdout);
}



