/*
 * udpserver.cpp
 *
 *  Created on: Jan 23, 2017
 *      Author: MohanSanodia
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
 #include <fcntl.h>
using namespace std;
class Udp_Server
{
public:
	Udp_Server(int portnumber);
	~Udp_Server();
public:
	void m_setportnumber(int port);
	int m_getportnumber();
	void SavetheFile(int,unsigned char*,unsigned char,int);
	void openSocketForListening();
	pthread_t t1;
	size_t m_file_size;
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
	char *hostaddrp; /* dotted decimal host addr string */
	char FILE_NAME[100];
	int clientlen;
	FILE *fp;
private:
	int m_udpport,sockfd,optval,m_send_response;
	Udp_Server();
	Udp_Server(const Udp_Server& );
	Udp_Server& operator =(const Udp_Server& other);
	static void * ReceiveSendData(void * );
};
Udp_Server::Udp_Server(int port)
{
	m_setportnumber(port);
	m_send_response = 0;
	openSocketForListening();
	fp = NULL;
}
Udp_Server::~Udp_Server()
{
	if(fp){
		fclose(fp);fp=NULL; }
	if(sockfd)close(sockfd);
}
void Udp_Server::openSocketForListening()
{
	/*  socket: create the parent socket    */
	int ret;
	  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	  if (sockfd < 0)
		  perror("ERROR opening socket");

	  /* make socket non blocking */
	  int flags = fcntl(sockfd, F_GETFL, 0);
	  fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	  /*set socketopt */
	  optval = 1;
	  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));
	  /*Socket Internet Address */
	   bzero((char *) &serveraddr, sizeof(serveraddr));
	   serveraddr.sin_family = AF_INET;
	   serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	   //serveraddr.sin_addr.s_addr = htonl("198.168.0.4");
	   printf("m_udpport : %d\n",m_udpport);
	   serveraddr.sin_port = htons((unsigned short)m_udpport);
	   /* Bind The socket */
	   if (bind(sockfd, (struct sockaddr *) &serveraddr,sizeof(serveraddr)) < 0)
	   perror("ERROR on binding");
	   clientlen = sizeof(clientaddr);
	   ///start receiver thread to receive the data.........
	   ret =  pthread_create(&t1, NULL, &ReceiveSendData, this);
	   	        if(ret != 0) {
	   	                printf("Error: pthread_create() failed\n");
	   	                exit(EXIT_FAILURE);
	   	        }
	   printf("Receive Data thread Started .......\n");
}
void* Udp_Server::ReceiveSendData(void *arg)
{
	int count, send_size,n;
	unsigned char* th_buf = NULL;
	unsigned char m_buff[MTU_SIZE];
	static int packet_loss = 0;
	size_t size;
	Udp_Server *t_recv = (Udp_Server*)arg;
	int s = t_recv->sockfd;
	int c_len = t_recv->clientlen;
	struct sockaddr_in c_addr = t_recv->clientaddr;
	bool send_response = false;
	size = t_recv->m_file_size;
	HDR response_hdr;
	HDR *t_hdr;
	int old_seq=0;
	bzero(&m_buff[0], MTU_SIZE);
	while (1) {

			/* recvfrom: receive a UDP datagram from a client    */
			count = recvfrom(s, m_buff, MTU_SIZE, 0,(struct sockaddr *) &c_addr, &c_len);
				//perror("ERROR in recvfrom");
			if (count > 0)
			{

					if (m_buff[0] == MSG_START)
					{
						printf("Valid Message Received.....\n");
						//for (int k = 0; k < 50; k++)// For Debug...
							//printf("%c ", m_buff[k]);
						printf("\n");
					}
					else
					{
						printf("Invalid Messages.....\n");
						continue;
					}
			t_hdr = (HDR*)&m_buff[0];
			//printf("%d\t%d\t%d\t%d\n", t_hdr->start_code, t_hdr->hdr_type,t_hdr->end_of_data,t_hdr->seq_num);
			if (t_hdr->payload_length) {
				th_buf = &m_buff[sizeof(HDR)];

			}
			switch (t_hdr->hdr_type)
			{
				case CAN_I_SEND_THE_FILE_WITHNAME_XYZ:
					char a[50];
					memcpy(&a,th_buf,t_hdr->payload_length);
					n = sprintf(t_recv->FILE_NAME, "recv_%s", a);
					t_recv->FILE_NAME[n] = '\0';
					printf("The File Name is %s \n", t_recv->FILE_NAME);
					//Prepare the Response header to be send....
					response_hdr.start_code = MSG_START;
					response_hdr.hdr_type = YES_U_CAN_SEND_THE_FILE;
					response_hdr.end_of_data = END_OF_DATA;
					response_hdr.payload_length = 0;
					memcpy(&m_buff[0],(unsigned char*)&response_hdr,sizeof(HDR));
					send_response = true;
					//send message to client for file size;
					break;
				case FILE_DATA:
					printf("seq no : %d ,data size : %d \n",t_hdr->seq_num,t_hdr->payload_length);
					if (t_hdr->seq_num != (old_seq + 1)) {
					printf("The packet Number %d is loss..\n");
					packet_loss++;
					}
					t_recv->SavetheFile(t_hdr->payload_length,th_buf,t_hdr->end_of_data,packet_loss);
					old_seq = t_hdr->seq_num;
					send_response = false;
					break;
				case ALIVE:
					//check for heart beat of the client;
					break;
				case FILE_SIZE_FROM_CLIENT://save the file size receive by client in a valiable;
					///size =
					break;
				case SEND_THE_FILE_SIZE:
					break;
				case NO_U_CANNT_SEND_THE_FILE:
					send_response = true;
					break;
				default:
					break;

			}
			if(send_response)
			{
				sleep(1);
				int sz = sizeof(HDR) + response_hdr.payload_length;
				printf("Length : %d \n",response_hdr.payload_length);
				send_size = sendto(s, m_buff, sz , 0,(struct sockaddr *) &c_addr, c_len);
						if (send_size < 0)
							perror("ERROR in sendto");
						printf("\nSend  : %d  bytes\n", send_size);
				//for (int k = 0; k < 50; k++)// For Debug...
					//printf("%d ", m_buff[k]);
				send_response = false;

			}
		} else {
			//printf("No data Received from client....[Server Listening]\n");
			usleep(100000);
		}

	}
}
void Udp_Server::SavetheFile(const int s,unsigned char *d,unsigned char m,int loss)
{
	int n;
	static int total_byte=0;
	static int total_packet = 0;
	if(!FILE_NAME)
	{
		printf("FILE NAME MISSING.....\n");
		return;
	}
	if (fp == NULL) {
		fp = fopen(FILE_NAME, "wb");
		fseek(fp, 0, SEEK_SET);

	}
	if(fp)
	{
		n = fwrite(d, 1, s, fp);
		if (n == 0) {
			perror("Wrote to file Failed:");
			//exit(1);
		}
		total_byte += n;
		fseek(fp, total_byte, SEEK_SET);
		printf("Wrote [%d] byte...total_byte received :%d\n", n, total_byte);
		total_packet++;
		if (m == 0) {
			if (fp)
				fclose(fp);
			fp = NULL;
			printf("[%d]Total Wrote : %d\n", fp, total_byte);
			total_byte = 0;
		}
	}
	if (m == 0) {
		printf("Total Packet Received : %d,Total packet loss : %d\n", total_packet,loss);
		printf("==========File Transfer Completed========\n");
		total_packet = 0;
	}
}

void Udp_Server::m_setportnumber(int port)
{
	m_udpport=port;
}
int Udp_Server::m_getportnumber()
{
	return m_udpport;
}


int main(int argc, char* argv[])
{
	Udp_Server *udpserver;
	int portno;
	/* check command line arguments  */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	portno = atoi(argv[1]);
	printf("hello world...\n");
	udpserver = new Udp_Server(portno);
	while (1){

	};
	printf("Exist....");
}




