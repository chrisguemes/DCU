#include "hal_utils.h"

#include "ifaceAdp_api.h"
#include "ifaceSniffer.h"

void usi_host_init()
{
	hal_usi_init();
	ifaceAdp_api_init();
	sniffer_init();
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

void usi_host_select_api(uint8_t app_id)
{
	ifaceAdp_select_api(app_id);
}

void usi_host_set_sniffer_cb(void (*sap_handler)(uint8_t* msg, uint16_t len))
{
	sniffer_set_cb(sap_handler);
}

