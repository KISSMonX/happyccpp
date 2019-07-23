
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <linux/can.h>
#include <net/if.h>
#include <linux/can/raw.h>

#define PF_CAN 29

#define LOOP 100

#define CAN_ID_DEFAULT	(2)

int init_can(char *name) {
	
	int skt;
	struct ifreq ifr;
	struct sockaddr_can addr;
	
	if ((skt = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) 
	{
		perror("socket");
		return -1;
	}

	addr.can_family = PF_CAN;
	strcpy(ifr.ifr_name, name);
	ioctl(skt, SIOCGIFINDEX, &ifr);
	addr.can_ifindex = ifr.ifr_ifindex;
	
	if (bind(skt, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return -1;
	}
	return skt;
}


int loop_can(int s1 , int s2 , struct can_frame frame){
		struct can_frame frame2;
		int ret,i,nbytes;
			
		ret = write(s1, &frame, sizeof(frame));
		if(ret < 0 ) {
			perror("can write");
			return -1;
		}
		
		usleep(20000);
		
		memset(frame2.data , 0 , sizeof(frame2.data));
		
		if ((nbytes = read(s2, &frame2, sizeof(frame2))) < 0) 
		{
			perror("can read");
			return -1;
		}
		
		if(frame2.can_id != frame.can_id) {
			printf("\nSend : [%03x] %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n"
				,frame.can_id
				,frame.data[0],frame.data[1],frame.data[2],frame.data[3]
				,frame.data[4],frame.data[5],frame.data[6],frame.data[7]);
			printf("Recv : [%03x] %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n"
				,frame2.can_id
				,frame2.data[0],frame2.data[1],frame2.data[2],frame2.data[3]
				,frame2.data[4],frame2.data[5],frame2.data[6],frame2.data[7]);
			return -1;
					
		}else{
			for (i = 0; i < frame2.can_dlc; i++) 
			{
				if(frame2.data[i] != frame.data[i]){
					printf("\nSend : [%03x] %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n"
						,frame.can_id
						,frame.data[0],frame.data[1],frame.data[2],frame.data[3]
						,frame.data[4],frame.data[5],frame.data[6],frame.data[7]);
					printf("Recv : [%03x] %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n"
						,frame2.can_id
						,frame2.data[0],frame2.data[1],frame2.data[2],frame2.data[3]
						,frame2.data[4],frame2.data[5],frame2.data[6],frame2.data[7]);
					
						i = frame2.can_dlc + 1;
						return -1;	
				}
			}
			
		}
		
		fflush(stdout);
	return 0;
}

int main(int argc, char **argv)
{
	static int skt = -1 , skt2 = -1,extended=1;
	int err = 0 , j , k , id ,loop;
	unsigned int maxid;
	struct can_frame frame = {
		.can_dlc = 1,
	};
	struct can_filter filter[] = {
		{
			.can_id = CAN_ID_DEFAULT,
		},
	};
	
	skt=init_can("can0");
	if(skt < 0) {
		printf("Error Init can0\n");
		return 0;
	}
	
	skt2=init_can("can1");
	if(skt2 < 0) {
		printf("Error Init can1\n");
		return 0;
	}
	
	srand(time(NULL));
	
	if(extended){
		maxid = 0x1FFFFFFF;
		j = 2000;
	}
	else{
		maxid = 0x7ff;
		j = 0;
	}
	loop = j + LOOP;
	
	for(; j < loop ; j++) {
		
		for( id = 0x1 + (0x1 * j) ; id > maxid ; id = id - maxid)
			;
		filter->can_id = id;
		if (extended) {
			filter->can_mask = CAN_EFF_MASK;
			filter->can_id  &= CAN_EFF_MASK;
			filter->can_id  |= CAN_EFF_FLAG;
		} else {
			filter->can_mask = CAN_SFF_MASK;
			filter->can_id  &= CAN_SFF_MASK;
		}
		frame.can_id = filter->can_id;
		
		printf("\rSend ID: [%03x]",frame.can_id);
		
		for(k = 0 ; k <= 7 ; k++) {
			frame.data[k] = 0x1 * (rand()%99) ;
		}
		frame.can_dlc = 8;
		
		if (setsockopt(skt, SOL_CAN_RAW, CAN_RAW_FILTER, filter, sizeof(filter))) {
			perror("Can0:setsockopt");
			exit(EXIT_FAILURE);
		}
		
		if (setsockopt(skt2, SOL_CAN_RAW, CAN_RAW_FILTER, filter, sizeof(filter))) {
			perror("Can1:setsockopt");
			exit(EXIT_FAILURE);
		}
		
		if(loop_can(skt , skt2 , frame) < 0)
			err++;
		
		usleep(10000);
		
		if(loop_can(skt2 , skt , frame) < 0)
			err++;
	}
	printf("\nTest CanBus Loop %d Times : ",LOOP);
	if(err == 0)
		printf("OK\n");
	else
		printf("FAIL\n");
		
	close(skt);
	close(skt2);

	return 0;
}