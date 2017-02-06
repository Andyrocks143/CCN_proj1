#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>

#define SERVER_PORT 5432
#define MAX_LINE 256
#define SEQ_NO_LEN 5

char parsePacket(char buf[], char payload[]) {
	char seq_no = buf[0];
	int framelen, i;
	framelen = strlen(buf);

	for (i = 1; i < framelen; i++) {
		payload[i-1] = buf[i];
	}
	payload[framelen + 1] = '\0';


	return seq_no;
}


int main(int argc, char * argv[])
{
	char *fname;
        char buf[MAX_LINE];
	char payload[MAX_LINE];
	char ack_string[5];
        struct sockaddr_in sin;
        int len, alen;
        int s, i;
        struct timeval tv;
	char seq_num = 1, highest_seq_num = 0; 
	FILE *fp;

        if (argc==2) {
                fname = argv[1];
        }
        else {
                fprintf(stderr, "usage: ./client_udp filename\n");
        	exit(1);
        }


        /* build address data structure */
        bzero((char *)&sin, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(SERVER_PORT);

        /* setup passive open */
        if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
                perror("simplex-talk: socket");
                exit(1);
        }
        if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
                perror("simplex-talk: bind");
                exit(1);
        }

	socklen_t sock_len = sizeof sin;
	srandom(time(NULL));

	fp = fopen(fname, "w");
        if (fp==NULL){
                printf("Can't open file\n");
                exit(1);
        }
	
	while(1){
		len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&sin, &sock_len);
		printf("Received packet length %d\n", len);
		if(len == -1){
        	    perror("PError");
		}	
		else if(len == 1){
			if (buf[0] == 0x02){
        	    		printf("Transmission Complete\n");
				break;
			}
        	    	else{
				perror("Error: Short packet\n");
			}
		}	
		else if(len > 1){
			seq_num = parsePacket(buf, payload);
			printf("Received sequence number %d\n", (int)seq_num);
			if (seq_num == highest_seq_num + 1) 
			{
				highest_seq_num = seq_num;
				
				if(fputs((char *) payload, fp) < 1){
					printf("fputs() error\n");
				}

				//Send an acknowledgment
				ack_string[0] = seq_num;
				ack_string[1] = '\0';
				alen = strlen(ack_string);
				if (sendto(s, ack_string, alen+1, 0, (struct sockaddr *)&sin, sock_len) < 0) {
					perror("Error in sending an acknowledgement");
					exit(1);
				}
			}
		}

        }
	fclose(fp);
	close(s);
}


