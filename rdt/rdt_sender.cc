/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
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
#include "rdt_sender.h"

#define BUFFER_SIZE 15000
#define HEADER_SIZE 7
#define TIMEOUT 0.1
#define WINDOW_SIZE 10

static int seq_number;  //most recent seq_num sended
static int max_seq_num; //the max seq_num in buffer cunrrently
static int expected_ack; //
static int* ack_buffer;
static packet* packet_buffer;

static short getCheckSum(packet *pkt)
{
  int len = pkt->data[HEADER_SIZE - 1] + sizeof(int);
  unsigned int sum = 0;
  int i = 2;
  for(;i < len; i+=2) sum += *((short*)(&pkt->data[i]));
  sum = ~((sum >> 16) + (sum & 0xFFFF));
  return sum;
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
    seq_number = 0;
    max_seq_num = 0;
    expected_ack = 0;
    packet_buffer = (packet*)malloc(BUFFER_SIZE*sizeof(packet));
    ack_buffer = (int*)malloc(WINDOW_SIZE*sizeof(int));
    memset(ack_buffer, -1, WINDOW_SIZE * sizeof(int));
    Sender_StartTimer(TIMEOUT);
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
    free(packet_buffer);
}

void Send_Message()
{
  /*while(seq_number - expected_ack + 1 < WINDOW_SIZE){
    if(seq_number > max_seq_num) return;
    Sender_ToLowerLayer(&packet_buffer[seq_number]);
    ++seq_number;
  }*/
  while(seq_number < max_seq_num && seq_number - expected_ack < 10){
    //printf("Send_Message seq_number:%d max_seq_num:%d expected_ack:%d\n", seq_number, max_seq_num, expected_ack);
    if(ack_buffer[seq_number % WINDOW_SIZE] == seq_number) goto conti;
    Sender_ToLowerLayer(&packet_buffer[seq_number % BUFFER_SIZE]);
conti:
    seq_number++;
    Sender_StartTimer(TIMEOUT);
  }
}

/* event handler, called when a message is passed from the upper layer at the
   sender */

/* my changes:
*  seq: the number of packet, prevent disorder, put it(4 bytes) before size(1 byte)
*/
void Sender_FromUpperLayer(struct message *msg)
{
    //printf("Sender_FromUpperLayer\n", );
    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - HEADER_SIZE;

    /* split the message if it is too big */

    /* reuse the same packet data structure */
    packet pkt;

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;
    short tmp_checksum;
    while (msg->size-cursor > maxpayload_size) {

      /* fill in the packet */
	    pkt.data[HEADER_SIZE - 1] = maxpayload_size;

	    memcpy(pkt.data + HEADER_SIZE, msg->data+cursor, maxpayload_size);
      //put seq number
      memcpy(pkt.data + sizeof(short), &max_seq_num, sizeof(int));
	    /* send it out through the lower layer */
	    //Sender_ToLowerLayer(&pkt);
      tmp_checksum = getCheckSum(&pkt);
      memcpy(pkt.data, &tmp_checksum, sizeof(short));
	    /* move the cursor */
	    cursor += maxpayload_size;

      //memcpy(&packet_buffer[max_seq_num % BUFFER_SIZE], &pkt, sizeof(packet));
      packet_buffer[max_seq_num % BUFFER_SIZE] = pkt;
      //change the order number
      //seq_number++;
      max_seq_num++;
    }

    /* send out the last packet */
    if (msg->size > cursor) {
      /* fill in the packet */
	    pkt.data[HEADER_SIZE - 1] = msg->size-cursor;

	    memcpy(pkt.data + HEADER_SIZE, msg->data+cursor, pkt.data[HEADER_SIZE - 1]);
      memcpy(pkt.data + sizeof(short), &max_seq_num, sizeof(int));

      tmp_checksum = getCheckSum(&pkt);
      memcpy(pkt.data, &tmp_checksum, sizeof(short));

	    /* send it out through the lower layer */
	    //Sender_ToLowerLayer(&pkt);

      //add it to buffer, prevent time out and resend
      //memcpy(&packet_buffer[max_seq_num % BUFFER_SIZE], &pkt, sizeof(packet));
      packet_buffer[max_seq_num % BUFFER_SIZE] = pkt;

      //seq_number++;
      max_seq_num++;
    }

    Send_Message();
}

/* event handler, called when a packet is passed from the lower layer at the
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
  //check_sum
  short check_sum = *((short*)pkt->data);
  int tmp_int = *((int*)&pkt->data[sizeof(short)]);
  short check_sum_ = ~((tmp_int & 0xFFFF) + (tmp_int >> 16));
  if(check_sum != check_sum_) return;

  int ack_number;
  memcpy(&ack_number, pkt->data + sizeof(short), sizeof(int));
  //printf("Sender_FromLowerLayer  expected_ack:%d ack_num:%d\n", expected_ack, ack_number);
  if(ack_number == expected_ack){
check:
    Sender_StartTimer(TIMEOUT);
    ++expected_ack;
    if(ack_buffer[expected_ack % WINDOW_SIZE] == expected_ack)
      goto check;
    //Send_Message();
  }else if(ack_number > expected_ack){
    ack_buffer[ack_number % WINDOW_SIZE] = ack_number;
  }
  if(expected_ack == max_seq_num) Sender_StopTimer();
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
  //printf("Sender_Timeout\n");
  Sender_StartTimer(TIMEOUT);
  seq_number = expected_ack;
  Send_Message();
}
