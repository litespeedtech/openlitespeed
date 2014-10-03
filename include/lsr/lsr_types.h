

#ifndef LSR_TYPES_H
#define LSR_TYPES_H

/**
 * @file
 */

#ifdef __cplusplus
extern "C" {
#endif 

#define lsr_inline      static inline

typedef unsigned long           lsr_hash_key_t;
typedef struct lsr_ptrlist_s    lsr_ptrlist_t;
typedef lsr_ptrlist_t           lsr_strlist_t;
/**
 * @addtogroup LSR_STR_GROUP
 * @{
 */
typedef struct lsr_str_s        lsr_str_t;
typedef struct lsr_str_pair_s   lsr_str_pair_t;
/**
 * @}
 */
typedef struct lsr_xpool_s      lsr_xpool_t;


#ifdef __cplusplus
}
#endif 

#endif //LSR_TYPES_H
