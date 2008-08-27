#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_libmemcached.h"

#include "libmemcached/memcached.h"

#include "ext/standard/php_string.h"
#include "ext/standard/php_var.h"
#include "ext/standard/php_smart_str.h"
/* If you declare any globals in php_libmemcached.h uncomment this:
 */
ZEND_DECLARE_MODULE_GLOBALS(libmemcached)

/* True global resources - no need for thread safety here */
static int le_memc;
static int le_servers;
static zend_class_entry *memcached_entry_ptr = NULL;

static void _libmemcached_connection_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC);
static void _memcached_create();

/* {{{ libmemcached_functions[]
 *
 * Every user visible function must have an entry in libmemcached_functions[].
 */
zend_function_entry libmemcached_functions[] = {
    PHP_FE(memcached_behavior_get, NULL)
    PHP_FE(memcached_behavior_set, NULL)
    PHP_FE(memcached_get, NULL)
    PHP_FE(memcached_get_by_key, NULL)
    PHP_FE(memcached_server_add, NULL)
    PHP_FE(memcached_set, NULL)
    PHP_FE(memcached_set_by_key, NULL)
    PHP_FE(memcached_ctor, NULL)
    PHP_FE(memcached_increment, NULL)
    PHP_FE(memcached_decrement, NULL)
    {NULL, NULL, NULL}  /* Must be the last line in libmemcached_functions[] */
};
zend_function_entry memcached_functions[] = {
    PHP_FALIAS(memcached, memcached_ctor, NULL)
    PHP_FALIAS(addserver, memcached_server_add, NULL)
    PHP_FALIAS(set, memcached_set, NULL)
    PHP_FALIAS(set_by_key, memcached_set_by_key, NULL)
    PHP_FALIAS(get, memcached_get, NULL)
    PHP_FALIAS(get_by_key, memcached_get_by_key, NULL)
    PHP_FALIAS(behavior_get, memcached_behavior_get, NULL)
    PHP_FALIAS(behavior_set, memcached_behavior_set, NULL)
    PHP_FALIAS(increment, memcached_increment, NULL)
    PHP_FALIAS(decrement, memcached_decrement, NULL)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ libmemcached_module_entry
 */
zend_module_entry libmemcached_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "libmemcached",
    libmemcached_functions,
    PHP_MINIT(libmemcached),
    PHP_MSHUTDOWN(libmemcached),
    PHP_RINIT(libmemcached),        /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(libmemcached),    /* Replace with NULL if there's nothing to do at request end */
    PHP_MINFO(libmemcached),
#if ZEND_MODULE_API_NO >= 20010901
    "0.1", /* Replace with version number for your extension */
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_LIBMEMCACHED
ZEND_GET_MODULE(libmemcached)
#endif

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(libmemcached)
{
    // le_memc = zend_register_list_destructors_ex(_libmemcached_connection_resource_dtor, NULL, "memcached_st", 0);
    le_memc = zend_register_list_destructors_ex(NULL, NULL, "memcached_st", 0);
    le_servers = zend_register_list_destructors_ex(NULL, NULL, "memcached server", module_number);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_NO_BLOCK", MEMCACHED_BEHAVIOR_NO_BLOCK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_TCP_NODELAY", MEMCACHED_BEHAVIOR_TCP_NODELAY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_HASH", MEMCACHED_BEHAVIOR_HASH, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_KETAMA", MEMCACHED_BEHAVIOR_KETAMA, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE", MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE", MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_CACHE_LOOKUPS", MEMCACHED_BEHAVIOR_CACHE_LOOKUPS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_SUPPORT_CAS", MEMCACHED_BEHAVIOR_SUPPORT_CAS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_POLL_TIMEOUT", MEMCACHED_BEHAVIOR_POLL_TIMEOUT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_DISTRIBUTION", MEMCACHED_BEHAVIOR_DISTRIBUTION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_BUFFER_REQUESTS", MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_USER_DATA", MEMCACHED_BEHAVIOR_USER_DATA, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_SORT_HOSTS", MEMCACHED_BEHAVIOR_SORT_HOSTS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_VERIFY_KEY", MEMCACHED_BEHAVIOR_VERIFY_KEY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT", MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_RETRY_TIMEOUT", MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_DISTRIBUTION_MODULA", MEMCACHED_DISTRIBUTION_MODULA, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_DISTRIBUTION_CONSISTENT", MEMCACHED_DISTRIBUTION_CONSISTENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_DISTRIBUTION_CONSISTENT_WHEEL", MEMCACHED_DISTRIBUTION_CONSISTENT_WHEEL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA", MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA, CONST_CS | CONST_PERSISTENT);

    zend_class_entry memcached_entry;
    INIT_CLASS_ENTRY(memcached_entry, "memcached", memcached_functions);
    memcached_entry_ptr = zend_register_internal_class(&memcached_entry TSRMLS_CC);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(libmemcached)
{
    /* uncomment this line if you have INI entries
       UNREGISTER_INI_ENTRIES();
    */
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(libmemcached)
{
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(libmemcached)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(libmemcached)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "libmemcached support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini
       DISPLAY_INI_ENTRIES();
    */
}
/* }}} */

// {{{ _libmemcached_get_memcached_connection()
static void *_libmemcached_get_memcached_connection(zval *obj) {
    zval **tmp;
    memcached_objprop_get_p(obj, "memc", tmp, 0);
    if (tmp == NULL) {
        return NULL;
    }
    int type;
    memcached_st *res_memc = NULL;
    res_memc = zend_list_find(Z_LVAL_PP(tmp), &type);
    return res_memc;
}
// }}}
// {{{ _libmemcached_connection_resource_dtor()
static void _libmemcached_connection_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
    memcached_free((memcached_st *)rsrc->ptr TSRMLS_CC);
}
// }}}
// {{{ _memcached_create()
static void _memcached_create(zval *obj)
{
    memcached_st *memc;
    int memc_id = -1;

    memcached_st *res_memc = NULL;
    memc = memcached_create(res_memc);
    if (memc == NULL) {
    }

    int ret = zend_list_insert(memc, le_memc);
    add_property_resource(obj, "memc", ret);
}
// }}}

// {{{ PHP_FUNCTION(memcached_ctor)
PHP_FUNCTION(memcached_ctor){

    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }
    _memcached_create(obj);
}
// }}}
// {{{ PHP_FUNCTION(memcached_get)
PHP_FUNCTION(memcached_get)
{
    zval *obj = getThis();
    memcached_return rc;

    uint32_t flags;
    const char *key = NULL;
    int key_len, value_len = 0;
    size_t val_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len) == FAILURE) {
        return;
    }

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_libmemcached_get_memcached_connection(obj);

    char *ret;
    ret = memcached_get(res_memc, key, strlen(key), &val_len, &flags, &rc);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    if (flags & MMC_SERIALIZED ) {
        const char *value_tmp = ret;
        php_unserialize_data_t var_hash;
        PHP_VAR_UNSERIALIZE_INIT(var_hash);

        if (!php_var_unserialize(&return_value, (const unsigned char **)&value_tmp, (const unsigned char *)(value_tmp + value_len), &var_hash TSRMLS_CC)) {
            ZVAL_FALSE(return_value);
            PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
            efree(ret);
            RETURN_FALSE;
        }

        PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
        efree(ret);
    } else {
        ZVAL_STRINGL(return_value, ret, strlen(ret), 0);
    }
}
// }}}
// {{{ PHP_FUNCTION(memcached_get_by_key)
PHP_FUNCTION(memcached_get_by_key)
{
    zval *obj = getThis();
    memcached_return rc;
    uint32_t flags;
    const char *key, *master_key = NULL;
    int key_len, master_key_len = 0;
    size_t val_len = 0;
    char *ret;

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_libmemcached_get_memcached_connection(obj);

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &master_key, &master_key_len, &key, &key_len) == FAILURE) {
        return;
    }   

    ret = memcached_get_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), &val_len, &flags, &rc);
    if(rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }

    RETURN_STRING(ret, 1);
}
// }}}
// {{{ PHP_FUNCTION(memcached_server_add)
PHP_FUNCTION(memcached_server_add)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    char *hostname;
    int hostname_len, port;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &hostname, &hostname_len, &port) == FAILURE) {
        return;
    }

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_libmemcached_get_memcached_connection(obj);
    if (res_memc == NULL) {
        RETURN_FALSE;
    }

    if (MEMCACHED_SUCCESS != memcached_server_add(res_memc, hostname, port)) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_set)
