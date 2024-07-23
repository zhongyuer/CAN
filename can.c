#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

void print_usage(const char *progname);
void send_can_message(const char *ifname);
void receive_can_message(const char *ifname);


int main(int argc, char **argv)
{
	int		opt, index = 0;
	const char	*ifname = NULL;
	const char	*mode = NULL;

	static struct option long_options[] = 
	{
		{"interface", required_argument, 0, 'i'},
		{"mode", required_argument, 0, 'm'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "i:m:h", long_options, &index)) != -1)
	{
		switch(opt)
		{
			case 'i':
				ifname = optarg;
				break;
			case'm':
				mode = optarg;
				break;
			case'h':
				print_usage(argv[0]);
				return 0;
			default:
				print_usage(argv[0]);
				return 1;
		}
	}
	if(ifname == NULL || mode == NULL)
	{
		print_usage(argv[0]);
		return 1;
	}

	if(strcmp(mode, "send") == 0)
	{
		send_can_message(ifname);
	}

	else if(strcmp(mode, "receeive") == 0)
	{
		receive_can_message(ifname);
	}
	else
	{
		fprintf(stderr, "Invalid mode: %s\n", mode);
		print_usage(argv[0]);
		return 1;
	}

	return 0;
}	

void print_usage(const char *progname)
{
	printf("Usage: %s -i <can_interface>, -m <mode>\n", progname);
	printf("Options:\n");
	printf(" -i, --interface   CAN interface(e.g., can0)\n");
	printf(" -m, --mode	   Mode: send or receive\n");
	printf(" -h, --help 	   Display this message\n");
}

void send_can_message(const char *ifname)
{
	int			fd;
	struct sockaddr_can	addr;
	struct ifreq		ifr;
	struct can_frame	frame;

	fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(fd < 0)
	{
		perror("socket");
		exit(1);
	}

	strcpy(ifr.ifr_name, ifname);
	ioctl(fd, SIOCGIFINDEX, &ifr);
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("bind");
		exit(1);
	}

	frame.can_id = 0x123;
	frame.can_dlc = 8;
	frame.data[0] = 0x11;
	frame.data[1] = 0x22;
	frame.data[2] = 0x33;
	frame.data[3] = 0x44;
	frame.data[4] = 0x55;
	frame.data[5] = 0x66;
	frame.data[6] = 0x77;
	frame.data[7] = 0x88;
	

	if(write(fd, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame))
	{
		perror("write");
		exit(1);
	}

	close(fd);
}

void receive_can_message(const char *ifname)
{
	int	 		fd;
	struct sockaddr_can	addr;
	struct ifreq		ifr;
	struct can_frame	frame;
	
	fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(fd < 0)
	{
		perror("socket");
		exit(1);
	}

	strcpy(ifr.ifr_name, ifname);
	ioctl(fd, SIOCGIFINDEX, &ifr);
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("bind");
		exit(1);
	}

	while(1)
	{
		int nbytes = read(fd, &frame, sizeof(struct can_frame));
		if(nbytes < 0)
		{
			fprintf(stderr, "read: incomlete CAN frame\n");
			exit(1);
		}

		printf("Receive CAN frame: ID:=0x%X, DLC=%d data=", frame.can_id, frame.can_dlc);
		for(int i = 0; i < frame.can_dlc; i++)
			printf("%02X", frame.data[i]);

		printf("\n");
	}

	close(fd);

}
