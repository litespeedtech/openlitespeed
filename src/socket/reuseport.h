
#ifndef __REUSEPORT_H__
#define __REUSEPORT_H__

#include <util/objarray.h>

class ReusePortFds : public TObjArray<int>
{
public:
    ReusePortFds()
    {}
    ~ReusePortFds()
    {   close();    }
    int passFds(const char *type, const char *addr, int target_fd);
    int getFdCount(int* maxfd);

    int getActiveFd(int seq, int *n);

    int shrink(int size);

    void close();
};

#endif //__REUSEPORT_H__

