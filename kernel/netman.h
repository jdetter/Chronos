#ifndef _NETMAN_H_
#define _NETMAN_H_

#define HOSTNAME_LEN 128

extern char hostname[HOSTNAME_LEN];

/**
 * Initilize the network manager
 */
int net_init(void);

#endif
