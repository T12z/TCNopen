
#ifndef TEST_UTILS_H
#define TEST_UTILS_H

void invalidate_crc(unsigned char* msg, unsigned short len);
int latency_ssc_generator(int start, int cycle, float ratio);

#endif