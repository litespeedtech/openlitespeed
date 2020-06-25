
#include <socket/reuseport.h>
#include <socket/ls_sock.h>
#include <log4cxx/logger.h>

#include <unistd.h>


int ReusePortFds::passFds(const char *type, const char *addr, int target_fd)
{
    int i;
    int count = 0;
    for(i = size() - 1; i >= 0; --i)
    {
        int fd = (*this)[i];
        if (fd != -1)
        {
            --target_fd;
            LS_INFO("[%s %s], SO_REUSEPORT #%d, pass listener copy fd %d to %d.",
                    type, addr, count, fd, target_fd);
            ++count;
            dup2(fd, target_fd);
            ls_close(fd);
        }
    }
    return count;
}


int ReusePortFds::getFdCount(int* maxfd)
{
    int count = 0;
    int i;
    for(i = 0; i < size(); ++i)
    {
        if ((*this)[i] != -1)
        {
            ++count;
            if ((*this)[i] > *maxfd)
                *maxfd = (*this)[i];
        }
    }
    return count;
}


void ReusePortFds::close()
{
    int i;
    for(i = 0; i < size(); ++i)
    {
        if ((*this)[i] != -1)
        {
            LS_INFO("Close SO_REUSEPORT %d.", (*this)[i]);
            ls_close((*this)[i]);
            (*this)[i] = -1;
        }
    }
}


int ReusePortFds::getActiveFd(int seq, int *n)
{
    if (size() <= 0)
        return -1;
    *n = (seq - 1) % size();
    int fd = (*this)[*n];
    (*this)[*n] = -1;
    close();
    return fd;
}


