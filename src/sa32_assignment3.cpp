/**
 * @sa32_assignment3
 * @author  Shailesh Adhikari <sa32@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */


#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string>
#include <sys/queue.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <cstring>
#include<iostream>
#include<vector>
#include <arpa/inet.h>
#include<sys/time.h>
#include <fstream>
#include <sstream>

using namespace std;


#define AUTHOR_STATEMENT "I, sa32, have read and understood the course academic integrity policy."
#define CNTRL_HEADER_SIZE 8
#define CNTRL_RESP_HEADER_SIZE 8
#define ROUTING_UPDATE_HEADER_SIZE 8

#define CONTROL_ROUTING_TABLE_RESP_STRUC_SIZE 8
#define ROUTING_UPDATE_STRUC_SIZE 12
#define DATA_PACKET_HEADER_SIZE 12
#define BLOCKSIZE 1024

#define CNTRL_RESP_CONTROL_CODE_OFFSET 0x04
#define CNTRL_RESP_RESPONSE_CODE_OFFSET 0x05
#define CNTRL_RESP_PAYLOAD_LEN_OFFSET 0x06

#define INFINITY 65535


#define ERROR(err_msg) {perror(err_msg); exit(EXIT_FAILURE);}

struct __attribute__((__packed__)) CONTROL_HEADER{
    uint32_t dest_ip_addr;
    uint8_t control_code;
    uint8_t response_time;
    uint16_t payload_len;
};

struct __attribute__((__packed__)) CONTROL_RESPONSE_HEADER{
    uint32_t controller_ip_addr;
    uint8_t control_code;
    uint8_t response_code;
    uint16_t payload_len;
};

struct __attribute__((__packed__)) CONTROL_RESPONSE_ROUTING_TABLE_PAYLOAD{
    uint16_t router_id;
    uint16_t padding;
    uint16_t next_hop_id;
    uint16_t cost;
};

/**struct __attribute__((__packed__)) CONTROL_UPDATE_PAYLOAD{
    uint16_t router_id;
    uint16_t cost;
};**/

struct __attribute__((__packed__)) ROUTING_UPDATE_HEADER{
	uint16_t router_count;
	uint16_t router_port;
    uint32_t router_ip;
};

struct __attribute__((__packed__)) ROUTING_UPDATE_MESSAGE{
	uint32_t router_ip;
	uint16_t router_port;
    uint16_t padding;
    uint16_t router_id;
    uint16_t cost;
};

struct __attribute__((__packed__)) DATA_PACKET_HEADER{
	uint32_t dest_ip;
	uint8_t transfer_id;
	uint8_t ttl;
    uint16_t seq_num;
    uint16_t fin_bit;
    uint16_t padding;
};
 
int CONTROL_PORT;
//uint16_t routing_table[5][5];
fd_set master_list, watch_list;
int head_fd;
int control_socket, router_socket, data_socket;

//Lists shall remain relative
uint16_t router_id_list[5];
uint16_t cost_list[5];
uint16_t next_hop_list[5];
uint16_t router_port_list[5];
uint16_t data_port_list[5];
uint32_t router_ip_list[5];

int neighbor_list[5];

uint16_t this_router_index = INFINITY;

uint16_t num_of_routers;

struct timeval tv;
uint16_t time_interval;

void print_current_time(){
	timeval curTime;
	gettimeofday(&curTime, NULL);
	int milli = curTime.tv_usec / 1000;
	
	char buffer [80];
	strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&curTime.tv_sec));
	
	char currentTime[84] = "";
	sprintf(currentTime, "%s:%d", buffer, milli);
	printf("current time: %s \n", currentTime);
}

// Generate ip addr from unsigned int type
struct in_addr get_ip_from_int(uint32_t ip){
	struct in_addr ip_addr;
    ip_addr.s_addr = ip;
    return ip_addr;
}


char* create_response_header(int sock_index, uint8_t control_code, uint8_t response_code, uint16_t payload_len){
    char *buffer;
    struct CONTROL_RESPONSE_HEADER *cntrl_resp_header;
    struct sockaddr_in addr;
    socklen_t addr_size;

    buffer = (char *) malloc(sizeof(char)*CNTRL_RESP_HEADER_SIZE);
	
	cntrl_resp_header = (struct CONTROL_RESPONSE_HEADER *) buffer;

    addr_size = sizeof(struct sockaddr_in);
    getpeername(sock_index, (struct sockaddr *)&addr, &addr_size);

    memcpy(&(cntrl_resp_header->controller_ip_addr), &(addr.sin_addr), sizeof(struct in_addr));/* Controller IP Address */
    cntrl_resp_header->control_code = control_code;/* Control Code */
    cntrl_resp_header->response_code = response_code;/* Response Code */
    cntrl_resp_header->payload_len = htons(payload_len);/* Payload Length */
    return buffer;
}

char* create_routing_update_header(){
	//cout<<" *********** create_routing_update_header START *********** "<<endl;
	char *buffer;
	
	buffer = (char*) malloc(ROUTING_UPDATE_HEADER_SIZE);
	
    struct ROUTING_UPDATE_HEADER s;
	s.router_count = htons(num_of_routers);
	s.router_port = htons(router_port_list[this_router_index]);
	s.router_ip = htonl(router_ip_list[this_router_index]);

    memcpy(buffer, &s, ROUTING_UPDATE_HEADER_SIZE);
    
    return buffer;
}

