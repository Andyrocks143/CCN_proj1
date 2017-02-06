#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 5432
#define MAX_LINE 80

int main(int argc, char * argv[])
{
	FILE *fp;
	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	char *fname;
	char buf[MAX_LINE];
	char frame[MAX_LINE + 4];
	int s;
	int slen;
	int seq_num = 1;
	char ack_str[5];
	int ack_recv;
	int flen, ack_len, ack_num;

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	if (argc==3) {
		host = argv[1];
		fname= argv[2];
	}
	else {
		fprintf(stderr, "Usage: ./a.out host filename\n");
		exit(1);
	}
	/* translate host name into peer’s IP address */
	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr, "Unknown host: %s\n", host);
		exit(1);
	}

	fp = fopen(fname, "r");
	if (fp==NULL){
		fprintf(stderr, "Can't open file: %s\n", fname);
		exit(1);
	}

	/* build address data structure */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);

	/* active open */
	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket");
		exit(1);
	}
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("TIMEOUT ERROR");
		exit(1);
	}

	srandom(time(NULL));
	socklen_t sock_len= sizeof sin;

	
	/* main loop: get and send lines of text */
	while(fgets(buf, 80, fp) != NULL){
		slen = strlen(buf);
		buf[slen] ='\0';

		sprintf(frame, "%d\t%s", seq_num, buf);
		flen = strlen(frame);
		
		ack_recv = 0;
		while(!ack_recv) {

        		if(sendto(s, frame, flen+1, 0, (struct sockaddr *)&sin, sock_len)<0){
				perror("SendTo Error\n");
				exit(1);
			}
			ack_len = recvfrom(s, ack_str, sizeof(ack_str), 0, (struct sockaddr*)&sin, &sock_len);
			
			if(ack_len > 0 ) {
				sscanf(ack_str, "%d", &ack_num);
				if (ack_num == seq_num) {
					ack_recv = 1;
				}
			}
		}
		seq_num++;

	}
	*buf = 0x02;	
        if(sendto(s, buf, 1, 0, (struct sockaddr *)&sin, sock_len)<0){
		perror("SendTo Error\n");
		exit(1);
	}
	fclose(fp);
}


