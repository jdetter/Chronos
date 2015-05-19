#ifndef _LI_PROXY_H_
#define _LI_PROXY_H_

void* li_mmap(void* a, uint l, int p, int fl, int fd, uint o);
void li_printf(const char *format , ... );
int li_strlen(char* string);
void li_strncpy(char* dst, char* src, unsigned int sz);
void li_strncat(char* dst, char* src, unsigned int sz);
int li_strcmp(char* str1, char* str2);
int li_strtol(char* str, char** dptr, int len);

#endif
