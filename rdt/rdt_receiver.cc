/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or
 *       reordering.  You will need to enhance it to deal with all these
 *       situations.  In this implementation, the packet format is laid out as
 *       the following:
 *
 *       |<-2 byte->|<--4 byte-->|<-1 byte->|<-             the rest            ->|
 *       | checksum | seq_number | length   |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_receiver.h"

#define HEADER_SIZE 7
#define BUFFER_SIZE 128

static packet* buffer; // buffer for disorder
static char* valid_bit;
static int expected_num;

static short getCheckSum(packet* pkt)
{
  int len = pkt->data[HEADER_SIZE - 1] + sizeof(int);
  unsigned int sum = 0;
  int i = 2;
  for(;i < len; i+=2) sum += *((short*)(&pkt->data[i]));
  sum = ~((sum >> 16) + (sum & 0xFFFF));
  return sum;
}

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
    expected_num = 0;
    buffer = (packet*)malloc(sizeof(packet) * BUFFER_SIZE);
    valid_bit = (char*)malloc(BUFFER_SIZE);
    memset(valid_bit, 0, BUFFER_SIZE);
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
    free(buffer);
    free(valid_bit);
}

void response(int ack_num)
{
  //printf("response\n");
  short check_sum = ~((ack_num >> 16)  + (ack_num & 0xFFFF));
  packet recv_pkt;
  memcpy(recv_pkt.data, &check_sum, sizeof(short));
  memcpy(recv_pkt.data + sizeof(short), &ack_num, sizeof(int));
  Receiver_ToLowerLayer(&recv_pkt);
}

/* event handler, called when a packet is passed from the lower layer at the
   receiver */
/* my changes
*  if the packet has no right seq_number, put it in buffer, continue to wait;
*/
void Receiver_FromLowerLayer(struct packet *pkt)
{
    /* 1-byte header indicating the size of the payload */
    // the seq number expected

    //checksum
    short tmp_sum = *((short*)(pkt->data));
    if(tmp_sum != getCheckSum(pkt)) return;

    //mycode
    int seq_number = *((int*)(pkt->data + sizeof(short)));
    //printf("Receiver_FromLowerLayer seq: %d  expect: %d\n", seq_number, expected_num);
    //put it in buffer and waitting
    if(seq_number > expected_num){
      buffer[seq_number % BUFFER_SIZE] = *pkt;
      valid_bit[seq_number % BUFFER_SIZE] = 1;
      return;
    }else if(seq_number < expected_num){
      // prevent ack packet loss
      response(seq_number);
      return;
    }

    /* construct a message and deliver to the upper layer */
    struct message *msg = (struct message*) malloc(sizeof(struct message));
    ASSERT(msg!=NULL);

deliver:
//----------------------------------------------------------------------


    msg->size = pkt->data[HEADER_SIZE - 1];

    /* sanity check in case the packet is corrupted */
    if (msg->size<0) msg->size=0;
    if (msg->size>RDT_PKTSIZE-HEADER_SIZE) msg->size=RDT_PKTSIZE-HEADER_SIZE;

    msg->data = (char*) malloc(msg->size);
    ASSERT(msg->data!=NULL);
    memcpy(msg->data, pkt->data+HEADER_SIZE, msg->size);
    Receiver_ToUpperLayer(msg);

    //send ack packets to sender
    response(expected_num);

//---------------------------------------------------------------------------
   //mycode
    expected_num++;
    if(valid_bit[expected_num % BUFFER_SIZE] == 1){
      pkt = &buffer[expected_num % BUFFER_SIZE];
      valid_bit[expected_num % BUFFER_SIZE] = 0;

      goto deliver;
    }

    /* don't forget to free the space */
    if (msg->data!=NULL) free(msg->data);
    if (msg!=NULL) free(msg);
}
