
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Maxim Dounin
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

//主要完成模块的管理工作
#define NGX_MAX_DYNAMIC_MODULES  128


static ngx_uint_t ngx_module_index(ngx_cycle_t *cycle);
static ngx_uint_t ngx_module_ctx_index(ngx_cycle_t *cycle, ngx_uint_t type,
    ngx_uint_t index);


ngx_uint_t         ngx_max_module;
static ngx_uint_t  ngx_modules_n;

// 开始计算所有的静态模块数量
// ngx_modules是nginx模块数组，存储所有的模块指针，由make生成在objs/ngx_modules.c
// 这里赋值每个模块的index成员
// ngx_modules_n保存了最后一个可用的序号
// ngx_max_module是模块数量的上限
*/
// typedef struct ngx_module_s          ngx_module_t;
#if 1  
ngx_modules[0]
    struct ngx_module_s {
        ngx_uint_t            ctx_index;
        ngx_uint_t            index;  // 下标 序号
    
        char                 *name;    // 设置名字
        ...........................;
   }
char *ngx_module_names[] = {
    "ngx_core_module",
模块数组 + 模块名字 数组  一一对应的
typedef struct ngx_module_s          ngx_module_t;
    struct ngx_module_s {
        ngx_uint_t            ctx_index;
        ngx_uint_t            index;  // 所以在此处给其index  和name初始化
        char                 *name;
        ngx_uint_t            spare0;
        ngx_uint_t            spare1;
        ngx_uint_t            version;
        const char           *signature;    // 前7个参数使用宏定义进行替换初始化
        void                 *ctx;               /* module context */ 
        ngx_command_t        *commands;
        ngx_uint_t            type;
        ngx_int_t           (*init_master)(ngx_log_t *log);
        ngx_int_t           (*init_module)(ngx_cycle_t *cycle);
        ngx_int_t           (*init_process)(ngx_cycle_t *cycle);
        ngx_int_t           (*init_thread)(ngx_cycle_t *cycle);
        void                (*exit_thread)(ngx_cycle_t *cycle);
        void                (*exit_process)(ngx_cycle_t *cycle);
        void                (*exit_master)(ngx_cycle_t *cycle);
        uintptr_t             spare_hook0;
        uintptr_t             spare_hook1;
        uintptr_t             spare_hook2;
        uintptr_t             spare_hook3;
        uintptr_t             spare_hook4;
        uintptr_t             spare_hook5;
        uintptr_t             spare_hook6;
        uintptr_t             spare_hook7;
    };
    
#define NGX_MODULE_V1                                                         \
        NGX_MODULE_UNSET_INDEX, NGX_MODULE_UNSET_INDEX,                           \
        NULL, 0, 0, nginx_version, NGX_MODULE_SIGNATURE
    
#define NGX_MODULE_V1_PADDING  0, 0, 0, 0, 0, 0, 0, 0

// 这里是程序员自己编写，最后makefile生成obj文件中的，对应下面生成的模块名字和模块表
ngx_module_t *ngx_modules[] = {
    &ngx_core_module,
    &ngx_errlog_module,
    &ngx_conf_module,

ngx_module_t  ngx_core_module = {// 该模块生成对应的模块名字ngx_core_module 
// 给其第三个成员变量name赋值
    NGX_MODULE_V1,
    &ngx_core_module_ctx,  /* module context */  模块内容自己填充
    ngx_core_commands,     /* module directives */ 
    NGX_CORE_MODULE,       /* module type */ 
    NULL,                  /* init master */ 
    NULL,                  /* init module */ 
    NULL,                  /* init process */ 
    NULL,                  /* init thread */ 
    NULL,                  /* exit thread */ 
    NULL,                  /* exit process */ 
    NULL,                  /* exit master */ 
    NGX_MODULE_V1_PADDING
};
#endif

ngx_int_t
ngx_preinit_modules(void)
{
    ngx_uint_t  i;
    // 从0开始，为所有静态模块设置序号和名字

    for (i = 0; ngx_modules[i]; i++) {
        ngx_modules[i]->index = i; // 结构体指针数组，指向每个模块的成员变量，给其编号及赋值
        ngx_modules[i]->name = ngx_module_names[i];
    }
    // ngx_modules_n保存了最后一个可用的序号
    // ngx_max_module是模块数量的上限
    ngx_modules_n = i;
    ngx_max_module = ngx_modules_n + NGX_MAX_DYNAMIC_MODULES;

    return NGX_OK;
}


ngx_int_t
ngx_cycle_modules(ngx_cycle_t *cycle)
{
    /*
     * create a list of modules to be used for this cycle,
     * copy static modules to it
     */

    cycle->modules = ngx_pcalloc(cycle->pool, (ngx_max_module + 1) * sizeof(ngx_module_t *));
    if (cycle->modules == NULL) {
        return NGX_ERROR;
    }
	// 将模块数组拷贝到全局变量modules中去
    ngx_memcpy(cycle->modules, ngx_modules, ngx_modules_n * sizeof(ngx_module_t *));

    cycle->modules_n = ngx_modules_n;

    return NGX_OK;
}


ngx_int_t
ngx_init_modules(ngx_cycle_t *cycle)
{
    ngx_uint_t  i;

    for (i = 0; cycle->modules[i]; i++) {
        if (cycle->modules[i]->init_module) {
            if (cycle->modules[i]->init_module(cycle) != NGX_OK) {
                return NGX_ERROR;
            }
        }
    }

    return NGX_OK;
}


ngx_int_t
ngx_count_modules(ngx_cycle_t *cycle, ngx_uint_t type)
{
    ngx_uint_t     i, next, max;
    ngx_module_t  *module;

    next = 0;
    max = 0;

    /* count appropriate modules, set up their indices */

    for (i = 0; cycle->modules[i]; i++) {
        module = cycle->modules[i];

        if (module->type != type) {
            continue;
        }

        if (module->ctx_index != NGX_MODULE_UNSET_INDEX) {

            /* if ctx_index was assigned, preserve it */

            if (module->ctx_index > max) {
                max = module->ctx_index;
            }

            if (module->ctx_index == next) {
                next++;
            }

            continue;
        }

        /* search for some free index */

        module->ctx_index = ngx_module_ctx_index(cycle, type, next);

        if (module->ctx_index > max) {
            max = module->ctx_index;
        }

        next = module->ctx_index + 1;
    }

    /*
     * make sure the number returned is big enough for previous
     * cycle as well, else there will be problems if the number
     * will be stored in a global variable (as it's used to be)
     * and we'll have to roll back to the previous cycle
     */

    if (cycle->old_cycle && cycle->old_cycle->modules) {

        for (i = 0; cycle->old_cycle->modules[i]; i++) {
            module = cycle->old_cycle->modules[i];

            if (module->type != type) {
                continue;
            }

            if (module->ctx_index > max) {
                max = module->ctx_index;
            }
        }
    }

    /* prevent loading of additional modules */

    cycle->modules_used = 1;

    return max + 1;
}


ngx_int_t
ngx_add_module(ngx_conf_t *cf, ngx_str_t *file, ngx_module_t *module,
    char **order)
{
    void               *rv;
    ngx_uint_t          i, m, before;
    ngx_core_module_t  *core_module;

    if (cf->cycle->modules_n >= ngx_max_module) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "too many modules loaded");
        return NGX_ERROR;
    }

    if (module->version != nginx_version) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "module \"%V\" version %ui instead of %ui",
                           file, module->version, (ngx_uint_t) nginx_version);
        return NGX_ERROR;
    }

    if (ngx_strcmp(module->signature, NGX_MODULE_SIGNATURE) != 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "module \"%V\" is not binary compatible",
                           file);
        return NGX_ERROR;
    }

    for (m = 0; cf->cycle->modules[m]; m++) {
        if (ngx_strcmp(cf->cycle->modules[m]->name, module->name) == 0) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "module \"%s\" is already loaded",
                               module->name);
            return NGX_ERROR;
        }
    }

    /*
     * if the module wasn't previously loaded, assign an index
     */

    if (module->index == NGX_MODULE_UNSET_INDEX) {
        module->index = ngx_module_index(cf->cycle);

        if (module->index >= ngx_max_module) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "too many modules loaded");
            return NGX_ERROR;
        }
    }

    /*
     * put the module into the cycle->modules array
     */

    before = cf->cycle->modules_n;

    if (order) {
        for (i = 0; order[i]; i++) {
            if (ngx_strcmp(order[i], module->name) == 0) {
                i++;
                break;
            }
        }

        for ( /* void */ ; order[i]; i++) {

#if 0
            ngx_log_debug2(NGX_LOG_DEBUG_CORE, cf->log, 0,
                           "module: %s before %s",
                           module->name, order[i]);
#endif

            for (m = 0; m < before; m++) {
                if (ngx_strcmp(cf->cycle->modules[m]->name, order[i]) == 0) {

                    ngx_log_debug3(NGX_LOG_DEBUG_CORE, cf->log, 0,
                                   "module: %s before %s:%i",
                                   module->name, order[i], m);

                    before = m;
                    break;
                }
            }
        }
    }

    /* put the module before modules[before] */

    if (before != cf->cycle->modules_n) {
        ngx_memmove(&cf->cycle->modules[before + 1],
                    &cf->cycle->modules[before],
                    (cf->cycle->modules_n - before) * sizeof(ngx_module_t *));
    }

    cf->cycle->modules[before] = module;
    cf->cycle->modules_n++;

    if (module->type == NGX_CORE_MODULE) {

        /*
         * we are smart enough to initialize core modules;
         * other modules are expected to be loaded before
         * initialization - e.g., http modules must be loaded
         * before http{} block
         */

        core_module = module->ctx;

        if (core_module->create_conf) {
            rv = core_module->create_conf(cf->cycle);
            if (rv == NULL) {
                return NGX_ERROR;
            }

            cf->cycle->conf_ctx[module->index] = rv;
        }
    }

    return NGX_OK;
}


