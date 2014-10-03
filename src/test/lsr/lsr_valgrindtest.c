/*
 * lsr_valgrindtest
 *
 * to run with valgrind:
 *   valgrind  --db-attach=yes --leak-check=full ./lsr_valgrindtest
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lsr/lsr_pool.h>
#include <lsr/lsr_xpool.h>

/*
 * global pool test
 */

#define SIZE_1      (350*16+8)
#define SIZE_2      (370*16+8)      /* larger than SIZE_1, but in same bucket */
#define SIZE_BIG    (64*1024+8)     /* lsr_pool calls malloc directly */

struct malloclink
{
    struct malloclink *pNext;
    char *pMalloc;
};
static struct malloclink *pMalloclink = NULL;

static void gpool_test()
{
    char c;
    char *ptr;

    ptr = (char *)lsr_palloc( 100 );
    ptr[0] = 'A';
    lsr_pfree( ptr );
    ptr = (char *)lsr_palloc( SIZE_1 );
    ptr[SIZE_1-1] = 'A';
    c = ptr[SIZE_1];        /* fails outside range */
    c = ptr[SIZE_2];        /* fails outside range */
    lsr_pfree( ptr );
    c = ptr[SIZE_1-1];      /* freed space */
    ptr = (char *)lsr_palloc( SIZE_2 );
    ptr[SIZE_1-1] = 'A';
    ptr[SIZE_1] = 'A';      /* same buffer, now inside range */
    c = ptr[SIZE_1];
    ptr[SIZE_1] = c;
    c = ptr[SIZE_2];        /* fails outside range */
    lsr_pfree( ptr );

    ptr = (char *)malloc( SIZE_BIG );
    ptr[SIZE_2] = 'A';
    c = ptr[SIZE_BIG];      /* fails outside range */
    /* definitely lost, attached to stack pointer */
#ifdef notdef
    free( ptr );
#endif

    struct malloclink *p;
    p = (struct malloclink *)malloc( sizeof(*p) );
    p->pNext = pMalloclink;
    pMalloclink = p;

    ptr = (char *)malloc( SIZE_BIG );
    p->pMalloc = ptr;
    /* still reachable, attached to static, global, or heap pointer */
#ifdef notdef
    free( ptr );
#endif

    p = (struct malloclink *)malloc( sizeof(*p) );
    p->pNext = pMalloclink;
    pMalloclink = p;

    ptr = (char *)lsr_palloc( SIZE_BIG );      /* calls malloc directly */
    p->pMalloc = ptr;
    p->pMalloc[SIZE_2] = 'A';
    c = p->pMalloc[SIZE_BIG];   /* fails outside range */
    /* possibly lost, attached to returned stack pointer from lsr_palloc */
#ifdef notdef 
    lsr_pfree( p->pMalloc );
    c = p->pMalloc[0];          /* freed space */
#endif

    return;
}

/*
 * session pool (xpool) test
 */

#define LSR_XPOOL_SB_SIZE   4096    /* size of superblock */
#define XSIZE_1      (253)          /* example small block */
#define XSIZE_2      (1032)         /* example large block */
#define XSIZE_BIG    (LSR_XPOOL_SB_SIZE)

static lsr_xpool_t *pool;

static void xpool_test()
{
    char c;
    char *ptr;

    pool = lsr_xpool_new();

    ptr = (char *)lsr_xpool_alloc( pool, 100 );     /* from superblock */
    ptr[0] = 'A';
    c = ptr[0];
    ptr[0] = c;
    c = ptr[100];                           /* fails outside range */
    lsr_xpool_free( pool, ptr );
    c = ptr[0];                             /* freed space */

    ptr = (char *)lsr_xpool_alloc( pool, 100 );     /* from freelist */
    ptr[0] = 'A';
    c = ptr[100];                           /* fails outside range */
    ptr = (char *)lsr_xpool_alloc( pool, 100 );
    c = ptr[104];                           /* fails outside range */
    lsr_xpool_free( pool, ptr );

    ptr = (char *)lsr_xpool_alloc( pool, XSIZE_1 ); /* new small block */
    ptr[0] = 'A';
    ptr[XSIZE_1-1] = 'A';
    c = ptr[XSIZE_1];                       /* fails outside range */
    lsr_xpool_free( pool, ptr );
    c = ptr[XSIZE_1-1];                     /* freed space */

    ptr = (char *)lsr_xpool_alloc( pool, XSIZE_2 ); /* new large block */
    ptr[0] = 'A';
    ptr[XSIZE_2-1] = 'A';
    c = ptr[XSIZE_2];                       /* fails outside range */
    lsr_xpool_free( pool, ptr );
    c = ptr[0];                             /* freed space */

    lsr_xpool_alloc( pool, XSIZE_1 );               /* new small block */
    ptr = (char *)lsr_xpool_alloc( pool, XSIZE_1 ); /* small block from large list */
    ptr[0] = 'A';
    ptr[XSIZE_1-1] = 'A';
    c = ptr[XSIZE_1];                       /* fails outside range */
    lsr_xpool_free( pool, ptr );
    c = ptr[XSIZE_1-1];                     /* freed space */

    ptr = (char *)lsr_xpool_alloc( pool, XSIZE_BIG );   /* new `big' block */
    ptr[0] = 'A';
    ptr[XSIZE_BIG-1] = 'A';
    c = ptr[XSIZE_BIG];                     /* fails outside range */
    lsr_xpool_free( pool, ptr );
    c = ptr[0];                             /* freed space */

    ptr = (char *)lsr_xpool_alloc( pool, XSIZE_2 ); /* large block to small block */
    ptr[XSIZE_2-1] = 'A';
    c = ptr[XSIZE_2-1];
    ptr = (char *)lsr_xpool_realloc( pool, ptr, XSIZE_1 );
    c = ptr[XSIZE_2-1];                     /* fails outside range */
    lsr_xpool_free( pool, ptr );

    ptr = (char *)lsr_xpool_alloc( pool, XSIZE_1 ); /* small block to large block */
    ptr[XSIZE_1-1] = 'A';
    c = ptr[XSIZE_1-1];
    ptr = (char *)lsr_xpool_realloc( pool, ptr, XSIZE_2 );
    c = ptr[XSIZE_1-1];
    lsr_xpool_free( pool, ptr );

    lsr_xpool_delete( pool );

    return;
}

/*
 */
int main( int ac, char *av[] )
{
    printf( "hello world!\n" );

    gpool_test();
    xpool_test();

    printf( "end!\n" );
    
    return 0;
}


