/*
 * main.c
 *
 *  Created on: 22/3/2016
 *      Author: cristian.guemes
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <asm/types.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <unistd.h>

#include "socket_handler.h"
#include "plcmanager.h"
#include "net_info_mng.h"
#include "http_mng.h"
#include "sniffer.h"
#include "usi_host.h"
#include "hal_utils.h"
#include "fu_mng.h"
#include "tools.h"

#include <time.h>
#include <sys/utsname.h>

#define MAIN_DEBUG_CONSOLE

#ifdef MAIN_DEBUG_CONSOLE
#	define LOG_MAIN_DEBUG(a)   printf a
#else
#	define LOG_MAIN_DEBUG(a)   (void)0
#endif

static char app_name[] = "PLC Manager";
static char app_version[] = "v1.0.0";

typedef struct {
	char sz_tty_name[255];
	unsigned ui_baudrate;
} x_serial_args_t;

/* Manage Callbacks of applications */
static pf_app_callback_t app_cbs[PLC_MNG_MAX_APP_ID];

static int _parse_arguments(int argc, char** argv, x_serial_args_t *serial_args)
{
	int c;

	while (1)
	{
	   c = getopt (argc, argv, "t:b:");

	   /* Detect the end of the options. */
	   if (c == -1) {
		   break;
	   }

	   switch (c)
	   {
	   		case 'b':
	   			serial_args->ui_baudrate = atoi(optarg);
	   		   	break;

	   		case 't':
	   			strncpy(serial_args->sz_tty_name, optarg, 255);
	   			break;

	   		default:
	   			/* getopt_long already printed an error message. */
	   			return -1;
	   			break;
           }
    }
	return 0;
}

