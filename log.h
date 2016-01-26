#ifndef LOG_H
#define LOG_H

#include <cstdio>

#define LOG_I(fmt, args...) fprintf(stdout, fmt "\n", ##args);

//#define DEBUG
#ifdef DEBUG
    #define LOG_D(fmt, args...) fprintf(stdout, "%s:%d: " fmt "\n", \
    __FILE__, __LINE__, ##args);
    #define LOG_E(fmt, args...) fprintf(stderr, "%s:%d: " fmt "\n", \
    __FILE__, __LINE__, ##args);
#else
    #define LOG_D(fmt, args...) do{} while(0);
    #define LOG_E(fmt, args...) fprintf(stderr, fmt "\n", ##args);
#endif


#endif // LOG_H

