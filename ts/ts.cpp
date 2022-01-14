#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __linux__
#include <arpa/inet.h>
#include <sys/socket.h>
#endif // __linux
#ifdef WIN32
#include <winsock2.h>
#include "../mingw_net.h"
#endif // WIN32
#include <thread>

#ifdef WIN32
void perror(const char* msg) { fprintf(stderr, "%s %ld\n", msg, GetLastError()); }
#endif // WIN32

void usage() {
	printf("syntax: ts [-e [-b]] <port>\n");
	printf("  -e : echo\n");
	printf("  -b : broadcast\n");
	printf("sample: ts 1234\n");
}

struct Param {
	bool echo{false};
	bool broadcast{false};
	uint16_t port{0};

	bool parse(int argc, char* argv[]) {
		for (int i = 1; i < argc; i++) {
			if(echo){
				if (strcmp(argv[i], "-b") == 0) {
					broadcast = true;
					continue;
				}
			}
			else if (strcmp(argv[i], "-e") == 0) {
				echo = true;
				continue;
			}
			port = atoi(argv[i++]);
		}
		return port != 0;
	}
} param;

struct Broadcast_node{
	struct Broadcast_node * next = nullptr;
	int client_sd{-1};
} typedef bn;


int mutex_flag = 0;

bn * first_node = new bn;
bn * last_node = nullptr;

// add Client Sd 
void addCS(int sd){ 
	while(true){
		// mutex check
		if(!mutex_flag){
			mutex_flag = 1;
			break;
		}
	}
	
	bn * node = new bn;
	node->client_sd = sd;
	
	if(last_node){
		last_node->next = node;
	}
	last_node = node;

	if(!first_node->next){
		first_node = node;
	}
	
	mutex_flag = 0;
}

// remove Client Sd
void removeCS(int sd){
	while(true){
		// mutex check
		if(!mutex_flag){
			mutex_flag = 1;
			break;
		}
	}

	bn * before_node = nullptr;
	for(bn * idx_node = first_node; ; idx_node = idx_node->next){
		if(idx_node->client_sd == sd){
		
			if(before_node){ 
				before_node->next = idx_node->next;
				if(!idx_node->next){ // last node
					last_node = before_node;
				}
			}
			else{ // first node
				if(!idx_node->next){ // last node
					last_node = nullptr;
					first_node = new bn;
				} else{
					first_node = idx_node->next;
				}
			}
			idx_node->next = nullptr;
			idx_node->client_sd = -1;
			delete idx_node;
			break;
		}
		before_node = idx_node;
		if(!idx_node->next){ // last node
			break;
		}
	}
	
	mutex_flag = 0;
}

void recvThread(int sd) {
	printf("connected\n");
	addCS(sd);
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE];
	while (true) {
		ssize_t res = ::recv(sd, buf, BUFSIZE - 1, 0);
		if (res == 0 || res == -1) {
			fprintf(stderr, "recv return %ld", res);
			perror(" ");
			break;
		}
		buf[res] = '\0';
		printf("%s", buf);
		fflush(stdout);
		if(param.broadcast){
			for(bn * idx_node = first_node; ; idx_node = idx_node->next){
				res = ::send(idx_node->client_sd, buf, res, 0);
				if (res == 0 || res == -1) {
					fprintf(stderr, "send return %ld", res);
					perror(" ");
					break;
				}
				if(!idx_node->next){ // last node
					break;
				}
			}
		}
		else if (param.echo) {
			res = ::send(sd, buf, res, 0);
			if (res == 0 || res == -1) {
				fprintf(stderr, "send return %ld", res);
				perror(" ");
				break;
			}
		}
	}
	printf("disconnected\n");
	removeCS(sd);
	::close(sd);
}

int main(int argc, char* argv[]) {
	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}

#ifdef WIN32
	WSAData wsaData;
	WSAStartup(0x0202, &wsaData);
#endif // WIN32

	int sd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return -1;
	}

	int res;
#ifdef __linux__
	int optval = 1;
	res = ::setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res == -1) {
		perror("setsockopt");
		return -1;
	}
#endif // __linux

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(param.port);

	ssize_t res2 = ::bind(sd, (struct sockaddr *)&addr, sizeof(addr));
	if (res2 == -1) {
		perror("bind");
		return -1;
	}

	res = listen(sd, 5);
	if (res == -1) {
		perror("listen");
		return -1;
	}

	while (true) {
		struct sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);
		int cli_sd = ::accept(sd, (struct sockaddr *)&cli_addr, &len);
		if (cli_sd == -1) {
			perror("accept");
			break;
		}
		std::thread* t = new std::thread(recvThread, cli_sd);
		t->detach();
	}
	::close(sd);
}
