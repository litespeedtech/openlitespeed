

#ifndef LS_XPOOL_INT_H
#define LS_XPOOL_INT_H

#include <lsr/ls_pooldef.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct ls_xpool_sb_s        ls_xpool_sb_t;
typedef struct xpool_alink_s        xpool_alink_t;
typedef struct xpool_qlist_s        xpool_qlist_t;

/**
 * @struct ls_qlist_s
 * @brief Session memory pool list management.
 */
struct xpool_qlist_s
{
    xpool_alink_t      *ptr;
#ifdef USE_THRSAFE_POOL
    ls_lfstack_t        stack;
    ls_spinlock_t       lock;
#endif
};

/**
 * @struct ls_xpool_s
 * @brief Session memory pool top level management.
 */
struct ls_xpool_s
{
#ifdef USE_THRSAFE_POOL
    volatile
#endif
    ls_pool_blk_t      *psuperblk;
    xpool_qlist_t       smblk;
    xpool_qlist_t       lgblk;
    ls_xpool_bblk_t    *pbigblk;
    ls_xblkctrl_t      *pfreelists;
    int                 flag;
    int                 init;
#ifdef USE_THRSAFE_POOL
    ls_spinlock_t       lock;
#endif
};



#ifdef __cplusplus
}
#endif


#endif

