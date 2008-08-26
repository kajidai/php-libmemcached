#ifndef PHP_LIBMEMCACHED_H
#define PHP_LIBMEMCACHED_H

extern zend_module_entry libmemcached_module_entry;
#define phpext_libmemcached_ptr &libmemcached_module_entry

#ifdef PHP_WIN32
#define PHP_LIBMEMCACHED_API __declspec(dllexport)
#else
#define PHP_LIBMEMCACHED_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include "libmemcached/memcached.h"

#define LIBMEMCACHED_GET_THIS(ce)         (getThis() ? (Z_OBJCE_P(getThis()) == ce ? getThis() : NULL) : NULL)
PHP_MINIT_FUNCTION(libmemcached);
PHP_MSHUTDOWN_FUNCTION(libmemcached);
PHP_RINIT_FUNCTION(libmemcached);
PHP_RSHUTDOWN_FUNCTION(libmemcached);
PHP_MINFO_FUNCTION(libmemcached);

PHP_FUNCTION(memcached_add);
PHP_FUNCTION(memcached_add_by_key);
PHP_FUNCTION(memcached_append);
PHP_FUNCTION(memcached_append_by_key);
PHP_FUNCTION(memcached_auto);
PHP_FUNCTION(memcached_behavior_get);
PHP_FUNCTION(memcached_behavior_set);
PHP_FUNCTION(memcached_callback);
PHP_FUNCTION(memcached_callback_get);
PHP_FUNCTION(memcached_callback_set);
PHP_FUNCTION(memcached_cas);
PHP_FUNCTION(memcached_cas_by_key);
PHP_FUNCTION(memcached_clone);
PHP_FUNCTION(memcached_create);
PHP_FUNCTION(memcached_decrement);
PHP_FUNCTION(memcached_delete);
PHP_FUNCTION(memcached_delete_by_key);
PHP_FUNCTION(memcached_fetch);
PHP_FUNCTION(memcached_fetch_execute);
PHP_FUNCTION(memcached_fetch_result);
PHP_FUNCTION(memcached_flush);
PHP_FUNCTION(memcached_free);
PHP_FUNCTION(memcached_get);
PHP_FUNCTION(memcached_get_by_key);
PHP_FUNCTION(memcached_increment);
PHP_FUNCTION(memcached_lib_version);
PHP_FUNCTION(memcached_mget);
PHP_FUNCTION(memcached_mget_by_key);
PHP_FUNCTION(memcached_prepend);
PHP_FUNCTION(memcached_prepend_by_key);
PHP_FUNCTION(memcached_quit);
PHP_FUNCTION(memcached_replace);
PHP_FUNCTION(memcached_replace_by_key);
PHP_FUNCTION(memcached_server_add);
PHP_FUNCTION(memcached_server_count);
PHP_FUNCTION(memcached_server_list);
PHP_FUNCTION(memcached_server_list_append);
PHP_FUNCTION(memcached_server_list_count);
PHP_FUNCTION(memcached_server_list_free);
PHP_FUNCTION(memcached_server_push);
PHP_FUNCTION(memcached_server_st);
PHP_FUNCTION(memcached_servers);
PHP_FUNCTION(memcached_servers_parse);
PHP_FUNCTION(memcached_set);
PHP_FUNCTION(memcached_set_by_key);
PHP_FUNCTION(memcached_stat);
PHP_FUNCTION(memcached_stat_get_keys);
PHP_FUNCTION(memcached_stat_get_value);
PHP_FUNCTION(memcached_stat_servername);
PHP_FUNCTION(memcached_stats);
PHP_FUNCTION(memcached_strerror);
PHP_FUNCTION(memcached_verbosity);
PHP_FUNCTION(memcached_version);
PHP_FUNCTION(memcached_ctor);

ZEND_BEGIN_MODULE_GLOBALS(libmemcached)
    memcached_st *active_object;
ZEND_END_MODULE_GLOBALS(libmemcached)

#ifdef ZTS
#define LIBMEMCACHED_G(v) TSRMG(libmemcached_globals_id, zend_libmemcached_globals *, v)
#else
#define LIBMEMCACHED_G(v) (libmemcached_globals.v)
#endif

#define memcached_objprop_get(zv, key, element, on_error) { \
    if (zend_hash_find(Z_OBJPROP(zv), key, strlen(key)+1, (void**)&element) != SUCCESS) { \
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "property [%s] is not set", key); \
        element = NULL; \
        on_error; \
    } \
}
#define memcached_objprop_get_p(zv_p, key, element, on_error)  memcached_objprop_get(*zv_p, key, element, on_error)

#endif  /* PHP_LIBMEMCACHED_H */
