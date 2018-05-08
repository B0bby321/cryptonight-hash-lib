#ifndef CRYPTONITE_HASH_CAPI_H
#define CRYPTONITE_HASH_CAPI_H

void cryptonight_hash(void* output, const void* input,unsigned int length,const int aes_ni_supported);
bool has_aes_ni(void);
void bestcpu_feature(char *outbuf, int maxsz);
float cpu_temp(int core);




#endif
