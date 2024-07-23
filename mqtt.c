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
#include<stdio.h>
#include <errno.h>
#include<string.h>
#include<unistd.h>
#include<mosquitto.h>


typedef struct conn_para_s
{
	char	*hostname;	//服务器地址
	char	*pub_topic;	//发布消息主题
	char	*clientid;	//客户端id
	char	*username;	//用户名
	char	*password;	//密码
	int		port;		//服务器监听的端口
	char	*payload;	//发布的消息内容
	char	*sub_topic;	//订阅的主题
	int		keepalive;	//活跃时间
	int		qos;		//消息服务质量等级
}conn_para_t;


void mqtt_clean(struct mosquitto *mosq);
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);

int main(int argc, char **argv)
{
	struct mosquitto	*mosq = NULL;
	int					mid, mid1;
	int					retain = 0;

	conn_para_t		conn_para = 
	{
		"iot-06z00bn9wpral22.mqtt.iothub.aliyuncs.com",
		"/sys/k1lgp8fSYGM/device1/thing/event/property/post",
		"k1lgp8fSYGM.device1|securemode=2,signmethod=hmacsha256,timestamp=1721703690425|",
		"device1&k1lgp8fSYGM",
		"dfff87903c572c7211e731c84d5b1cfe1e7c855686186c7a17fd9e4d8e831888",
		1883,
		"{\"params\":{\"Temperature\":27.32, \"Humidity\":87.37}}",
		"/k1lgp8fSYGM/device1/user/get",
		60,
		0
	};

	mosquitto_lib_init();

	mosq = mosquitto_new(conn_para.clientid, true, NULL);

	if(mosquitto_username_pw_set(mosq, conn_para.username, conn_para.password) != MOSQ_ERR_SUCCESS)
	{
		printf("connect broker failure: %s\n", strerror(errno));
		goto cleanup;
	}
	printf("mqtt connect parameters set OK\n");

	if(mosquitto_connect(mosq, conn_para.hostname, conn_para.port, conn_para.keepalive) != MOSQ_ERR_SUCCESS)
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
		if(mosquitto_publish(mosq, &mid1, conn_para.pub_topic,  strlen(conn_para.payload), conn_para.payload, conn_para.qos, retain) != MOSQ_ERR_SUCCESS)
		{	
			printf("mosquitto publish data failure: %s\n", strerror(errno));
			continue;
		}
		printf("publish message: %s\n", conn_para.payload);
		sleep(3);
	}

cleanup:
	mqtt_clean(mosq);
	return 0;
}

void mqtt_clean(struct mosquitto *mosq)
{
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup;
}

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	printf("subscribe message: %s\n", (char *)msg->payload);
}
