
/* 
* @license Moon Young Ha <https://github.com/hamoonyoung/serial-port>
*
* References:
* https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
* https://www.cmrr.umn.edu/~strupp/serial.html
* https://pbxbook.com/other/mac-tty.html
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include "serial_port.h"

void * send (void * serialPort){
	char inputMsg[255];
	int fd = *((int *) serialPort);
	printf ("Please write a message to be sent. Also, you may receive a message from others here.\n\n");
	while (1){
		fflush(stdin);
		memset(inputMsg, 0, sizeof(inputMsg));
		fgets (inputMsg, sizeof(inputMsg), stdin);
		fcntl(fd, F_SETFL, 0);
		if (write(fd, inputMsg, sizeof(inputMsg)) < 0)
			fputs ("Write()failed!\n", stderr);
	} 
}

void * receive (void * serialPort){
	// Allocate memory for read buffer, set size according to your needs
	char read_buf[255];
	char save_buf[255];

	int fd = *((int *) serialPort);
	int n;
	while (1){
		
		memset(read_buf, 0, sizeof(read_buf));
		memset(save_buf, 0, sizeof(save_buf));
		while((n = read(fd, &read_buf, sizeof(read_buf)))>0) {
			strcat (save_buf, read_buf);
		}
		if (*save_buf != 0){
				printf ("%s", save_buf);
		}

	}
	return NULL;
}

int open_port(char * serialPort)
{
	int fd = open (serialPort, O_RDWR | O_NOCTTY | O_NDELAY);
	//int fd = open (serialPort, O_RDWR | O_NOCTTY);

	printf ("Serial Port: %s, Status: %i\n", serialPort, fd);

	// Check for errors
	if (fd == -1)
	{
		printf ("Unable to open %s", serialPort);
		perror("Error: ");
		printf("\n");
	}

	// If there is no error
	else
	{
		struct termios tty;
		
		// Reading existing settgins
		if (tcgetattr (fd, &tty) != 0) {
			printf ("Error %i from tcgetattr: %s\n", errno, strerror (errno));
		}

		///////////// 
		// c_cflag //
		/////////////
		tty.c_cflag &= ~PARENB;		// Clear parity bit, disabling parity. Or, |= PARENB Set parity bit, enabling parity
		tty.c_cflag &= ~CSTOPB; 	// Clear stop field, only one stop bit used in communication. Or, |= CSTOPB Set stop field, two stop bits used in communication 
		tty.c_cflag &= ~CSIZE;		// Clear all the size bits
		tty.c_cflag |= CS8;			// 8 bits per byte (or CS5, CS6, CS7)
		tty.c_cflag &= ~CRTSCTS;	// Disable RTS/CTS hardware flow control (most common). Or, |= CRTSCTS Enable RTS/CTS hardware flow control
		tty.c_cflag |= CREAD | CLOCAL; 	// Turn on READ & ignore ctrl lines (CLOCAL = 1);


		///////////////////////////
		// c_lflag (Local modes) //
		///////////////////////////

		// In canonical mode, input is processed when a new line character is received.
		tty.c_lflag &= ~ICANON; 	// Canonical mode is disabled 

		tty.c_lflag &= ~ECHO;		// Disable echo
		tty.c_lflag &= ~ECHOE; 		// Disable erasure
		tty.c_lflag &= ~ECHONL; 	// Disable new-line echo

		tty.c_lflag &= ~ISIG; 		// Disable interpretation of INTR, QUIT and SUSP


		///////////////////////////
		// c_iflag (Input modes) //
		///////////////////////////
		tty.c_iflag &= ~(IXON | IXOFF | IXANY); 	// Turn off s/w flow ctrl

		tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);	// Disable any special handling of received bytes


		////////////////////////////
		// c_oflag (Output Modes) //
		////////////////////////////
		tty.c_oflag &= ~OPOST; 		// Prevent special interpretation of output bytes (e.g. newline chars)
		tty.c_oflag &= ~ONLCR; 		// Prevent conversion of newline to carriage return/line feed


		//////////
		// c_cc //
		//////////
		tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
		tty.c_cc[VMIN] = 0;


		// Set in/out baud rate to be 9600
		cfsetispeed(&tty, B9600);
		cfsetospeed(&tty, B9600);


		// Save tty settings, also checking for error
		if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
		}
	}

	return (fd);
}


int main(int argc, char ** argv)
{
	// If you know the path of a serial port, use -p serialPortPath
	int opt;
	if ((opt = getopt (argc, argv, "p:")) != EOF){ 
		switch (opt){
			case 'p':
				open_port(optarg);
				break;

			default:
				fprintf (stderr, "Usage: %p [-] serialPortPath [-l]\n", argv[0]);
				break;
		}
	} 

	// Or you can choose one
	else 
	{
		printf ("Currently available serial ports are:\n");
		DIR *d;
		struct dirent *dir;
		d = opendir ("/dev/");
		char tempbuf[4];
		char fullPath[255];		// Full path of a serial port
		char * devices[20];		// All available serial devices
		int i = 0;

		if (d){
			while ((dir = readdir(d)) != NULL){
				memcpy (tempbuf, dir->d_name, 3);
				tempbuf[3] = '\0';
				if (strcmp(tempbuf,"cu.") == 0){	// cu.* is for MacOS.
					devices[i] = dir->d_name;
					strcpy(fullPath, "/dev/");
					strcat(fullPath, devices[i]);
					strcpy(devices[i], fullPath);
					printf("%d. %s\n", i, devices[i]);
					i++;
				}
			}
			closedir(d);
		}

		int s;
		printf("\nPlease select a serial port: ");
		scanf("%d", &s);

		int serialPort = open_port(devices[s]);

		
		// Receiving messages
		pthread_t thread_receive;
		int rt = pthread_create (&thread_receive, NULL, &receive, &serialPort);
		if (rt){
			printf ("\nERROR: return code from pthread_create is %d \n", rt);
		}else{
			printf ("\nReady to receive a message.\n");
		}

		// Sending messages
		pthread_t thread_send;
		int st = pthread_create (&thread_send, NULL, &send, &serialPort);
		if (rt){
			printf ("\nERROR: return code from pthread_create is %d \n", st);
		}else{
			printf ("\nReady to send a message.\n\n");
		}

		while (1){
			// nothing
		}

	}

	return 0;
}
