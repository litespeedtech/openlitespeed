

#ifndef LS_EVTCB_H
#define LS_EVTCB_H

#include <lsr/ls_types.h>
#include <lsr/ls_atomic.h>

#ifdef __cplusplus
extern "C" {
#endif


struct evtcbnode_s;
typedef struct evtcbhead_s
{
    struct evtcbnode_s *evtcb_tail;
    struct evtcbnode_s *evtcb_runqueue;
    struct evtcbhead_s **back_ref_ptr;
}
evtcbhead_t;


/**
 * @typedef evtcb_pf
 * @brief The callback function for scheduling event.
 *
 */
typedef int (*evtcb_pf)(evtcbhead_t *pSession,
                        const long lParam, void *pParam);

ls_inline void
evtcbhead_reset(evtcbhead_t *pThis)
{
    pThis->evtcb_tail = NULL;
    pThis->evtcb_runqueue = NULL;
    pThis->back_ref_ptr = NULL;
}

// Getters and setters.

ls_inline struct evtcbnode_s *
evtcbhead_get_tail(evtcbhead_t *pThis)      {   return pThis->evtcb_tail;               }


ls_inline struct evtcbnode_s *
evtcbhead_get_runqueue(evtcbhead_t *pThis)  {   return pThis->evtcb_runqueue;           }


ls_inline evtcbhead_t **
evtcbhead_get_backrefptr(evtcbhead_t *pThis){   return pThis->back_ref_ptr;             }


ls_inline struct evtcbnode_s *
evtcbhead_set_tail(evtcbhead_t *pThis, struct evtcbnode_s *pNode)
{
    struct evtcbnode_s *pOld = pThis->evtcb_tail;
    pThis->evtcb_tail = pNode;
    return pOld;
}


ls_inline struct evtcbnode_s *
evtcbhead_set_runqueue(evtcbhead_t *pThis, struct evtcbnode_s *pNode)
{
    struct evtcbnode_s *pOld = pThis->evtcb_runqueue;
    pThis->evtcb_runqueue = pNode;
    return pOld;
}


ls_inline evtcbhead_t **
evtcbhead_set_backrefptr(evtcbhead_t *pThis, evtcbhead_t **pBackRef)
{
    evtcbhead_t **pOld = pThis->back_ref_ptr;
    pThis->back_ref_ptr = pBackRef;
    return pOld;
}

// The below two functions only dereference the pointers.
// back_ref_ptr and *back_ref_ptr should be memory-managed elsewhere.

ls_inline void
evtcbhead_backref_clear_if_match(evtcbhead_t *pThis, evtcbhead_t **pBackRef)
{
    if (evtcbhead_get_backrefptr(pThis) == pBackRef)
        evtcbhead_set_backrefptr(pThis, NULL);
}


ls_inline void
evtcbhead_backref_reset(evtcbhead_t *pThis)
{
    if (*evtcbhead_get_backrefptr(pThis) == pThis)
        *pThis->back_ref_ptr = NULL;
    evtcbhead_set_backrefptr(pThis, NULL);
}

ls_inline int
evtcbhead_is_queued(evtcbhead_t *pThis)     {   return (pThis->evtcb_tail != NULL);     }


ls_inline int
evtcbhead_is_running(evtcbhead_t *pThis)    {   return (pThis->evtcb_runqueue != NULL); }


ls_inline int
evtcbhead_is_active(evtcbhead_t *pThis)
{   return (evtcbhead_is_queued(pThis) || evtcbhead_is_running(pThis));     }


// Thread-safe getters and setters.

ls_inline struct evtcbnode_s *
evtcbhead_get_tail_ts(evtcbhead_t *pThis)
{   return ls_atomic_value(&pThis->evtcb_tail);             }


ls_inline struct evtcbnode_s *
evtcbhead_get_runqueue_ts(evtcbhead_t *pThis)
{   return ls_atomic_value(&pThis->evtcb_runqueue);         }

#ifdef __cplusplus
}
#endif


#endif
