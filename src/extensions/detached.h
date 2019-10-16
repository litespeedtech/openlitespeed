
#ifndef DETACHED_H
#define DETACHED_H



struct DetachedPidInfo
{
    ino_t inode;
    long  last_modify;
    pid_t pid;
    unsigned start_time_diff;
    uint32_t bin_last_mod;
    uint32_t reserved;

    DetachedPidInfo()
        : inode(0)
        , last_modify(0)
        , pid(-1)
        , start_time_diff(0)
        , bin_last_mod(0)
        , reserved(0)
        {}
};
typedef struct DetachedPidInfo DetachedPidInfo_t;


struct DetachedProcess
{
    DetachedPidInfo_t  pid_info;
    uint32_t           last_check_time;
    uint32_t           last_stop_time;
    int                fd_pid_file;

    DetachedProcess()
        : last_check_time(0)
        , last_stop_time(0)
        , fd_pid_file(-1)
        {}
};
typedef struct DetachedProcess DetachedProcess_t;



#endif //DETACHED_H



