#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include<errno.h>
//#include "led.h"

//#define ADC_A 340
#define ADC_CENTER 515
//#define ADC_B 600

void dec(int fd)//CS-GPQ3  UD-GPQ2 INC-GPQ1    011 --> 010  INC下降沿开始计数
{
	ioctl(fd,1,5);
	usleep(10);
	ioctl(fd,1,4);
	//usleep(100);
	//ioctl(fd,1,1);
	//usleep(100);
	//ioctl(fd,1,17);
	//usleep(100);
}

void inc(int fd)//CS-GPQ3  UD-GPQ2 INC-GPQ1    001 --> 000  INC下降沿开始计数
{
	
	ioctl(fd,1,1);
	usleep(10);
	ioctl(fd,1,0);
	//usleep(100);
	//ioctl(fd,1,1);
	//usleep(100);
	//ioctl(fd,1,17);
	//usleep(100);
}

int main()
{
	int adc_fd,led_fd;
	int ret,value;
	int i;
	
	adc_fd = open("/dev/myadc",O_RDONLY);
	if(-1 == adc_fd)
	{
		perror("open adc error");
		exit(1);
	}

	led_fd = open("/dev/three_gpio",O_RDWR);// /dev/led io驱动对应的文件
	if(led_fd<0)
	{
		perror("open led error");
		exit(1);
	}
	
	while(1)
	{
		ret=read(adc_fd,&value,sizeof(value));
		if(ret!=sizeof(value))
			{
				perror("read ADC:");
				close(adc_fd);
				exit(EXIT_FAILURE);
			}
		printf("read from ADC:%d\n",value);
		if(value>(ADC_CENTER+7))
			{
				inc(led_fd);//增加电位器阻值
			}
		else if(value<(ADC_CENTER-7))
			{ 
				dec(led_fd);//减小电位器阻值
			}
	}
	return 0;
}
