#include <sys/select.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/time.h>

int main(int argc, char** argv)
{
        fd_set s;
        FD_ZERO(&s);
        FD_SET(0, &s);

        struct timeval t;
        t.tv_sec = 0;
        t.tv_usec = 0;
        int ret = select(1, &s, NULL, NULL, &t);

        printf("Return value: %d\n", ret);

        return 0;
}
