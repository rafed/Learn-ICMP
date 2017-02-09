#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <inttypes.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>


unsigned short checksumCalc(unsigned short *addr, int len)
{
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(unsigned char *) (&answer) = *(unsigned char *) w;
		sum += answer;
	}
	
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return answer;
}

int main(int argc, char **argv)
{
	uint8_t *packet;
	packet = malloc(500);
	
	struct ip ip;
	ip.ip_hl = 0x5;
	ip.ip_v = 0x4;
	ip.ip_tos = 0x0;
	ip.ip_len = htons(60);
	ip.ip_id = htons(12830);
	ip.ip_off = 0x0;
	ip.ip_ttl = 1;
	ip.ip_p = IPPROTO_ICMP;
	ip.ip_sum = 0x0;
	ip.ip_src.s_addr = inet_addr("192.168.1.180");
	ip.ip_dst.s_addr = inet_addr("8.8.8.8");
	ip.ip_sum = checksumCalc((unsigned short *)&ip, sizeof(ip));
	
	memcpy(packet, &ip, sizeof(ip));
	
	struct icmp icmp;	
	icmp.icmp_type = 8;	// 8 means ICMP_ECHO
	icmp.icmp_code = 0;
	icmp.icmp_id = 1000;
	icmp.icmp_seq = 0;
	
	char icmpData[500];
	char msg[500];
	printf("Enter the data you want to send: \n");
	fgets(msg, 499, stdin);
	
	// first: doing it to calculate checksum
	memcpy(icmpData, &icmp, 8);
	memcpy(icmpData + 8, msg, strlen(msg));
	
	icmp.icmp_cksum = checksumCalc((short unsigned int*)&icmpData, 8 + strlen(msg));
	
	// second: doing it to include the checksum in the packet this time
	memcpy(icmpData, &icmp, 8);
	memcpy(icmpData + 8, msg, strlen(msg));
	
	memcpy(packet + 20, icmpData, 8 + strlen(msg));
	
	int sock;
	if ((sock = socket(AF_INET, SOCK_RAW, 1)) < 0) {		// 1 for icmp
		perror("Error creating socket. Are you root? \n");
		exit(1);
	}
	
	const int on = 1;
	if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
		perror("Couldn't set socket options. \n");
		exit(1);
	}

	struct sockaddr_in send; 
	//memset(&sin, 0, sizeof(sin));
	send.sin_family = AF_INET;
	send.sin_port = 0;
	send.sin_addr.s_addr = ip.ip_dst.s_addr;
	
	struct sockaddr receive;
	uint32_t receiveSize;
	uint32_t *receivePacket = malloc(500);
	
	int i;
	for(i=0; i<5; i++){
		if (sendto(sock, packet, 60, 0, (struct sockaddr *)&send, sizeof(struct sockaddr)) < 0)  {
			perror("Failed to send. \n");
			exit(1);
		}
		
		int response = recvfrom(sock, receivePacket, sizeof(receivePacket), 0, &receive, &receiveSize);
		
		if(response > 0)
			printf("received a packet! size: %d \n", receiveSize);

	}
	
	return 0;
}
