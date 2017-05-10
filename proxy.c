/* $begin proxy.c */

/*
 * proxy.c - A Simple Concurrent Web proxy
 *
 * Course Name: 14:332:456-Network Centric Programming
 * Assignment 2
 * Student Name: Alejandro Aguilar Esteban
 * 
 *
 * This program contains two sockets, one for the server and one for the client, essentially operating like a proxy.
 * The socket server reads input from the client(browser) and identifies HTTP functions. THe client socket
 * sends the information to the server, where the HTTP request is parsed and identified as a valid request, else
 * it is denied. When the request is confirmed a log entry is logged in the proxy.log and the information is sent back to the client.
 * On the terminal it will display the host and the ip address of the website.
 *
 * This is proxy is done through forking, as it allows multiple processors to tackle the connections. Forking speeds up the process of
 * accepting multiply request.
 */ 

#include "csapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * Function prototypes
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{

	/*** Variables ***/
    	pid_t pid;
	int cfd;
	int optval = 1;

   	/*** Structs ***/
	struct hostent* host;
   	/*** sockets for client and server ***/
   	struct sockaddr_in client_addr, SA;

    	/* Check arguments */
   	 if (argc != 2) {
		fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
		exit(0);
    	}


     	printf("/*****Proxy enabled*****/\n");

	/* socket stucture has many fields, must make 0
	 * since in c the bits come in after another easier to fill
	 * starting location */

	/***Socket***/
	int sfd = Socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);

	/** Reuse Socket***/
	if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) < 0 ){ return -1; }	// allow user to reuse same port
	
	/** bzerro takes two arguments and sets them to zero ***/
	bzero((char*)&SA,sizeof(SA)); 
	bzero((char*)&client_addr,sizeof(client_addr)); 
	SA.sin_family=AF_INET;		       // family
	SA.sin_addr.s_addr=htonl(INADDR_ANY);  // don't care about interface
	SA.sin_port=htons(atoi(argv[1]));      // port number given at execution


	
	
	/*** Bind ***/
	Bind(sfd,(struct sockaddr*)&SA, sizeof(SA));
	

	/*** Listen ***/
	/**Backlog can be any number ***/
	Listen(sfd,50);
	int client_len=sizeof(client_addr);
	

	/*** Accept **/
	/* last two arguments allow to see who is connect
	 * Return argument is a file descriptor
	 */
		
 accepting:

	cfd = Accept(sfd,(struct sockaddr*)&client_addr, (socklen_t*)&client_len);
	
	
	/*** concurrent server	is made to allow more than one request at a time***/
	pid=fork();		

	/*** Connect client ***/
	if(pid==0){
	struct sockaddr_in host_addr;
	int flag=0,n,port=0,i,sfd1;

	/*** The following will be  request strings that will be parsed ***/
	char buffer[510],temp1[300],temp2[300],temp3[10];
	char* temp=NULL;
	bzero((char*)buffer,500);
	recv(cfd,buffer,500,0);
   	sscanf(buffer,"%s %s %s",temp1,temp2,temp3);
   	
   
	/*** Read Request server ***/
	if(((strncmp(temp1,"GET",3)==0))&&((strncmp(temp3,"HTTP/1.1",8)==0)||(strncmp(temp3,"HTTP/1.0",8)==0))&&(strncmp(temp2,"http://",7)==0)){
	strcpy(temp1,temp2);
   	flag=0;
   
	for(i=7;i<strlen(temp2);i++){
		if(temp2[i]==':'){
			flag=1;
			break;
			}
		}
   
	temp=strtok(temp2,"//");
	if(flag==0){
	port=80;
	temp=strtok(NULL,"/");
	}else{
	temp=strtok(NULL,":");
	}
   

	/*** Read Request client ***/
	sprintf(temp2,"%s",temp);
	printf("host = %s",temp2);
	host=gethostbyname(temp2);



	char* filename = "proxy.log";

	/*** open log file and write ***/
	FILE* fp = fopen(filename,"a");
	if(fp == NULL){	
		perror("proxy.c");
		exit(2);
	}
	char log[500];
	char *temp5 = " ";
	char temp6[500];
	strcpy(temp6,temp1);
	char *temp4 = strcat(temp6,temp5);
	temp4 = strcat(temp4,temp3);
        format_log_entry(log,&client_addr,temp4,sizeof(inet_ntoa(host_addr.sin_addr)));
	fprintf(fp, "log:\n %s\n\n",log);
	fclose(fp);
	//printf("THis is past format_log function\n");


   
	if(flag==1){
	temp=strtok(NULL,"/");
	port=atoi(temp);
	}
   	
	strcat(temp1,"^]");
	temp=strtok(temp1,"//");
	temp=strtok(NULL,"/");


	if(temp!=NULL){
	temp=strtok(NULL,"^]");
	printf("\npath = %s\nPort = %d\n",temp,port);
	}
   
   
	bzero((char*)&host_addr,sizeof(host_addr));
	host_addr.sin_port=htons(port);
	host_addr.sin_family=AF_INET;
	bcopy((char*)host->h_addr,(char*)&host_addr.sin_addr.s_addr,host->h_length);
   
	sfd1=Socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	Connect(sfd1,(struct sockaddr*)&host_addr,sizeof(struct sockaddr));
	sprintf(buffer,"\nConnected to %s  IP - %s\n",temp2,inet_ntoa(host_addr.sin_addr));
	
	printf("\n%s\n",buffer);
	bzero((char*)buffer,sizeof(buffer));


	/*** Write server ***/
	
	if(temp!=NULL){
	sprintf(buffer,"GET /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",temp,temp3,temp2);
	}else{
	sprintf(buffer,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",temp3,temp2);
 	}
 
	
	/*** Write client ***/
	n=send(sfd1,buffer,strlen(buffer),0);
	printf("\n%s\n",buffer);
		do{
			bzero((char*)buffer,500);
			n=recv(sfd1,buffer,500,0);
			if(!(n<=0)){
				send(cfd,buffer,n,0);
				}
		   }while(n>0);
	}else{
		send(cfd,"400 : BAD REQUEST\nONLY HTTP REQUESTS ALLOWED",18,0);
	}
	/*** close client ***/
	close(sfd1);
	close(cfd);
	/*** close server ***/
	close(sfd);
	_exit(0);
	}else{
	close(cfd);
	goto accepting;
	}


        return 0;


}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}


