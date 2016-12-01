/*
    Simple udp server
*/
#include "server.h" 

struct sockaddr_in si_me, si_other;
int s, slen = sizeof(si_other) , recv_len;
Byte status = XON;          // XON atau XOFF
unsigned send_xon = 0,
send_xoff = 0;

static int indexBuffer = 0;
static int front = 0;
static int rear = 0;

void die(char *s)
{
    perror(s);
    exit(1);
}
 
void *childRProcess(void *threadid) {
    static int counter = 1;
    char b[1];
    char message[BUFLEN];
    char aceka[2];
    int i;

    while (1) {

        //Hanya mengonsumsi buffer jika tidak 0
        if (indexBuffer > 0) {
            memset(message,'\0', BUFLEN);
            memset(aceka,'\0', 2);
            aceka[0]=ACK+'0';
            aceka[1]=bufer[front].data[BUFLEN];
            aceka[2]='\0';

            
            for (i=0;i<BUFLEN;i++) {
                message[i] = bufer[front].data[i];
            }
            message[BUFLEN]='\0';

            printf("Mengkonsumsi frame ke-%d : %s\n", counter++, message);

            //Mengirim ACK
            if(sendto(s, aceka, 2, 0, (struct sockaddr *) &si_other, slen) < 0)
                error("ERROR: Gagal mengirim ACK.\n");

            memset(bufer[front].data,'\0', BUFLEN+1);
            front++;
            if(front == 8)
                front=0;
            indexBuffer--;
        }


        if (indexBuffer < MAX_LOWERLIMIT && status == XOFF) {
            printf("Buffer < maximum lowerlimit\n");
            printf("Mengirim XON\n");
            send_xon = 1; send_xoff = 0;

            b[0] = status = XON;
            if(sendto(s, b, 1, 0, (struct sockaddr *) &si_other, slen) < 0)
                error("ERROR: Gagal mengirim XON.\n");
        }   

        sleep(2);
    }
    pthread_exit(NULL);
} 

int main(int argc, char *argv[]) {
    pthread_t thread[1];
    int i;
    char b[1];
    char buf[BUFLEN+1];
    static int counter = 1;


    if (argc<2) {
        // argumen pemanggilan program kurang
        printf("Usage: %s <port>\n", argv[0]);
        return 0;
    }

    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
    
    printf("Membuat socket lokal pada port %s...\n", argv[1]); 
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(atoi(argv[1]));
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }

    // membuat child
    if(pthread_create(&thread[0], NULL, childRProcess, 0)) 
        error("ERROR: Gagal membuat proses anak");
     
    //keep listening for data
    while(1)
    {
        fflush(stdout);
        bzero(buf, BUFLEN+1);

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, buf, BUFLEN+1, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }

        //input data ke buffer
        if (indexBuffer < BUFLEN) {
            memset(bufer[rear].data,'\0', BUFLEN+1);
            for (i=0;i<BUFLEN+1;i++) {
                bufer[rear].data[i] = '\0';
            }
            strncpy(bufer[rear].data,buf,BUFLEN+1);
            bufer[rear].data[BUFLEN+1]='\0';
            rear++;
            if(rear == 8)
                rear=0;
            indexBuffer++;
        }

        // mengirim sinyal XOFF jika buffer mencapai Minimum Upperlimit
        if(indexBuffer >= (MIN_UPPERLIMIT) && status == XON) {
            printf("Buffer > minimum upperlimit\n");
            printf("Mengirim XOFF.\n");
            send_xoff = 1; send_xon = 0;
            b[0] = status = XOFF;

            if(sendto(s, b, 1, 0,(struct sockaddr *) &si_other, slen) < 0)
                error("ERROR: Gagal mengirim XOFF.\n");
        }

         
    }
    
    printf("Data received\n");
    printf("Now close Connections\n");

    close(s);
    return 0;
}