PHP_FUNCTION(memcached_set)
{
    const char *key, *value = NULL;
    int key_len, value_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;


    php_serialize_data_t var_hash;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|ll", &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    zval *obj = getThis();
    memcached_st *res_memc = _libmemcached_get_memcached_connection(obj);

    switch (Z_TYPE_P(var)) {
        case IS_STRING:
            value = Z_STRVAL_P(var);
            value_len = Z_STRLEN_P(var);
            break;

        case IS_LONG:
        case IS_DOUBLE:
        case IS_BOOL:
            convert_to_string(var);
            value = Z_STRVAL_P(var);
            value_len = Z_STRLEN_P(var);
            break;

        default: {
            zval var_copy, *var_copy_ptr;

            var_copy = *var;
            zval_copy_ctor(&var_copy);

            var_copy_ptr = &var_copy;

            PHP_VAR_SERIALIZE_INIT(var_hash);
            php_var_serialize(&buf, &var_copy_ptr, &var_hash TSRMLS_CC);
            PHP_VAR_SERIALIZE_DESTROY(var_hash);

            if (!buf.c) {
                zval_dtor(&var_copy);
                RETURN_FALSE;
            }

            value = buf.c;
            value_len = buf.len;
            flags |= MMC_SERIALIZED;
            zval_dtor(&var_copy);
        }   
        break;
    }

    memcached_return rc;
    rc = memcached_set(res_memc, key, strlen(key), value, strlen(value), (time_t)expiration, (uint16_t)flags);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_set_by_key)
