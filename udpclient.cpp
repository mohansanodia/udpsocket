/*
 * udpclient.cpp
 *
 *  Created on: Jan 18, 2017
 *      Author: MohanSanodia
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include <sys/stat.h>
#include <string>

class Udp_Client
{
public:
	Udp_Client(char *,int, char*);
	~Udp_Client();
	void SendtheFile();
	void SendRequestforFileTransfer(int);
	void SendToServer();
	int ReceivefromServer();
	bool getfilesendstatus();
	struct sockaddr_in serveraddr;
	struct hostent *server;
	void setFilesize();
	size_t getFileSize();
	pthread_t t1,t2;
	HDR hdr;
	FILE *fd;
	int serverlen;
	int sockfd;
	char  m_file_name[50];
	int file_name_size;

private:
	Udp_Client();
	char serverip[50];
	int tosend;
	int serverport;
	size_t m_file_size;
	bool FILE_CAN_BE_SEND;
	bool file_send;
	static void * receive_thread(void * );
	static void * sender_thread(void * );

};
Udp_Client::~Udp_Client()
{
	if(fd)
		fclose(fd);

}
Udp_Client::Udp_Client(char* ip,int p, char* filename)
{
	printf("Constructor..[p : %d]\n",p);
	serverport=p;
	file_send = false;
	memset(&m_file_name,0,50);
	memcpy(serverip,ip,50);m_file_size=0;
	int n = sprintf(m_file_name,"%s",filename);
	m_file_name[n]='\0';
	file_name_size = n;
	printf("file_name_size : %d\n",n);
	FILE_CAN_BE_SEND=false;
	//printf("Server ip : %s,  Port :%d\n", serverip, serverport);
	/* socket: create the socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0)
		perror("ERROR opening socket");
	/* gethostbyname: get the server's DNS entry */
	server = gethostbyname(serverip);
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host as %s\n", serverip);
		exit(0);
	}
	/* build the server's Internet address */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &serveraddr.sin_addr.s_addr,server->h_length);
	serveraddr.sin_port = htons(serverport);
	setFilesize();
	SendRequestforFileTransfer(CAN_I_SEND_THE_FILE_WITHNAME_XYZ);

	printf("Start The receiver Thread.......\n");
	int ret = pthread_create(&t2,NULL,&receive_thread,this);


}
void* Udp_Client::receive_thread(void *arg)
{
	Udp_Client *t_recv = (Udp_Client*)arg;
	int s = t_recv->sockfd;
	struct sockaddr_in s_addr = t_recv->serveraddr;
	int s_len = t_recv->serverlen;
	int retval = 0;
	HDR *m;
	while(1)
	{
		retval = recvfrom(s, &recv_buf[0], sizeof(recv_buf), 0,(struct sockaddr*) &s_addr, &s_len);
		if (retval < 0)
		{
					perror("ERROR in recvfrom.");
					usleep(0);
		}
		else
		{
			printf("Received Bytes :  %d \n",retval);
			m = (HDR*) &recv_buf[0];
			printf("%d\t%d\t%d\t%d\n", m->start_code, m->hdr_type,m->end_of_data,m->seq_num);
			switch(m->hdr_type)
			{
				case HANDSHAKE_OK:
					printf("The Server is Alive.............\n");
					break;
				case YES_U_CAN_SEND_THE_FILE:
					printf("YES_U_CAN_SEND_THE_FILE\n");
					t_recv->FILE_CAN_BE_SEND = true;
					break;
				case NO_U_CANNT_SEND_THE_FILE:
					t_recv->FILE_CAN_BE_SEND = false;
					printf("server refuse to receive the file...\n");
					break;
				case NOT_RECEIVED_PACKET_NO:
					printf("The Packet Number xyz Not Received...\n");
						break;
				case TRANSFER_SUCCESSFUL:
					printf("The File Transfer is successful..\n");
					fclose(t_recv->fd);t_recv->fd = NULL;
					break;
				case TOTAL_DATA_SEND_TO_SERVER:
					break;
				default:
					printf("Unknown header type......\n");
					assert(0);
				}

		}
}
}
void Udp_Client::setFilesize()
{
    struct stat st;

    if( access( m_file_name, F_OK ) != -1 ) {
        printf(" The file exist.......[%s]\n",m_file_name);
    } else {
    	printf(" The file does not exist.......[%s]\n",m_file_name);
    	exit(0);
    }


    if(stat(m_file_name, &st) != 0) {
        return;
    }
    m_file_size = st.st_size;
    printf("The file size is %d  byte\n",m_file_size);

}

size_t Udp_Client::getFileSize()
{
	return m_file_size;
}