char* create_data_packet_header(uint32_t dest_ip, uint8_t transfer_id, uint8_t ttl, uint16_t seq_num, bool is_last){
	//cout<<" *********** create dp _header START ***********  is_last : "<<is_last<<endl;
	//cout<<" transfer_id : "<<unsigned(transfer_id)<<", ttl : "<<unsigned(ttl)<<", dest_ip : "<<inet_ntoa(get_ip_from_int((unsigned)dest_ip)) <<endl;
	char *buffer;
	
	buffer = (char*) malloc(DATA_PACKET_HEADER_SIZE);
	
    struct DATA_PACKET_HEADER s;
	s.dest_ip = htonl(dest_ip);
	s.transfer_id = transfer_id;
	s.ttl = ttl;
	s.seq_num = htons(seq_num);
	s.padding = 0;
	(is_last)?htons(s.fin_bit=0X8000):htons(s.fin_bit=0);
	
    memcpy(buffer, &s, DATA_PACKET_HEADER_SIZE);
    
    /**cout<<"DATA HEADER sent : ";
    for(int k=0; k<DATA_PACKET_HEADER_SIZE;k++){
		for (int j = 0; j < 8; j++) {
		  printf("%d", !!((buffer[k] << j) & 0x80));
		}
		if((k+1)%4==0)
			printf("\n");
		else
			printf(" ");
	}
    **/
    return buffer;
}

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes){
    ssize_t bytes = 0;
    bytes = recv(sock_index, buffer, nbytes, 0);
    //cout<<"In recv all : bytes : "<<bytes;
    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += recv(sock_index, buffer+bytes, nbytes-bytes, 0);
    return bytes;
}

ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes){
    ssize_t bytes = 0;
    bytes = send(sock_index, buffer, nbytes, 0);
    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += send(sock_index, buffer+bytes, nbytes-bytes, 0);
    return bytes;
}


/* Linked List for active control connections */
struct ControlConn{
    int sockfd;
    LIST_ENTRY(ControlConn) next;
}*connection, *conn_temp;
LIST_HEAD(ControlConnsHead, ControlConn) control_conn_list;


bool isControl(int sock_index)
{
    LIST_FOREACH(connection, &control_conn_list, next)
        if(connection->sockfd == sock_index) return true;
    return false;
}

int new_control_conn(int sock_index){
    int fdaccept;
	socklen_t  caddr_len;
    struct sockaddr_in remote_controller_addr;
    caddr_len = sizeof(remote_controller_addr);
    fdaccept = accept(sock_index, (struct sockaddr *)&remote_controller_addr, &caddr_len);
    if(fdaccept < 0)
        ERROR("accept() failed");
    /* Insert into list of active control connections */
    connection = (ControlConn*)malloc(sizeof(struct ControlConn));
    connection->sockfd = fdaccept;
    LIST_INSERT_HEAD(&control_conn_list, connection, next);
    return fdaccept;
}

void remove_control_conn(int sock_index){
    LIST_FOREACH(connection, &control_conn_list, next) {
        if(connection->sockfd == sock_index) LIST_REMOVE(connection, next); // this may be unsafe?
        free(connection);
    }
    close(sock_index);
}


/* Linked List for active data connections */
struct DataConn{
    int sockfd;
    LIST_ENTRY(DataConn) next;
}*data_conn, *data_conn_temp;
LIST_HEAD(DataConnsHead, DataConn) data_conn_list;

bool isData(int sock_index)
{
    LIST_FOREACH(data_conn, &data_conn_list, next)
        if(data_conn->sockfd == sock_index) return true;
    return false;
}

void remove_data_conn(int sock_index){
    LIST_FOREACH(data_conn, &data_conn_list, next) {
        if(data_conn->sockfd == sock_index) LIST_REMOVE(data_conn, next); // this may be unsafe?
        free(data_conn);
    }
    close(sock_index);
}

int new_data_conn(int sock_index){
    int fdaccept;
	socklen_t  daddr_len;
    struct sockaddr_in data_addr;
    daddr_len = sizeof(data_addr);
    fdaccept = accept(sock_index, (struct sockaddr *)&data_addr, &daddr_len);
    if(fdaccept < 0)
        ERROR("accept() failed");
    /* Insert into list of active control connections */
    data_conn = (DataConn*)malloc(sizeof(struct DataConn));
    data_conn->sockfd = fdaccept;
    LIST_INSERT_HEAD(&data_conn_list, data_conn, next);
    return fdaccept;
}

/**
* Create a router udp socket to receive routing updates
**/
void create_router_sock()
{
    struct sockaddr_in router_addr;
    socklen_t addrlen = sizeof(router_addr);

    router_socket = socket(AF_INET, SOCK_DGRAM, 0);
    
    if(router_socket < 0){
    	cout << "create_router_sock : ERROR : sock not created "<<endl;
    	ERROR("socket() failed");
    } 
	bzero(&router_addr, addrlen);

    router_addr.sin_family = AF_INET;
    router_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    router_addr.sin_port = htons(router_port_list[this_router_index]);
    
    //cout<<"Router port : "<<router_addr.sin_port<<endl;

    if(bind(router_socket, (struct sockaddr *)&router_addr, sizeof(router_addr)) < 0){
    	cout << "create_router_sock : ERROR : sock not binding "<<endl;
    	ERROR("bind() failed");
    }
}

