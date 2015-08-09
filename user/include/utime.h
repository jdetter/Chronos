#ifndef _SYS_UTIME_H
#define _SYS_UTIME_H

#ifdef __cplusplus
extern "C" {
#endif

struct utimbuf 
{
  time_t actime;
  time_t modtime; 
};

/**
 * Change the modification and access time on a file to the given times
 * struct.
 */
int utime(const char* filename, const struct utimbuf* times);

/**
 * Just like utime except times is a pointer to an array of timevals.
 */
int utimes(const char* filename, const struct timeval times[2]);

#ifdef __cplusplus
};
#endif

#endif /* _SYS_UTIME_H */
