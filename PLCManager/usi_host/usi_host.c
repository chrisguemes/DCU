#include "hal_utils.h"

#include "ifacePrime_api.h"
#include "ifacePrimeSniffer.h"
#include "ifaceMngLayer.h"

void usi_host_init()
{
	hal_usi_init();
	mngLay_Init();
	ifacePrime_api_init();
	prime_sniffer_init();
}

int usi_host_open(char *sz_tty_name, unsigned ui_baudrate)
{
	return hal_usi_open(sz_tty_name, ui_baudrate);
}

void usi_host_process(void)
{
	hal_usi_process();
}

usi_status_t usi_host_set_callback(usi_protocol_t protocol_id, uint8_t (*p_handler)(uint8_t *puc_rx_msg, uint16_t us_len))
{
	return hal_usi_set_callback(protocol_id, p_handler);
}

usi_status_t usi_host_send_cmd(void *msg)
{
	return hal_usi_send_cmd(msg);
}

void usi_host_loopback(int _fd_redirect)
{
	hal_usi_loopback(_fd_redirect);
}

void usi_host_select_api(uint8_t prime_app_id)
{
	ifacePrime_select_api(prime_app_id);
}

void usi_host_set_sniffer_cb(void (*sap_handler)(uint8_t* msg, uint16_t len))
{
	prime_sniffer_set_cb(sap_handler);
}

