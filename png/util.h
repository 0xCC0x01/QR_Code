#ifndef _UTIL_H_
#define _UTIL_H_

unsigned long CRC32(void *buf, long len);

unsigned long Adler32(unsigned char *buf, int len);

#endif /* _UTIL_H_ */