PHP_FUNCTION(memcached_set_by_key)
{
    zval *obj = getThis();
    memcached_st *res_memc = _libmemcached_get_memcached_connection(obj);

    const char *master_key, *key, *val = NULL;
    int master_key_len, key_len, val_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss|ll", &master_key, &master_key_len, &key, &key_len, &val, &val_len, &expiration, &flags) == FAILURE) {
        return;
    }

    memcached_return rc;
    rc = memcached_set_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), val, strlen(val), (time_t)expiration, (uint16_t)flags);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_behavior_get)
PHP_FUNCTION(memcached_behavior_get)
{
    int key, val = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &key) == FAILURE) {
        return;
    }

    zval *obj = getThis();
    memcached_st *res_memc = _libmemcached_get_memcached_connection(obj);

    memcached_return rc;
    val = memcached_behavior_get(res_memc, key);
    RETURN_DOUBLE(val);
}
// }}}
// {{{ PHP_FUNCTION(memcached_behavior_set)
PHP_FUNCTION(memcached_behavior_set)
{
    int key, val = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &key, &val) == FAILURE) {
        return;
    }

    zval *obj = getThis();
    memcached_st *res_memc = _libmemcached_get_memcached_connection(obj);

    memcached_return rc;
    rc = memcached_behavior_set(res_memc, key, val);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_increment)
PHP_FUNCTION(memcached_increment)
{
    zval *obj = getThis();
    memcached_st *res_memc = _libmemcached_get_memcached_connection(obj);

    memcached_return rc;
    zval *memc = NULL;

    const char *key;
    int key_len;
    unsigned int offset;
    uint64_t value;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &key, &key_len, &offset) == FAILURE) {
        return;
    }

    rc = memcached_increment(res_memc, key, strlen(key), offset, &value);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_decrement)
PHP_FUNCTION(memcached_decrement)
{
    zval *obj = getThis();
    memcached_st *res_memc = _libmemcached_get_memcached_connection(obj);

    memcached_return rc;
    zval *memc = NULL;

    const char *key;
    int key_len;
    unsigned int offset;
    uint64_t value;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &key, &key_len, &offset) == FAILURE) {
        return;
    }

    rc = memcached_decrement(res_memc, key, strlen(key), offset, &value);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
