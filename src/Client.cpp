#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

using namespace std;

int main(int argc, char** argv) {
    if(argc < 2) {
        cout << "Please input IP and port!" << endl;
    }
    const char* ip = argv[1];
	int port = atoi(argv[2]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);
	
    int ret = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);
    char sendMsg[512];
    char recvMsg[512];
    while(true) {
        cout << "Input: ";
        cin >> sendMsg;
        if(sizeof(sendMsg) > 0) {
            send(sockfd, sendMsg, sizeof(sendMsg), 0);
        }
        sendMsg[512] = {'\0'};
        recv(sockfd, recvMsg, 512, 0);
        cout << "Recv: " << recvMsg << endl;
	recvMsg[512] = {'\0'};
    }

    return 0;
}