void Udp_Client::SendRequestforFileTransfer(int req)
{
	int n;

	bzero(&buf, MTU_SIZE);
	bzero(&hdr, sizeof(HDR));
	hdr.start_code=MSG_START;hdr.hdr_type=req;hdr.end_of_data = END_OF_DATA;hdr.seq_num=1;
	memcpy(&buf[sizeof(HDR)],m_file_name,file_name_size);//sending file name to server;
	hdr.payload_length = file_name_size;
	printf("%s  , %d,first :%d\n",m_file_name,hdr.payload_length,hdr.start_code);
	tosend = sizeof(hdr)+ hdr.payload_length;
	memcpy(&buf[0],&hdr,sizeof(hdr));
	SendToServer();
}
void Udp_Client::SendToServer()
{
	int n,retval=false;

	if(hdr.payload_length==0)
	{
		printf("The Payload Length is zero........\n");
		return;
	}
	serverlen = sizeof(serveraddr);
	retval = sendto(sockfd, &buf[0], tosend, 0,(struct sockaddr*) &serveraddr, serverlen);
	if (retval < 0) perror("ERROR in sendto");
	//printf("Send : %d bytes.\n",retval);
	usleep(100);
	return;
}

void Udp_Client::SendtheFile()
{
	pthread_t udp_thread;
	int ret;

	if(m_file_size > 1024*1024)
	{
		printf("The file size is greater than 2GB.............\n");
		return;
	}
	ret =  pthread_create(&t1, NULL, &sender_thread, this);
	        if(ret != 0) {
	                printf("Error: pthread_create() failed\n");
	                exit(EXIT_FAILURE);
	        }
}
void* Udp_Client::sender_thread(void *arg) {//This Thread is to send data to server ,only file data;

	printf("This is Sender_thread()\n");
	Udp_Client *udp = (Udp_Client*)arg;
	size_t size = udp->m_file_size;
	unsigned char ReadBuffer[size]={0,};
	unsigned int result = 0, p;
	unsigned int seq_num = 1;
	size_t total_byte = 0;
	FILE *t_fd=NULL;
	int temp = 0;p = sizeof(HDR);
	HDR h= udp->hdr;
	bzero(&buf, MTU_SIZE);
	bzero(&h, sizeof(HDR));

	int payload = MTU_SIZE - sizeof(HDR);

	if (t_fd == NULL) {
		t_fd = fopen(udp->m_file_name, "rb");
	}

	printf("t_fd : %d ,file size : %d \n",t_fd,udp->getFileSize());
	result = fread(&ReadBuffer, 1, size, t_fd);//full file is copied in a buffer;

	printf("Read from file %d byte \n",result);

	while (total_byte < udp->getFileSize())
	{

			if (udp->FILE_CAN_BE_SEND)
			{
				udp->hdr.start_code = MSG_START;
				udp->hdr.hdr_type = FILE_DATA;
				udp->hdr.payload_length = payload;
				udp->hdr.seq_num = seq_num++;
				udp->hdr.end_of_data = 1;
				if (udp->getFileSize() <= temp + payload) {
					udp->hdr.end_of_data = 0;
					udp->hdr.payload_length = udp->getFileSize() - temp;
				}
				memcpy(&buf[0], &udp->hdr, sizeof(HDR));
				memcpy(&buf[p], &ReadBuffer[temp], payload);
			temp += udp->hdr.payload_length;
			total_byte = temp;
			printf("file size :%d,byte send : %d ,sequence no : %d\n",
					udp->getFileSize(), total_byte, udp->hdr.seq_num);
			//for (int k = 0; k < 8; k++) // for debug messages;
			//printf("%d \t", buf[k+8]);//printf("\n");
			udp->tosend = udp->hdr.payload_length + sizeof(HDR);
			udp->SendToServer();//to be removed to avoid function calling;
			if (udp->hdr.end_of_data == 0) { //0 means this packets is last packet
				printf("==============File Data Sent Completed==================\n");
				udp->file_send = true;


			}
		} else {
			usleep(10000000);
				printf("In Sender_Thread.....\n");
			}
		}
		printf("=========total byte send %d ==================\n", total_byte);
		//Stoping all the threads
		printf("Closing all The Threads........");
		if (udp->file_send) {
		pthread_join(udp->t1, NULL);
		pthread_join(udp->t2, NULL);
		}
	}
bool Udp_Client::getfilesendstatus()
{
	return file_send;
}
int main(int argc, char* argv[]) {
	Udp_Client *udpclient;
	int portno;
	printf("hello world...\n");
	/* check command line arguments  */
	if (argc != 4) {
		fprintf(stderr, "Usage: %s  <ip address> <port> <file name>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	//printf("Address:%s,Port No : %s,filename : %s\n", argv[1], argv[2], argv[3]);
	portno = atoi(argv[2]);
	udpclient = new Udp_Client(argv[1], portno, argv[3]);
	udpclient->SendtheFile();//create sender thread;
	while (1) {
		usleep(1000);
		if(udpclient->getfilesendstatus())//To be added the condition for exit;
			break;
	};

}