static ngx_uint_t
ngx_module_index(ngx_cycle_t *cycle)
{
    ngx_uint_t     i, index;
    ngx_module_t  *module;

    index = 0;

again:

    /* find an unused index */

    for (i = 0; cycle->modules[i]; i++) {
        module = cycle->modules[i];

        if (module->index == index) {
            index++;
            goto again;
        }
    }

    /* check previous cycle */

    if (cycle->old_cycle && cycle->old_cycle->modules) {

        for (i = 0; cycle->old_cycle->modules[i]; i++) {
            module = cycle->old_cycle->modules[i];

            if (module->index == index) {
                index++;
                goto again;
            }
        }
    }

    return index;
}


static ngx_uint_t
ngx_module_ctx_index(ngx_cycle_t *cycle, ngx_uint_t type, ngx_uint_t index)
{
    ngx_uint_t     i;
    ngx_module_t  *module;

again:

    /* find an unused ctx_index */

    for (i = 0; cycle->modules[i]; i++) {
        module = cycle->modules[i];

        if (module->type != type) {
            continue;
        }

        if (module->ctx_index == index) {
            index++;
            goto again;
        }
    }

    /* check previous cycle */

    if (cycle->old_cycle && cycle->old_cycle->modules) {

        for (i = 0; cycle->old_cycle->modules[i]; i++) {
            module = cycle->old_cycle->modules[i];

            if (module->type != type) {
                continue;
            }

            if (module->ctx_index == index) {
                index++;
                goto again;
            }
        }
    }

    return index;
}