void create_data_sock(){
    struct sockaddr_in data_addr;
    socklen_t addrlen = sizeof(data_addr);

    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(data_socket < 0){
    	cout << "create_data_sock : ERROR : sock not created "<<endl;
    	ERROR("socket() failed");
    } 

    /* Make socket re-usable */
    if(setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0){
    	cout << "create_data_sock : ERROR : sock not reusable "<<endl;
    	ERROR("setsockopt() failed");
    }
    
	bzero(&data_addr, sizeof(data_addr));

    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    data_addr.sin_port = htons(data_port_list[this_router_index]);
    
    //cout << " ip : "<< inet_ntoa(control_addr.sin_addr)<<endl;

    if(bind(data_socket, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0){
    	cout << "create_data_sock : ERROR : sock not binding "<<endl;
    	ERROR("bind() failed");
    }
        
    if(listen(data_socket, 5) < 0){
    	cout << "create_data_sock : ERROR : sock not listening "<<endl;
    	ERROR("listen() failed");
    }
        
	//Initialize te data connection list
    LIST_INIT(&data_conn_list);
}

void author_response(int sock_index){
	cout<<" *********** author_response START *********** "<<endl;
	uint16_t payload_len, response_len;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

	payload_len = sizeof(AUTHOR_STATEMENT)-1; // Discount the NULL chararcter
	cntrl_response_payload = (char *) malloc(payload_len);
	memcpy(cntrl_response_payload, AUTHOR_STATEMENT, payload_len);
	cntrl_response_header = create_response_header(sock_index, 0, 0, payload_len);
	response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	/* Copy Payload */
	memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	free(cntrl_response_payload);
	
	sendALL(sock_index, cntrl_response, response_len);
	free(cntrl_response);
	cout<<" *********** author_response END *********** "<<endl;	
}

// TODO : Need to update as per commands
void init_data(char* cntrl_payload){	
	num_of_routers = (unsigned int)cntrl_payload[0] << 8 | cntrl_payload[1];
	time_interval= (unsigned int)cntrl_payload[2] << 8 | cntrl_payload[3];
	
	tv.tv_sec = time_interval;
    tv.tv_usec = 0;
    
	struct in_addr ip_addr;
		
	int offset = 4;	
	
	//Clearing cost and next hop list b4 init : next hop list contains router ids after init
	for(int i = 0;i<5;i++){
		cost_list[i] = INFINITY;
		next_hop_list[i] = INFINITY;
		router_id_list[i] = INFINITY;
		neighbor_list[i] = -1;
	}
	
	//TODO : Better way to do this
	for(int i=0; i<num_of_routers;i++){
		uint16_t router_id = (uint8_t)cntrl_payload[offset]<<8 | (uint8_t)cntrl_payload[offset+1];
		offset+=2;
		uint16_t router_port = (uint8_t)cntrl_payload[offset]<<8 | (uint8_t)cntrl_payload[offset+1];
		offset+=2;
		uint16_t data_port = (uint8_t)cntrl_payload[offset]<<8 | (uint8_t)cntrl_payload[offset+1];
		offset+=2;
		uint16_t cost = (uint8_t)cntrl_payload[offset]<<8 | (uint8_t)cntrl_payload[offset+1];
		offset+=2;
		uint32_t router_ip = (unsigned int)((uint8_t)cntrl_payload[offset] << 24 | (uint8_t)cntrl_payload[offset+1] << 16 |(uint8_t)cntrl_payload[offset+2] << 8 | (uint8_t)cntrl_payload[offset+3]);
		offset+=4;
		
		router_id_list[i] = router_id;
		cost_list[i] = cost;
		router_port_list[i] = router_port;
		data_port_list[i] = data_port;
		router_ip_list[i] = router_ip;
		
		if(cost != INFINITY){
			next_hop_list[i] = router_id;
			neighbor_list[i] = 1;
		}
		else{
			next_hop_list[i] = INFINITY;
			neighbor_list[i] = 0;
		}
			
		if(cost==0)
			this_router_index = i;
		
		//cout<<"router_id : "<<router_id<<" ; "<<"router_port : "<<router_port<<" ; "<<"data_port : "
			//<<data_port<<" ; "<<"cost : "<<cost<<" ; ""router_ip : "<<inet_ntoa(get_ip_from_int(router_ip))<<endl;
	}	
	
	cout<<"Data on init >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> "<<endl;
	cout<<"Num of routers : "<<num_of_routers<<" ; "<<"Interval : "<<time_interval<<endl;
	for(int i =0; i<num_of_routers;i++){
		cout<< i <<"r id : "<<router_id_list[i]<<" | cost : "<<cost_list[i]<<" | next hop : "<<next_hop_list[i]
		<<" | r port : "<<router_port_list[i]<<" | d port : "<<data_port_list[i]<<" | r ip : "<<inet_ntoa(get_ip_from_int(router_ip_list[i]))<<endl;
	}
	cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< "<<endl;
}

bool is_real_neighbor(int index){
	for(int j=0;j<num_of_routers;j++){
		if(router_id_list[index]==next_hop_list[j])
			return true;
	}
	return false;
}

void init_response(int sock_index,char* cntrl_payload){
	print_current_time();
	cout<<" *********** init_response START*********** "<<endl;
	uint16_t payload_len = 0;
	uint16_t response_len;
	
	init_data(cntrl_payload); //initialize all data for this router when screenshot shared
	
	char *cntrl_response_header, *cntrl_response;
	cntrl_response_header = create_response_header(sock_index, 1, 0, payload_len);
	response_len = CNTRL_RESP_HEADER_SIZE;
	
	sendALL(sock_index, cntrl_response_header, response_len);
	
	free(cntrl_response_header);
	
	//Create this router sockets on init
	create_router_sock();
	
	FD_SET(router_socket, &master_list);
    if(router_socket > head_fd) head_fd = router_socket;
    
	//create_data_sock();
	create_data_sock();
	
	FD_SET(data_socket, &master_list);
    if(data_socket > head_fd) head_fd = data_socket;
	
	cout<<"Router socket created at : "<<router_socket<<endl;
	
	cout<<"DATA socket created at : "<<data_socket<<", data port : "<<data_port_list[this_router_index]<<endl;
	
	
	cout<<" *********** init_response END*********** "<<endl;
}


void routing_table_response(int sock_index,char* cntrl_payload){
	cout<<" *********** routing table _response START *********** "<<endl;
	//freopen ("myfile.txt","a",stdout);
	uint16_t payload_len, response_len;
	//char *cntrl_response_payload;
	char *cntrl_response_header, *cntrl_response;
	
	payload_len = num_of_routers*CONTROL_ROUTING_TABLE_RESP_STRUC_SIZE; 
	cntrl_response_header = create_response_header(sock_index, 2, 0, payload_len);

	response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	
	/**cout<<"From lists : "<<endl;
	for(int i =0; i<num_of_routers;i++){
		cout<< i <<" : router id : "<<router_id_list[i]<<" : cost : "<<cost_list[i]<<" : next hop : "<<next_hop_list[i]<<endl;
	}**/
	
	//Adding routing info to payload
	for(int i=0; i<num_of_routers ; i++){
		struct CONTROL_RESPONSE_ROUTING_TABLE_PAYLOAD s;
		s.router_id = htons(router_id_list[i]);
		s.padding = htons(0);
		s.next_hop_id = htons(next_hop_list[i]);
		s.cost = htons(cost_list[i]) ;	
		//cout<<" putting struct at : "<<CNTRL_RESP_HEADER_SIZE+i*CONTROL_ROUTING_TABLE_RESP_STRUC_SIZE<<endl;
		memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE+(i*CONTROL_ROUTING_TABLE_RESP_STRUC_SIZE), &s, CONTROL_ROUTING_TABLE_RESP_STRUC_SIZE);
		//cout<<" routing at "<<i<<" , router_id : "<<s.router_id<<" , padding : "<<s.padding<<" , next_hop : "<<s.next_hop_id<<" , cost : "<<s.cost<<endl;
	}
	
	cout<<"Routing table response &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& : "<<endl;
	for(int k=0; k<response_len;k++){
		for (int j = 0; j < 8; j++) {
		  printf("%d", !!((cntrl_response[k] << j) & 0x80));
		}
		if((k+1)%4==0)
			printf("\n");
		else
			printf(" ");
	}
	cout<<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& : "<<endl;
	
	sendALL(sock_index, cntrl_response, response_len);

	free(cntrl_response);	
	cout<<"response_len : "<<response_len<<" *********** routing table _response END *********** "<<endl;
}

