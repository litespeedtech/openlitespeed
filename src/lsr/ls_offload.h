

#ifndef LS_OFFLOAD_H
#define LS_OFFLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

struct ls_offload;

typedef void (*offload_done_cb)(void *param);

struct ls_offload_api
{
    int (*perform)(struct ls_offload *task);
    void (*release)(struct ls_offload *task);
    offload_done_cb on_task_done;
};

enum LS_OFFLOAD_STATE
{
    LS_OFFLOAD_NOT_INUSE,
    LS_OFFLOAD_ENQUEUE,
    LS_OFFLOAD_PROCESSING,
    LS_OFFLOAD_IN_FINISH_QUEUE,
    LS_OFFLOAD_FINISH_CB,
    LS_OFFLOAD_FINISH_CB_DONE,
    LS_OFFLOAD_FINISH_CB_BYPASS,
    LS_OFFLOAD_BYPASS,
    LS_OFFLOAD_ENQUEUE_FAIL
};

typedef struct ls_offload
{
    void           *task_link;
    struct ls_offload_api *api;
    void           *param_task_done;
    int             ref_cnt;
    char            state;
    char            is_canceled;
} ls_offload_t;

struct Offloader;
struct Offloader *offloader_new(const char *log_id, int workers);
int offloader_enqueue(struct Offloader *, struct ls_offload *task);

#ifdef __cplusplus
}
#endif


#endif //LS_OFFLOAD_H