static void _report_app_cfg(void)
{
	char puc_ln_buf[50];
	int i_ln_len, i_size_fd;
	int fd;
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	struct utsname unameData;

	/* remove all files to update them */
	system("rm /home/cfg/*");

	// G3 version
	fd = open("/home/cfg/appname", O_RDWR|O_CREAT, S_IROTH|S_IWOTH|S_IXOTH);
	i_ln_len = sprintf(puc_ln_buf, app_name);
	i_size_fd = write(fd, puc_ln_buf, i_ln_len);
	close(fd);

	fd = open("/home/cfg/appversion", O_RDWR|O_CREAT, S_IROTH|S_IWOTH|S_IXOTH);
	i_ln_len = sprintf(puc_ln_buf, app_version);
	i_size_fd = write(fd, puc_ln_buf, i_ln_len);
	close(fd);

	fd = open("/home/cfg/startuptime", O_RDWR|O_CREAT, S_IROTH|S_IWOTH|S_IXOTH);
	i_ln_len = sprintf(puc_ln_buf, "%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec );
	i_size_fd = write(fd, puc_ln_buf, i_ln_len);
	close(fd);

	uname(&unameData);
	fd = open("/home/cfg/sysname", O_RDWR|O_CREAT, S_IROTH|S_IWOTH|S_IXOTH);
	i_ln_len = sprintf(puc_ln_buf, unameData.sysname);
	i_size_fd = write(fd, puc_ln_buf, i_ln_len);
	close(fd);
	fd = open("/home/cfg/sysrelease", O_RDWR|O_CREAT, S_IROTH|S_IWOTH|S_IXOTH);
	i_ln_len = sprintf(puc_ln_buf, unameData.release);
	i_size_fd = write(fd, puc_ln_buf, i_ln_len);
	close(fd);
	fd = open("/home/cfg/sysversion", O_RDWR|O_CREAT, S_IROTH|S_IWOTH|S_IXOTH);
	i_ln_len = sprintf(puc_ln_buf, unameData.version);
	i_size_fd = write(fd, puc_ln_buf, i_ln_len);
	close(fd);
	fd = open("/home/cfg/sysmachine", O_RDWR|O_CREAT, S_IROTH|S_IWOTH|S_IXOTH);
	i_ln_len = sprintf(puc_ln_buf, unameData.machine);
	i_size_fd = write(fd, puc_ln_buf, i_ln_len);
	close(fd);
	fd = open("/home/cfg/sysnodename", O_RDWR|O_CREAT, S_IROTH|S_IWOTH|S_IXOTH);
	i_ln_len = sprintf(puc_ln_buf, unameData.nodename);
	i_size_fd = write(fd, puc_ln_buf, i_ln_len);
	close(fd);
	fd = open("/home/cfg/gprs_en", O_RDWR|O_CREAT, S_IROTH|S_IWOTH|S_IXOTH);
	i_ln_len = sprintf(puc_ln_buf, "0");
	i_size_fd = write(fd, puc_ln_buf, i_ln_len);
	close(fd);
	fd = open("/home/cfg/sniffer_en", O_RDWR|O_CREAT, S_IROTH|S_IWOTH|S_IXOTH);
	i_ln_len = sprintf(puc_ln_buf, "0");
	i_size_fd = write(fd, puc_ln_buf, i_ln_len);
	close(fd);

}

int main(int argc, char** argv)
{
	socket_res_t i_socket_res;
	int pi_usi_fds;
	x_serial_args_t serial_args; // = {"/dev/ttyUSB2", 115200}; //{"/dev/ttyS0", 115200};

	/* Load command line parameters */
	if (_parse_arguments(argc,argv, &serial_args) < 0) {
		printf("PLC Manager v%d,%d\n", PLC_MNG_VERSION_HI, PLC_MNG_VERSION_LO);
		printf("Error, check arguments.\n");
		printf("\t-t tty                : tty device connecting to a base node, default: /dev/ttyUSB0 \n");
		printf("\t-b baudrate           : tty baudrate configuration, default: 115200\n");
		exit(-1);
	}

	_report_app_cfg();

	system("killall pppd");
	sleep(2);

	/* Init callbacks for applications */
	memset(&app_cbs, 0, sizeof(app_cbs));

	/* Init sockets */
	socket_init();

	/* Init Tools module (Init GPIOs) */
	tools_init();

	/* Open SNIFFER server socket */
	i_socket_res =  socket_create_server(PLC_MNG_SNIFFER_APP_ID, INADDR_ANY, PLC_MNG_SNIFFER_APP_PORT);
	if (i_socket_res == SOCKET_ERROR) {
		LOG_MAIN_DEBUG(("Cannot open Sniffer Server socket."));
		exit(-1);
	}

//	/* Open ADP internal server socket */
//	i_socket_res =  socket_create_server(PLC_MNG_NET_INFO_APP_ID, INADDR_ANY, PLC_MNG_NET_INFO_APP_PORT);
//	if (i_socket_res == SOCKET_ERROR) {
//		LOG_MAIN_DEBUG(("Cannot open Net Info Manager internal Server socket."));
//		exit(-1);
//	}

	/* Open HTTP internal NODE server socket */
	i_socket_res =  socket_create_server(PLC_MNG_HTTP_MNG_APP_ID, INADDR_LOOPBACK, PLC_MNG_HTTP_MNG_APP_PORT);
	if (i_socket_res == SOCKET_ERROR) {
		LOG_MAIN_DEBUG(("Cannot open HTTP internal NODE Server socket."));
		exit(-1);
	}

	/* USI_HOST serial connection. */
	usi_host_init();
	pi_usi_fds = usi_host_open(serial_args.sz_tty_name, serial_args.ui_baudrate);
	if (pi_usi_fds == SOCKET_ERROR) {
		LOG_MAIN_DEBUG(("Cannot open Serial Port socket\n"));
		exit(-1);
	} else {
		/* Add listener to USI port */
		socket_attach_connection(PLC_MNG_USI_APP_ID, pi_usi_fds);
	}

	/* Register SNIFFER APP callback */
	app_cbs[PLC_MNG_SNIFFER_APP_ID] = sniffer_callback;
	/* Init Sniffer APP : Serve to SNIFFER tool. */
	sniffer_init(PLC_MNG_SNIFFER_APP_ID, pi_usi_fds);

//	/* Register Net Info Manager APP callback */
//	app_cbs[PLC_MNG_NET_INFO_APP_ID] = net_info_mng_callback;
	/* Init NET INFO app through USI interface */
	net_info_mng_init(PLC_MNG_NET_INFO_APP_ID);

	/* Register HTTP internal NODE APP callback */
	app_cbs[PLC_MNG_HTTP_MNG_APP_ID] = http_mng_callback;
	/* Init HTTP client manager to connect with NODE server -> Send data to NetInfo */
	http_mng_init();

	/* Init Firmware Upgrade Manager */
	fu_mng_init(PLC_MNG_FU_APP_ID);

	while(1) {
		int i_res;
		socket_ev_info_t socket_evet_info;

		/* Wait on SOCKETs for data available */
		i_res = socket_select(&socket_evet_info);

		switch (i_res) {
				case SOCKET_TIMEOUT:
					/* Process USI */
					usi_host_process();
					//http_mng_send_cmd();
					net_info_mng_process();
					break;

				case SOCKET_ERROR:
					/* Reset sockets */
					//socket_restart();
					/* Reset applications */
					/* ... */
					break;

				case SOCKET_SUCCESS:
					/* Send event to APP */
					if (socket_evet_info.i_app_id == PLC_MNG_USI_APP_ID) {
						/* Process USI */
						usi_host_process();
						net_info_mng_process();
					} else {
						/* Launch APP callback */
						if (app_cbs[socket_evet_info.i_app_id] != NULL) {
							app_cbs[socket_evet_info.i_app_id](&socket_evet_info);
						}
					}
					break;
		}
	}

	return 0;
}