void update_response(int sock_index,char* cntrl_payload){
	cout<<" *********** update response START *********** "<<endl;
	
	char *cntrl_response_header = create_response_header(sock_index, 3, 0, 0);
	
	uint16_t router_id = (uint8_t)cntrl_payload[0]<<8 | (uint8_t)cntrl_payload[1];
	uint16_t cost = (uint8_t)cntrl_payload[2]<<8 | (uint8_t)cntrl_payload[3];
	
	for(int i=0; i<num_of_routers; i++){
		if(router_id_list[i] == router_id){
			cost_list[i] = cost;
			(cost==INFINITY) ? (next_hop_list[i]=INFINITY) : (next_hop_list[i]=router_id);
		}		
	}
	
	cout<<"Updating at router_id : "<<router_id<<", cost : "<<cost<<endl;

	sendALL(sock_index, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	cout<<" *********** update response END *********** "<<endl;
}

void crash_response(int sock_index,char* cntrl_payload){
	cout<<" *********** crash response START *********** "<<endl;
	
	char *cntrl_response_header = create_response_header(sock_index, 4, 0, 0);
	
	sendALL(sock_index, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	
	exit(EXIT_FAILURE);
	cout<<" *********** crash response END *********** "<<endl;
}

int get_dummy_socket(uint32_t dest_ip){
	int dummy_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(dummy_socket < 0){
    	cout << "create_dummy_sock : ERROR : sock not created "<<endl;
    	ERROR("socket() failed");
    } 
    
    uint16_t next_hop_id;
    uint16_t next_hop_port;
    uint32_t next_hop_ip;
    
    /**cout<<"From lists : "<<endl;
	for(int i =0; i<num_of_routers;i++){
		cout<< i <<" : router id : "<<router_id_list[i]<<" : cost : "<<cost_list[i]<<" : next hop : "<<next_hop_list[i]<<endl;
	}**/
	
	/**cout<<"Socket creation for ip : "<<inet_ntoa(get_ip_from_int(dest_ip))<<endl;
    
    cout<<"From lists : "<<endl;
	for(int i =0; i<num_of_routers;i++){
		cout<< i <<"rid : "<<router_id_list[i]<<",cost : "<<cost_list[i]<<",hop : "<<next_hop_list[i]
		<<", ip : "<<inet_ntoa(get_ip_from_int(router_ip_list[i]))<<endl;
	}**/
    
    print_current_time();
    for(int i=0;i<num_of_routers;i++){
    	cout<<"in loop : "<<i<<endl;
    	if(router_ip_list[i]==dest_ip){
    		cout<<"dest port : "<<data_port_list[i]<<endl;
    		next_hop_id = next_hop_list[i];
    		cout<<"next hop check : "<<next_hop_id<<endl;
    		for(int j=0;j<num_of_routers;j++){
    			if(next_hop_id == router_id_list[j]){
    				next_hop_port = data_port_list[j];
	    			next_hop_ip = router_ip_list[j];
	    			cout<<"next hop port : "<<next_hop_port<<" , ";
	    			cout<<"next hop ip : "<<next_hop_ip<<endl;
	    			break;
    			}
    		}
    		break;
    	}
    }

    struct sockaddr_in data_addr;
    socklen_t addrlen = sizeof(data_addr);
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = htonl(next_hop_ip);
    data_addr.sin_port = htons(next_hop_port);
    
    cout<<" socket created for : ip "<<inet_ntoa(get_ip_from_int(data_addr.sin_addr.s_addr))<<" , data port : "<<(data_addr.sin_port)<<endl;
    
    if(connect(dummy_socket, (struct sockaddr*)&data_addr, sizeof(data_addr))<0)
    	cout<<"Dummy socket didnt connect for ip : "<<inet_ntoa(get_ip_from_int(data_addr.sin_addr.s_addr))<<" , data port : "<<(data_addr.sin_port)<<endl;
    cout<<"Dummy socket creation ends"<<endl;
    return dummy_socket;
}

void read_send_kB_style(uint32_t dest_ip, uint8_t ttl, uint8_t transfer_id, uint16_t seq_num, char* filename){
//	cout<<"************************ read_send_kB_style starts : fiename : "<<filename<<endl;
    fstream file;
    file.open(filename, ios::in|ios::binary|ios::ate);
    
    file.seekg (0, file.end);
    int length = file.tellg();
    file.seekg (0, file.beg);
    
    int chunk_count = length/1024;
    //cout<<" , file length : "<<length<<" , ";
    int dumm_sock = get_dummy_socket(dest_ip);
    //print_current_time();
   // cout<<" , chun count : "<<chunk_count<<endl;
    char *data_packet_header;
    char *data_packet = (char*)malloc(DATA_PACKET_HEADER_SIZE + BLOCKSIZE);
	char* mem_block = (char*) malloc(BLOCKSIZE);
	/**cout<<" , chunk count 2 : "<<chunk_count<<endl;
	print_current_time();
	cout<<"From lists : "<<endl;
	for(int i =0; i<num_of_routers;i++){
		cout<< i <<" : router id : "<<router_id_list[i]<<" : cost : "<<cost_list[i]<<" : next hop : "<<next_hop_list[i]<<endl;
	}**/
	
	for(int i=0;i<chunk_count;i++){
		data_packet_header = create_data_packet_header(dest_ip, transfer_id, ttl, seq_num, (i==(chunk_count-1)));
		
		memcpy(data_packet, data_packet_header, DATA_PACKET_HEADER_SIZE);
		
		file.read(mem_block, BLOCKSIZE); //Read Block 
		
		
		
		memcpy(data_packet+DATA_PACKET_HEADER_SIZE, mem_block, BLOCKSIZE);
		 
		sendALL(dumm_sock, data_packet, DATA_PACKET_HEADER_SIZE + BLOCKSIZE);
		
		seq_num++;
		free(data_packet_header);
	}
	
	free(mem_block);
	free(data_packet);
	close(dumm_sock);
	file.close();
	//print_current_time();
	cout<<"************************ read_send_kB_style ends "<<filename<<endl;
}

void send_file_init_n_response(int sock_index, char* cntrl_payload, int payload_len){
	print_current_time();
	cout<<"#############SEND FILE start###############   Payload length : "<<payload_len<<endl;
	
	int filename_len = payload_len - 8;
	// Initializing payload data
	uint32_t dest_ip = (uint8_t)cntrl_payload[0]<<24 | (uint8_t)cntrl_payload[1]<<16 | (uint8_t)cntrl_payload[2]<<8 | (uint8_t)cntrl_payload[3];
	uint8_t ttl = cntrl_payload[4];
	uint8_t transfer_id = cntrl_payload[5];
	uint16_t seq_num = (uint8_t)cntrl_payload[6]<<8 | (uint8_t)cntrl_payload[7];
	std::string file_name = string(cntrl_payload+8,filename_len);
	
	//char *filename = (char*)malloc(sizeof(char)*filename_len);
	char *filename = (char*)file_name.c_str();
	//memcpy(filename, cntrl_payload+8, filename_len);
	
	//cout<<"dest_ip as int : "<<dest_ip<<" , ";
	//cout<<"dest_ip : "<<inet_ntoa(get_ip_from_int((unsigned)dest_ip))<<", ttl : "<<unsigned(ttl)<<", transfer id : "<<unsigned(transfer_id)
	//<<", seq num : "<<seq_num<<", filename ; "<<filename<<", filename_len : "<<filename_len<<endl;
	
	read_send_kB_style(dest_ip, ttl, transfer_id, seq_num, filename);
	
	//Sending response
	char *cntrl_response_header, *cntrl_response;
	cntrl_response_header = create_response_header(sock_index, 5, 0, 0);
	int response_len = CNTRL_RESP_HEADER_SIZE;
	
	sendALL(sock_index, cntrl_response_header , response_len);
	
	//cout<<"Sending response back "<<endl;
	//print_current_time();
	//cout<<"Response data : %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"<<endl;
	/**for(int k=0; k<CNTRL_RESP_HEADER_SIZE;k++){
		for (int j = 0; j < 8; j++) {
		  printf("%d", !!((cntrl_response_header[k] << j) & 0x80));
		}
		if((k+1)%4==0)
			printf("\n");
		else
			printf(" ");
	}**/
	
	free(cntrl_response_header);
	
}	

bool control_recv_hook(int sock_index)
{
	//cout<< "control_recv_hook start : "<<endl;
    char *cntrl_header, *cntrl_payload;
    uint8_t control_code;
    uint16_t payload_len;

    /* Get control header */
    cntrl_header = (char *) malloc(sizeof(char)*CNTRL_HEADER_SIZE);
    bzero(cntrl_header, CNTRL_HEADER_SIZE);

    if(recvALL(sock_index, cntrl_header, CNTRL_HEADER_SIZE) < 0){
    	
        remove_control_conn(sock_index);
        free(cntrl_header);
        //cout<< "recvALL less than zero error"<<endl;
        return false;
    }
    struct CONTROL_HEADER *header = (struct CONTROL_HEADER *) cntrl_header;
    control_code = header->control_code;
    payload_len = ntohs(header->payload_len);
    //cout<< "Control code : "<< ntohs(control_code) <<endl;
    //cout<< "Payload length : "<< payload_len <<endl;
    free(cntrl_header);

    /* Get control payload */
    if(payload_len != 0){
        cntrl_payload = (char *) malloc(sizeof(char)*payload_len);
        bzero(cntrl_payload, payload_len);
        if(recvALL(sock_index, cntrl_payload, payload_len) < 0){
            remove_control_conn(sock_index);
            free(cntrl_payload);
            return false;
        }
    }
   /* Triage on control_code */
    switch(control_code){
        case 0: author_response(sock_index);
                break;
        case 1: init_response(sock_index, cntrl_payload);
        		break;
		case 2: routing_table_response(sock_index, cntrl_payload);
				break;
		case 3: update_response(sock_index, cntrl_payload);
				break;
		case 4: crash_response(sock_index, cntrl_payload);
				break;
		case 5: send_file_init_n_response(sock_index, cntrl_payload, payload_len);
				break;

        /* .......
        case 1: init_response(sock_index, cntrl_payload);
                break;

            .........
           ....... 
         ......*/
    }

    if(payload_len != 0) free(cntrl_payload);
    return true;
}

void send_routing_update(){
	//cout<<" *********** send_routing_update START *********** "<<endl;
	
	uint16_t payload_len, response_len;
	//char *cntrl_response_payload;
	char *routing_update_header, *routing_update_response;
	
	payload_len = num_of_routers*ROUTING_UPDATE_STRUC_SIZE; 
	routing_update_header = create_routing_update_header();

	response_len = ROUTING_UPDATE_HEADER_SIZE + payload_len;
	routing_update_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(routing_update_response, routing_update_header, ROUTING_UPDATE_HEADER_SIZE);
	free(routing_update_header);
	
	/* Copy payload */	
	for(int i=0; i<num_of_routers; i++){	
		struct ROUTING_UPDATE_MESSAGE s;
		s.router_ip = htonl(router_ip_list[i]);		
		s.router_port = htons(router_port_list[i]);
		s.padding = htons(0);
		s.router_id = htons(router_id_list[i]);
		s.cost = htons(cost_list[i]) ;	
		//cout<<"Data to be sent at i : "<<i<<" : s.router_ip : "<<s.router_ip<<", s.padding : "<<s.padding<<",s.router_id : "<<s.router_id
		//<<" : s.cost : "<<s.cost<<", s.router_port : "<<s.router_port<<endl;
		memcpy(routing_update_response+ROUTING_UPDATE_HEADER_SIZE+(i*ROUTING_UPDATE_STRUC_SIZE), &s, ROUTING_UPDATE_STRUC_SIZE);	
    }
    //sendto(int sockfd, const void *msg, int len, unsigned int flags,const struct sockaddr *to, socklen_t tolen);
    for(int i=0; i<num_of_routers; i++){
    	if(this_router_index!=i && cost_list[i]!=INFINITY && neighbor_list[i]>0 && is_real_neighbor(i)){
    		struct sockaddr_in router_addr;
		    socklen_t addrlen = sizeof(router_addr);
		    router_addr.sin_family = AF_INET;
		    router_addr.sin_addr.s_addr = htonl(router_ip_list[i]);
		    //inet_pton(AF_INET, ,router_addr.sin_addr);
		    router_addr.sin_port = htons(router_port_list[i]);
		    
		    //cout<<router_socket<<"Sending to : ip : "<<inet_ntoa(get_ip_from_int(router_addr.sin_addr.s_addr))
			//<<", router port : "<<router_addr.sin_port<<endl;
		
	    	if(sendto(router_socket, routing_update_response, response_len, 0,(struct sockaddr *)&router_addr, sizeof(router_addr))<0)
				cout<<"Error sending routing update to : " <<i<<endl;
			/**cout<<"Msg sent : %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"<<endl;
			for(int k=0; k<response_len;k++){
				for (int j = 0; j < 8; j++) {
				  printf("%d", !!((routing_update_response[k] << j) & 0x80));
				}
				if((k+1)%4==0)
					printf("\n");
				else
					printf(" ");
			}
			cout<<"End of message : %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"<<endl; **/   
    	}
    }
    //cout<<" *********** send_routing_update ENDS *********** "<<endl;
}

void receive_routing_update(int sock_index){
	int msg_len = ROUTING_UPDATE_HEADER_SIZE+num_of_routers*ROUTING_UPDATE_STRUC_SIZE;
    char* msg = (char *)malloc(msg_len);
    struct sockaddr from_addr;
    socklen_t from_addr_len = sizeof(from_addr);
    bzero(&from_addr, from_addr_len);
	//bytes = recv(sock_index, buffer, msg_len, 0);
    //int recvfrom(int sockfd, void *buf, int len, unsigned int flags, struct sockaddr *from, int *fromlen);
    recvfrom(sock_index, msg, msg_len, 0, &from_addr, &from_addr_len);
    /**cout<<"Message recevied : ";
    for(int k=0; k<msg_len;k++){
		for (int j = 0; j < 8; j++) {
		  printf("%d", !!((msg[k] << j) & 0x80));
		}
		if((k+1)%4==0)
			printf("\n");
		else
			printf(" ");
	}**/
	
	uint16_t num_of_fields = (uint8_t)msg[0] << 8 | (uint8_t)msg[1];
	uint16_t router_port = (uint8_t)msg[2] << 8 | (uint8_t)msg[3];
	int neighbor_index = -1;// u
	
	for(int i=0; i<num_of_routers; i++){
		if(router_port == router_port_list[i]){			
			neighbor_index = i;
			break;
		}
	}
	/**cout<<"***************************************************"<<endl;
	print_current_time();
	cout<<"Before cost list : "<<endl;
	for(int i=0; i<num_of_routers; i++){
		cout<<this_router_index+1<<" -> "<<i+1<<" : "<<cost_list[i]<<" | ";
	}
	cout<<endl;**/
	
	//cout<<"At receive : fields : "<<num_of_fields<<", r_port : "<<router_port<<", neigh_index : "<<neighbor_index<<endl;
	//cout<<"At receive :  neigh_index : "<<neighbor_index<<endl;
	
	int offset = 18;
	
	for(int i=0; i<num_of_fields; i++){
		//cout<<i<<" : offset : "<<offset<<endl; 
		// i is v
		uint16_t router_id = router_id_list[i];
		uint16_t cost_from_neighbor = (uint8_t)msg[offset]<<8 | (uint8_t)msg[offset+1];//weight
		offset += 12;
		//offset+=2;
		
		//cout<<neighbor_index+1<<" -> "<<i+1<<" : "<<cost_from_neighbor<<" | ";
		
		//if(cost[v]>cost[u]+weight) : update cost as cost[u]=weight and next hop as neighbor
		
		if(cost_from_neighbor != INFINITY && (cost_list[i]>cost_list[neighbor_index]+cost_from_neighbor)){
			cost_list[i] = cost_list[neighbor_index]+cost_from_neighbor;
			next_hop_list[i] = router_id_list[neighbor_index];
		}
	}
/**	cout<<endl;
	cout<<"Updated cost list : "<<endl;
	for(int i=0; i<num_of_routers; i++){
		cout<<this_router_index+1<<" -> "<<i+1<<" : "<<cost_list[i]<<" | ";
	}
	cout<<"*******************************************************"<<endl;**/
}

int forwarding_dummy_socket = -1;
bool writing_started = false;
ofstream file;

bool data_recv_hook(int sock_index)
{
	char* received_data = (char*)malloc(DATA_PACKET_HEADER_SIZE + BLOCKSIZE);
	char* data_payload;
	char* data_packet_header;
	char* packet_to_send;
	
	int received_bytes = recvALL(sock_index, received_data, DATA_PACKET_HEADER_SIZE + BLOCKSIZE) ;
	
	if( received_bytes < 0){
    	
        remove_data_conn(sock_index);
        free(received_data);
        return false;
    }
    
    
    
    //cout<<"Data receviced in HOOK : trying to write now. Bytes : "<<received_bytes<<endl;
    
   /** cout<<"DATA HEADER recevied : ";
    for(int k=0; k<DATA_PACKET_HEADER_SIZE;k++){
		for (int j = 0; j < 8; j++) {
		  printf("%d", !!((received_data[k] << j) & 0x80));
		}
		if((k+1)%4==0)
			printf("\n");
		else
			printf(" ");
	}**/
    
    uint32_t dest_ip = (uint8_t)received_data[0]<<24 | (uint8_t)received_data[1]<<16 | (uint8_t)received_data[2]<<8 | (uint8_t)received_data[3];
	uint8_t transfer_id = received_data[4];
	uint8_t ttl = received_data[5];
	uint16_t seq_num = (uint8_t)received_data[6]<<8 | (uint8_t)received_data[7];
	uint16_t fin_bit = (uint8_t)received_data[8]<<8 | (uint8_t)received_data[9];
	ttl=(unsigned)ttl-1;
	
	 if(ttl==0){
    	cout<<"TTL became 0"<<endl;
    //	remove_data_conn(sock_index);
    //	free(received_data);
    //	writing_started=false;
    	//if(file!=NULL)
    	//	file.close();
    //	close(forwarding_dummy_socket);
    	//forwarding_dummy_socket = -1;
    	//return false;//drop if ttl is zero
    	//return 
    }
    
    
    
    //if(seq_num%10 == 0)
    //	cout<<"FILE TRANSFER GOING ON : is dest : "<<(dest_ip==router_ip_list[this_router_index])<<" , ttl : "<<(unsigned)ttl<<", seq_num : "<<seq_num<<endl;
    
        
   
    	 
	
	if(dest_ip==router_ip_list[this_router_index]){ //destination reached
    
    	if(!writing_started){
			writing_started=true;
	    	std::stringstream out_file_name_ss;
	        out_file_name_ss<<"file-"<<(unsigned)transfer_id;
	        string new_s = out_file_name_ss.str();
	        const char * out_file_name = new_s.c_str();
			file.open(out_file_name, std::ios::binary | std::ios::out | std::ios::trunc);
			cout<<" out_file_name : "<<out_file_name<<endl;
		}
	    data_payload = (char*)malloc(BLOCKSIZE);
	    memcpy(data_payload, received_data+DATA_PACKET_HEADER_SIZE, BLOCKSIZE);
    	
		file.write(data_payload, 1024);
		free(received_data);
		free(data_payload);
		
    }
    else{
    	
    	if(forwarding_dummy_socket==-1){
		//cout<<"STARTING FILE TRANSFER "<<endl;
		//cout<<"From lists : "<<endl;
		//for(int i =0; i<num_of_routers;i++){
		//	cout<< i <<" : router id : "<<router_id_list[i]<<" : cost : "<<cost_list[i]<<" : next hop : "<<next_hop_list[i]<<endl;
		//}
			forwarding_dummy_socket = get_dummy_socket(dest_ip);
		}
    	
    	data_payload = (char*)malloc(BLOCKSIZE);
    	packet_to_send = (char*)malloc(DATA_PACKET_HEADER_SIZE + BLOCKSIZE);
    	
    	memcpy(data_payload, received_data+DATA_PACKET_HEADER_SIZE, BLOCKSIZE);
    	
    	data_packet_header = create_data_packet_header(dest_ip, transfer_id, ttl, seq_num, (ntohs(fin_bit)==0X8000));
		
		memcpy(packet_to_send, data_packet_header, DATA_PACKET_HEADER_SIZE);
		
		memcpy(packet_to_send+DATA_PACKET_HEADER_SIZE, data_payload, BLOCKSIZE);
		
    	sendALL(forwarding_dummy_socket, packet_to_send, DATA_PACKET_HEADER_SIZE + BLOCKSIZE);
    	
    	free(received_data);
    	free(data_payload);
    	free(data_packet_header);
    	free(packet_to_send);
    } 	
    
    
	
		
	
    //cout<<"dest_ip : "<<inet_ntoa(get_ip_from_int((unsigned)dest_ip))<<", ttl : "<<unsigned(ttl)<<", transfer id : "<<unsigned(transfer_id)
	//<<", seq num : "<<seq_num<<endl;
    
    
    //Write the data
    
    
    if(ntohs(fin_bit)==0X8000){
		/**cout<<"Fin bit : ";
		for (int j = 0; j < 16; j++) {
			printf("%d", !!((fin_bit << j) & 0x80));
		}**/
		//cout<<"ENDING FILE TRANSFER "<<endl;
		//cout<<"dest_ip : "<<inet_ntoa(get_ip_from_int((unsigned)dest_ip))<<", ttl : "<<unsigned(ttl)<<", transfer id : "<<unsigned(transfer_id)
		//<<", seq num : "<<seq_num<<endl;
		remove_data_conn(sock_index);
		writing_started=false;
		close(forwarding_dummy_socket);
		forwarding_dummy_socket = -1;
		file.close();
		return false;
	}	
	else
		return true;
    
    
}

void main_loop()
{
    int selret, sock_index, fdaccept, data_fd_accept;
    
    tv.tv_sec = 1000;
	tv.tv_usec = 0;
    
    while(true){
        watch_list = master_list;
        selret = select(head_fd+1, &watch_list, NULL, NULL, &tv);
        
        if(selret < 0){
        	cout << "select failed."<<endl;  
        	ERROR("select failed.");
        }
        else if(selret == 0){
        	//cout << "********************** Timed out !!!!!! ********************** selret : "<<selret<<endl;
            tv.tv_sec = time_interval;
            tv.tv_usec = 0;
            send_routing_update();
        }
        
        /* Loop through file descriptors to check which ones are ready */
        for(sock_index=0; sock_index<=head_fd; sock_index+=1){

            if(FD_ISSET(sock_index, &watch_list)){
				//cout << "main_loop  : : in fd is set : sock index : "<<sock_index<<" , head_fd : "<<head_fd<<", selret : "<<selret<<endl;
                /* control_socket */
                if(sock_index == control_socket){
                    fdaccept = new_control_conn(sock_index);

                    /* Add to watched socket list */
                    FD_SET(fdaccept, &master_list);
                    if(fdaccept > head_fd) head_fd = fdaccept;
                }

                /* router_socket */
                else if(sock_index == router_socket){
                    //call handler that will call recvfrom() .....
                    //cout<<"Receiving at ROUTER PORT!!"<<endl;
                    receive_routing_update(sock_index);
                }

                /* data_socket */
                else if(sock_index == data_socket){
                    //new_data_conn(sock_index);
                    //cout<<"Receiving something at DATA PORT!!"<<endl;
                    data_fd_accept = new_data_conn(sock_index);

                    /* Add to watched socket list */
                    FD_SET(data_fd_accept, &master_list);
                    if(data_fd_accept > head_fd) head_fd = data_fd_accept;
                }

                /* Existing connection */
                else{
                	//cout << "Existing connection!!!!"<<endl;
                    if(isControl(sock_index)){
                        if(!control_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);
                        //cout << "Its a command"<<endl;
                        
                    }
                    else if(isData(sock_index)){
                    	//cout<<"Its data"<<endl;
                    	if(!data_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);
                    }
                    //else if isData(sock_index);
                    else ERROR("Unknown socket index");
                }
            }
        }
    }
}

/**
* Creating a tcp socket for control messages
**/
 int create_control_sock()
{
    int sock;
    struct sockaddr_in control_addr;
    socklen_t addrlen = sizeof(control_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
    	cout << "create_control_sock : ERROR : sock not created "<<endl;
    	ERROR("socket() failed");
    } 

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0){
    	cout << "create_control_sock : ERROR : sock not reusable "<<endl;
    	ERROR("setsockopt() failed");
    }
    
	bzero(&control_addr, sizeof(control_addr));

    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    control_addr.sin_port = htons(CONTROL_PORT);
    
    
    
    //cout << " ip : "<< inet_ntoa(control_addr.sin_addr)<<endl;

    if(bind(sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0){
    	cout << "create_control_sock : ERROR : sock not binding "<<endl;
    	ERROR("bind() failed");
    }
        

    if(listen(sock, 5) < 0){
    	cout << "create_control_sock : ERROR : sock not listening "<<endl;
    	ERROR("listen() failed");
    }
        
	//Initialize te control connection list
    LIST_INIT(&control_conn_list);

    return sock;
}

/**void reset_routing_table(){
	for(int i = 0; i<5; i++)
		for(int j=0; j<5; j++)
			routing_table[i][j]=INFINITY;
}**/
 
/**
* main function
*
* @param  argc Number of arguments
* @param  argv The argument list
* @return 0 EXIT_SUCCESS
*/
int main(int argc, char **argv)
{
	/*Start Here*/
	
	//freopen ("new_data11.txt","a",stdout);

	CONTROL_PORT= atoi(argv[1]);
	control_socket = create_control_sock();
	
	
	// Clearing master and watch list
	FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

	//cout << "Control socket : "<<control_socket<<endl;
    /* Register the control socket */
    FD_SET(control_socket, &master_list);
    head_fd = control_socket;
    
    //reset_routing_table();

    main_loop();
    
    //fclose (stdout);
	
	return 0;
	
}