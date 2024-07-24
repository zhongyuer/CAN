/*********************************************************************************
 *      Copyright:  (C) 2024 LingYun IoT System Studio
 *                  All rights reserved.
 *
 *       Filename:  mqtt.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(21/07/24)
 *         Author:  LingYun <iot@gmail.com>
 *      ChangeLog:  1, Release initial version on "21/07/24 17:08:18"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <mosquitto.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>


typedef struct conn_para_s
{
	char	*hostname;	//服务器地址
	char	*pub_topic;	//发布消息主题
	char	*clientid;	//客户端id
	char	*username;	//用户名
	char	*password;	//密码
	int		port;		//服务器监听的端口
	char	*sub_topic;	//订阅的主题
	int		keepalive;	//活跃时间
	int		qos;		//消息服务质量等级
}conn_para_t;

int		g_flag = 0;

void print_usage(const char *progname);
void send_can_message(const char *ifname, struct can_frame frame);
void receive_can_message(const char *ifname, char *temp_humd, size_t size);
void mqtt_clean(struct mosquitto *mosq);
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);

int main(int argc, char **argv)
{
	conn_para_t     conn_para = 
	{   
		"iot-06z00bn9wpral22.mqtt.iothub.aliyuncs.com",
		"/sys/k1lgp8fSYGM/device1/thing/event/property/post",
		"k1lgp8fSYGM.device1|securemode=2,signmethod=hmacsha256,timestamp=1721711367927|",
		"device1&k1lgp8fSYGM",
		"34aaeb2f25702db82fb8fa7e3f2e3c0581eb414c6b0bae1567b26fdc677d2b06",
		1883,
		"/k1lgp8fSYGM/device1/user/get",
		60, 
		0   
	}; 

	struct mosquitto    *mosq = NULL;
	struct can_frame	frame;
	int                 mid, mid1;
	int                 retain = 0;
	int					opt = 0;
	int					index = 0;
	const char	 		*ifname = NULL;
	char				temp_humd[256];

	static struct option long_options[] =
	{
		{"interface", required_argument, 0, 'i'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "i:h", long_options, &index)) != -1)
	{
		switch(opt)
		{
			case 'i':
				ifname = optarg;
				break;
			case'h':
				print_usage(argv[0]);
				return 0;
			default:
				print_usage(argv[0]);
				return 1;
		}
	}

	if(ifname == NULL)
	{   
		print_usage(argv[0]);
		return 1;
	}

	mosquitto_lib_init();

	mosq = mosquitto_new(conn_para.clientid, true, NULL);

	if(mosquitto_username_pw_set(mosq, conn_para.username, conn_para.password) !=           MOSQ_ERR_SUCCESS)
	{
		printf("connect broker failure: %s\n", strerror(errno));
		goto cleanup;
	}
	printf("mqtt connect parameters set OK\n");

	if(mosquitto_connect(mosq, conn_para.hostname, conn_para.port, conn_para.keepalive) !=  MOSQ_ERR_SUCCESS)
	{
		printf("connect broker failure: %s\n", strerror(errno));
		goto cleanup;
	}
	printf("connect successfully!\n");

	if(mosquitto_loop_start(mosq) != MOSQ_ERR_SUCCESS)
	{
		printf("mosqiuitto_loop_start failure: %s\n", strerror(errno));
		goto cleanup;
	}   

	mosquitto_message_callback_set(mosq, my_message_callback);

	if(mosquitto_subscribe(mosq, &mid, conn_para.sub_topic, conn_para.qos) != MOSQ_ERR_SUCCESS)
	{
		printf("mosquitto subscribe failure: %s\n", strerror(errno));
		goto cleanup;
	}
	printf("subscribe topic successfully!\n");
	
	while(1)
	{
		if(g_flag == 1)
		{
			printf("Led turn on\n");
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

			send_can_message(ifname, frame);
			g_flag = 0;
		}

		if(g_flag == 2)
		{
			printf("Led turn off\n");
			frame.can_id = 0x123;
			frame.can_dlc = 8;
			frame.data[0] = 0x10;
			frame.data[1] = 0x22;
			frame.data[2] = 0x33;
			frame.data[3] = 0x44;
			frame.data[4] = 0x55;
			frame.data[5] = 0x66;
			frame.data[6] = 0x77;
			frame.data[7] = 0x88;

			send_can_message(ifname, frame);
			g_flag = 0;
		}

		if(g_flag == 3)
		{
			printf("start sht20 sample data\n");
			frame.can_id = 0x123;
			frame.can_dlc = 8;
			frame.data[0] = 0x10;
			frame.data[1] = 0x20;
			frame.data[2] = 0x33;
			frame.data[3] = 0x44;
			frame.data[4] = 0x55;
			frame.data[5] = 0x66;
			frame.data[6] = 0x77;
			frame.data[7] = 0x88;

			send_can_message(ifname, frame);			

			while(g_flag == 3) 
			{
				receive_can_message(ifname, temp_humd, sizeof(temp_humd));

				if(mosquitto_publish(mosq, &mid1, conn_para.pub_topic, strlen(temp_humd), temp_humd, conn_para.qos, retain) != MOSQ_ERR_SUCCESS)
				{	   
					printf("mosquitto publish data failure: %s\n", strerror(errno));
					continue;
				}
				printf("data: %s\n", temp_humd);
			}
		}

		if(g_flag == 4)
		{
			printf("stop sht20 sample data\n");
			frame.can_id = 0x123;
			frame.can_dlc = 8;
			frame.data[0] = 0x10;
			frame.data[1] = 0x20;
			frame.data[2] = 0x30;
			frame.data[3] = 0x44;
			frame.data[4] = 0x55;
			frame.data[5] = 0x66;
			frame.data[6] = 0x77;
			frame.data[7] = 0x88;

			send_can_message(ifname, frame);
			g_flag = 0;
		}	


	}

	return 0;

cleanup:
	mqtt_clean(mosq);
	return 0;

}

void print_usage(const char *progname)
{
	printf("Usage: %s -i <can_interface>\n", progname);
	printf("Options:\n");
	printf(" -i, --interface   CAN interface(e.g., can0)\n");
	printf(" -h, --help        Display this message\n");
}

void send_can_message(const char *ifname, struct can_frame frame)
{
	int       			  	fd;
	struct sockaddr_can		addr;
	struct ifreq        	ifr;

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

	if(write(fd, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame))
	{
		perror("write");
		exit(1);
	}

	close(fd);
}

void receive_can_message(const char *ifname, char *temp_humd, size_t size)
{
	int							fd;
	int							nbytes = 0;
	struct sockaddr_can 		addr;
	struct ifreq        		ifr;
	struct can_frame    		frame;

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

	nbytes = read(fd, &frame, sizeof(struct can_frame));
	if(nbytes < 0)
	{
		fprintf(stderr, "read: incomlete CAN frame\n");
		exit(1);
	}

	snprintf(temp_humd, size, "{\"params\":{\"Temperature\":%d.%d, \"Humidity\":%d.%d}}", frame.data[4], frame.data[5], frame.data[6], frame.data[7]);

	//printf("Receive CAN frame: ID:=0x%X, DLC=%d data=", frame.can_id, frame.can_dlc);
	printf("%d.%d; %d.%d\n", frame.data[4], frame.data[5], frame.data[6], frame.data[7]);
	printf("temperature and humidity: %s\n", temp_humd);

	close(fd);
}

void mqtt_clean(struct mosquitto *mosq)
{
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup;
}

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	printf("subscribe message: %s\n", (char *)msg->payload);

	if(strcmp((char *)msg->payload, "led turn on") == 0)
	{
		g_flag = 1;
	}

	if(strcmp((char *)msg->payload, "led turn off") == 0)
	{
		g_flag = 2;
	}

	if(strcmp((char *)msg->payload, "start sample data") == 0)
	{
		g_flag = 3;
	}

	if(strcmp((char *)msg->payload, "stop sample data") == 0)
	{
		g_flag = 4;
	}
}
