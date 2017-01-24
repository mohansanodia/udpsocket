/*
 * common.h
 *
 *  Created on: Jan 18, 2017
 *      Author: MohanSanodia
 */

#ifndef COMMON_H_
#define COMMON_H_
#define MTU_SIZE 1500
#define GENERIC 100
#define FILE_TRANSFER 101
#define FILE_TRANSFER_REQUEST 102
#define FILE_TRANSFER_REQUEST_RESPONSE 103

#include <string.h>
#include <assert.h>
#include <pthread.h>
#define FILE_TRANSFER_DONE 105

 enum state {
	//Initialisation......
	CAN_I_SEND_THE_FILE_WITHNAME_XYZ=100,//client to server
	YES_U_CAN_SEND_THE_FILE,//server to client
	NO_U_CANNT_SEND_THE_FILE,//server to client

	FILE_DATA,//client to server;
	SENDINF_FILE_DATA_COMPLETED,//client to server;

	NOT_RECEIVED_PACKET_NO,//client to server;
	TOTAL_DATA_SEND_TO_SERVER,//client to server;
	TRANSFER_SUCCESSFUL,
	HANDSHAKE,
	HANDSHAKE_OK,
	ALIVE,
	LAST_PACKET,
	SEND_THE_FILE_SIZE=200,
	FILE_SIZE_FROM_CLIENT,
	//Reason...
	VERY_BIG_FILE_SIZE

};

#define END_OF_DATA 0
#define HDR_START 0xFF;
char data[1500];
unsigned char buf[MTU_SIZE];
unsigned char recv_buf[MTU_SIZE];

#define KB 1024
#define MB 1024*1024
#define GB (1024*1024*1024)

#define MSG_START 255

typedef struct {
	unsigned char start_code;//Always HDR_START;
	unsigned char hdr_type;//Message Type;
	unsigned char end_of_data;//Check for end of data;if last packet it is one;
	unsigned int seq_num;//sequence number;
	unsigned int payload_length;
}HDR;

#endif /* COMMON_H_ */
