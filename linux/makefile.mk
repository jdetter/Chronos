LINUX_CLEAN := linux/li_proxy.o

li_proxy:
	$(CC) $(CFLAGS) -c -o linux/li_proxy.o linux/li_proxy.c
