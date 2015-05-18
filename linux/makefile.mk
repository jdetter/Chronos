li_proxy:
	$(CC) $(CFLAGS) -c -o linux/li_proxy.o linux/li_proxy.c

li_clean:
	rm -rf linux/li_proxy.o	
