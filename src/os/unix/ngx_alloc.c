
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

// 基本页的大小 ngx_pagesize = getpagesize()

ngx_uint_t  ngx_pagesize;

// 页大小的左移数
// 左移数,4k即2^12,值12

ngx_uint_t  ngx_pagesize_shift;
// 宏定义为64
// 然后由ngx_cpuinfo（ngx_cpuinfo.c）来探测

ngx_uint_t  ngx_cacheline_size;


// 封装c库函数malloc 可以记录错误日志
void *
ngx_alloc(size_t size, ngx_log_t *log)
{
    void  *p = malloc(size);
    if (p == NULL) { ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "malloc(%uz) failed", size);
    }
    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);
    return p;
}

// 封装c库函数malloc 可以记录错误日志
void *
ngx_calloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = ngx_alloc(size, log);

    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}


#if (NGX_HAVE_POSIX_MEMALIGN)
// 对齐分配内存
void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void  *p;
    int    err;

    err = posix_memalign(&p, alignment, size);
/*
int posix_memalign(void **memptr, size_t alignment, size_t size);

成功时，会返回size字节的动态内存，并且这块内存的地址是alignment的倍数，参数alignment必须是2的幂
返回的内存块的地址放在了memptr中，调用时
*/
    if (err) {
        ngx_log_error(NGX_LOG_EMERG, log, err,
                      "posix_memalign(%uz, %uz) failed", alignment, size);
        p = NULL;
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                   "posix_memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#elif (NGX_HAVE_MEMALIGN)

void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void  *p;
/*
函数：void * memalign (size_t boundary, size_t size) 
函数memalign将分配一个由size指定大小，地址是boundary的倍数的内存块。参数boundary必须是2的
幂！函数memalign可以分配较大的内存块，并且可以为返回的地址指定粒度。

函数：void * valloc (size_t size) 
使用函数valloc与使用函数memalign类似，函数valloc的内部实现里，使用页的大小作为对齐长度,使用memalign来分配内存。
它的实现如下所示： 
void * valloc (size_t size)
{
    return memalign (getpagesize (), size);
}
int getpagesize(void);

*/
    p = memalign(alignment, size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "memalign(%uz, %uz) failed", alignment, size);
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0, "memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#endif
