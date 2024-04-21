#ifndef NETWORK_SERVICE_H_
#define NETWORK_STATUS_H_

typedef void (*network_connected_cb)(void);
typedef void (*network_disconnected_cb)(void);

extern void init_network_status(network_connected_cb on_network_connected,
                                network_disconnected_cb on_network_disconnected);
extern void network_status_give();
extern void network_status_take();

#endif  // NETWORK_STATUS_H_