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

#include "zlib.h"

ZEND_DECLARE_MODULE_GLOBALS(libmemcached)

/* True global resources - no need for thread safety here */
static int le_memc;
static zend_class_entry *memcached_entry_ptr = NULL;

static void _php_libmemcached_connection_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC);
static void _php_libmemcached_server_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC);
static void _php_libmemcached_create(zval *obj TSRMLS_DC);
static int _php_libmemcached_get_value(zval *, char *, uint32_t TSRMLS_DC);
static char* _get_value_from_zval(smart_str *, zval *, uint32_t * TSRMLS_DC);

/* {{{ libmemcached_functions[]
 *
 * Every user visible function must have an entry in libmemcached_functions[].
 */
zend_function_entry libmemcached_functions[] = {
    PHP_FE(memcached_ctor, NULL)
    PHP_FE(memcached_server_add, NULL)
    PHP_FE(memcached_add, NULL)
    PHP_FE(memcached_add_by_key, NULL)
    PHP_FE(memcached_append, NULL)
    PHP_FE(memcached_append_by_key, NULL)
    PHP_FE(memcached_behavior_get, NULL)
    PHP_FE(memcached_behavior_set, NULL)
    PHP_FE(memcached_cas, NULL)
    PHP_FE(memcached_cas_by_key, NULL)
    PHP_FE(memcached_delete, NULL)
    PHP_FE(memcached_delete_by_key, NULL)
    PHP_FE(memcached_get, NULL)
    PHP_FE(memcached_get_by_key, NULL)
    PHP_FE(memcached_set, NULL)
    PHP_FE(memcached_set_by_key, NULL)
    PHP_FE(memcached_increment, NULL)
    PHP_FE(memcached_decrement, NULL)
    PHP_FE(memcached_prepend, NULL)
    PHP_FE(memcached_prepend_by_key, NULL)
    PHP_FE(memcached_replace, NULL)
    PHP_FE(memcached_replace_by_key, NULL)
    PHP_FE(memcached_server_list, NULL)
    PHP_FE(memcached_mget, NULL)
    PHP_FE(memcached_fetch, NULL)
    PHP_FE(memcached_server_list_append, NULL)
    PHP_FE(memcached_server_push, NULL)
    {NULL, NULL, NULL}  /* Must be the last line in libmemcached_functions[] */
};
/* }}} */
/* {{{ memcached_functions[] */
static zend_function_entry memcached_functions[] = {
    PHP_FALIAS(memcached, memcached_ctor, NULL)
    PHP_FALIAS(addserver, memcached_server_add, NULL)
    PHP_FALIAS(add, memcached_add, NULL)
    PHP_FALIAS(add_by_key, memcached_add_by_key, NULL)
    PHP_FALIAS(append, memcached_append, NULL)
    PHP_FALIAS(append_by_key, memcached_append_by_key, NULL)
    PHP_FALIAS(behavior_get, memcached_behavior_get, NULL)
    PHP_FALIAS(behavior_set, memcached_behavior_set, NULL)
    PHP_FALIAS(cas, memcached_cas, NULL)
    PHP_FALIAS(cas_by_key, memcached_cas_by_key, NULL)
    PHP_FALIAS(delete, memcached_delete, NULL)
    PHP_FALIAS(delete_by_key, memcached_delete_by_key, NULL)
    PHP_FALIAS(get, memcached_get, NULL)
    PHP_FALIAS(get_by_key, memcached_get_by_key, NULL)
    PHP_FALIAS(set, memcached_set, NULL)
    PHP_FALIAS(set_by_key, memcached_set_by_key, NULL)
    PHP_FALIAS(increment, memcached_increment, NULL)
    PHP_FALIAS(decrement, memcached_decrement, NULL)
    PHP_FALIAS(prepend, memcached_prepend, NULL)
    PHP_FALIAS(prepend_by_key, memcached_prepend_by_key, NULL)
    PHP_FALIAS(replace, memcached_replace, NULL)
    PHP_FALIAS(replace_by_key, memcached_replace_by_key, NULL)
    PHP_FALIAS(server_list, memcached_server_list, NULL)
    PHP_FALIAS(server_list_append, memcached_server_list_append, NULL)
    PHP_FALIAS(server_push, memcached_server_push, NULL)
    PHP_FALIAS(mget, memcached_mget, NULL)
    PHP_FALIAS(fetch, memcached_fetch, NULL)
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
    le_memc = zend_register_list_destructors_ex(_php_libmemcached_connection_resource_dtor, _php_libmemcached_server_resource_dtor, "memcached_st", module_number);
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
    REGISTER_LONG_CONSTANT("MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA", MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_COMPRESSED", MEMCACHED_COMPRESSED, CONST_CS | CONST_PERSISTENT);

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

// {{{ _php_libmemcached_get_memcached_connection()
static void *_php_libmemcached_get_memcached_connection(zval *obj TSRMLS_DC) {
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
// {{{ _php_libmemcached_connection_resource_dtor()
static void _php_libmemcached_connection_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
    memcached_free((memcached_st *)rsrc->ptr);
}
// }}}
// {{{ _php_libmemcached_server_resource_dtor()
static void _php_libmemcached_server_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
    memcached_quit((memcached_st *)rsrc->ptr);
}
// }}}
// {{{ _php_libmemcached_create()
static void _php_libmemcached_create(zval *obj TSRMLS_DC)
{
    memcached_st *memc = NULL;
    zend_rsrc_list_entry *le;
    if (SUCCESS == zend_hash_find(&EG(persistent_list), "persistent_memc", strlen("persistent_memc")+1, (void*)&le)) {
        if (Z_TYPE_P(le) == le_memc) {
            memc = (memcached_st *)le->ptr;
        }
    }

    if (memc == NULL) {
        memc = memcached_create(NULL);
        if (memc == NULL) {
        }
        zend_rsrc_list_entry le;
        le.type = le_memc;
        le.ptr = memc;
        if (FAILURE == zend_hash_update(&EG(persistent_list),
                    "persistent_memc", strlen("persistent_memc")+1, (void*)&le,
                    sizeof(le), NULL)) {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "Failed to register persistent entry");
        }
    }
    int ret = zend_list_insert(memc, le_memc);
    add_property_resource(obj, "memc", ret);
}
// }}}
// {{{ _php_libmemcached_get_value()
static int _php_libmemcached_get_value(zval *var, char* ret, uint32_t flags TSRMLS_DC) {
    size_t value_len = strlen(ret);
    if (ret == NULL) {
        return -1;
    }
    if (flags & MEMCACHED_COMPRESSED) {
        unsigned long origsize = strlen(ret) + (strlen(ret) / 1000) + 25 + 1;
        char *origbuf = (char *)emalloc(origsize);
        memset(origbuf, 0, origsize);
        uncompress(origbuf, &origsize, ret, strlen(ret));
        ret = (char *)emalloc(origsize);
        strncpy(ret, origbuf, origsize);
        efree(origbuf);
    }

    if (flags & MEMCACHED_SERIALIZED) {
        const char *value_tmp = ret;
        php_unserialize_data_t var_hash;
        PHP_VAR_UNSERIALIZE_INIT(var_hash);
        if (!php_var_unserialize(&var, (const unsigned char **)&value_tmp, (const unsigned char *)(value_tmp + value_len), &var_hash TSRMLS_CC)) {
            ZVAL_FALSE(var);
            PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
            return -1;
        }
        PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
    } else {
        ZVAL_STRINGL(var, ret, strlen(ret), 1);
    }

    if (flags & MEMCACHED_COMPRESSED) {
        efree(ret);
    }
    return 0;
}
// }}}
// {{{ _get_value_from_zval()
static char* _get_value_from_zval(smart_str *buf, zval *var, uint32_t *flags TSRMLS_DC)
{
    php_serialize_data_t var_hash;

    switch (Z_TYPE_P(var)) {
        case IS_STRING:
            smart_str_appends(buf, Z_STRVAL_P(var));
            smart_str_0(buf);
            break;

        case IS_LONG:
        case IS_DOUBLE:
        case IS_BOOL:
            convert_to_string(var);
            smart_str_appends(buf, Z_STRVAL_P(var));
            break;

        default: {
             zval var_copy, *var_copy_ptr;

             var_copy = *var;
             zval_copy_ctor(&var_copy);

             var_copy_ptr = &var_copy;

             PHP_VAR_SERIALIZE_INIT(var_hash);
             php_var_serialize(buf, &var_copy_ptr, &var_hash TSRMLS_CC);
             PHP_VAR_SERIALIZE_DESTROY(var_hash);

             if (!buf->c) {
                 zval_dtor(&var_copy);
                 return NULL;
             }

             *flags |= MEMCACHED_SERIALIZED;
             zval_dtor(&var_copy);
         }   
         break;
    }

    if (*flags & MEMCACHED_COMPRESSED) {
        unsigned long compsize = buf->len + (buf->len / 1000) + 25 + 1;
        char *compbuf = (char *)emalloc(compsize);
        memset(compbuf, 0, compsize);
        if(compress(compbuf, &compsize, buf->c, buf->len) != Z_OK) {
            return NULL;
        }
        smart_str_free(buf);
        smart_str_appends(buf, compbuf);
        efree(compbuf);
    }
    return buf->c;
}
// }}}

// {{{ PHP_FUNCTION(memcached_ctor)
PHP_FUNCTION(memcached_ctor) 
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }
    _php_libmemcached_create(obj TSRMLS_CC);
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

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len) == FAILURE) {
        return;
    }

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    char *ret;
    ret = memcached_get(res_memc, key, strlen(key), &value_len, &flags, &rc);
    if (ret == NULL) {
        RETURN_FALSE
    }
    if (_php_libmemcached_get_value(return_value, ret, flags TSRMLS_CC) < 0) {
        RETURN_FALSE
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
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

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
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);
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
    const char *key = NULL;
    int key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|ll", &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);

    char * val;
    val = _get_value_from_zval(&buf, var, &flags TSRMLS_CC);
    if (val == NULL) {
        RETURN_FALSE;
    }

    memcached_return rc;
    rc = memcached_set(res_memc, key, strlen(key), val, strlen(val), (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_set_by_key)
PHP_FUNCTION(memcached_set_by_key)
{
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz|ll", &master_key, &master_key_len, &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    _get_value_from_zval(&buf, var, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_set_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), buf.c, buf.len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_add)
PHP_FUNCTION(memcached_add)
{
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);

    const char *key = NULL;
    int key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|ll", &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    _get_value_from_zval(&buf, var, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_add(res_memc, key, strlen(key), buf.c, buf.len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_add_by_key)
PHP_FUNCTION(memcached_add_by_key)
{
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz|ll", &master_key, &master_key_len, &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    _get_value_from_zval(&buf, var, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_add_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), buf.c, buf.len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_append)
PHP_FUNCTION(memcached_append)
{
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);

    const char *key = NULL;
    int key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|ll", &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    _get_value_from_zval(&buf, var, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_append(res_memc, key, strlen(key), buf.c, buf.len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_append_by_key)
PHP_FUNCTION(memcached_append_by_key)
{
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz|ll", &master_key, &master_key_len, &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    _get_value_from_zval(&buf, var, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_append_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), buf.c, buf.len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
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
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

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
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    memcached_return rc;
    rc = memcached_behavior_set(res_memc, key, val);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_cas)
PHP_FUNCTION(memcached_cas)
{
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);

    const char *key = NULL;
    int key_len = 0;
    time_t expiration = 0;
    uint32_t flags, cas = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|lll", &key, &key_len, &var, &expiration, &flags, &cas) == FAILURE) {
        return;
    }

    _get_value_from_zval(&buf, var, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_cas(res_memc, key, strlen(key), buf.c, buf.len, (time_t)expiration, (uint16_t)flags, cas);
    smart_str_free(&buf);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_cas_by_key)
PHP_FUNCTION(memcached_cas_by_key)
{
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;
    uint32_t flags, cas = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz|ll", &master_key, &master_key_len, &key, &key_len, &var, &expiration, &flags, &cas) == FAILURE) {
        return;
    }

    _get_value_from_zval(&buf, var, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_cas_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), buf.c, buf.len, (time_t)expiration, (uint16_t)flags, cas);
    smart_str_free(&buf);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_delete)
PHP_FUNCTION(memcached_delete)
{
    zval *obj = getThis();

    const char *key = NULL;
    int key_len, expiration = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &key, &key_len, &expiration) == FAILURE) {
        return;
    }

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    memcached_return rc;
    rc = memcached_delete(res_memc, key, strlen(key), (time_t)expiration);
    if(rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_delete_by_key)
PHP_FUNCTION(memcached_delete_by_key)
{
    zval *obj = getThis();
    memcached_return rc;

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &master_key, &master_key_len, &key, &key_len) == FAILURE) {
        return;
    }

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    rc = memcached_delete_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), expiration);
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
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    memcached_return rc;

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
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    memcached_return rc;

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
// {{{ PHP_FUNCTION(memcached_prepend)
PHP_FUNCTION(memcached_prepend)
{
    const char *key = NULL;
    int key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|ll", &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);

    _get_value_from_zval(&buf, var, &flags TSRMLS_CC);

    memcached_return rc;
    rc = memcached_prepend(res_memc, key, strlen(key), buf.c, buf.len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_prepend_by_key)
PHP_FUNCTION(memcached_prepend_by_key)
{
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz|ll", &master_key, &master_key_len, &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    _get_value_from_zval(&buf, var, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_prepend_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), buf.c, buf.len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_replace)
PHP_FUNCTION(memcached_replace)
{
    const char *key = NULL;
    int key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|ll", &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);

    _get_value_from_zval(&buf, var, &flags TSRMLS_CC);

    memcached_return rc;
    rc = memcached_replace(res_memc, key, strlen(key), buf.c, buf.len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_replace_by_key)
PHP_FUNCTION(memcached_replace_by_key)
{
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz|ll", &master_key, &master_key_len, &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    _get_value_from_zval(&buf, var, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_replace_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), buf.c, buf.len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_server_list)
PHP_FUNCTION(memcached_server_list)
{
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(getThis() TSRMLS_CC);
    memcached_server_st *servers = memcached_server_list(res_memc);
    int x;
    array_init(return_value);

    zval *new_array;
    for (x = 0; x < res_memc->number_of_hosts; x++) {
        MAKE_STD_ZVAL(new_array);
        array_init(new_array);
        add_assoc_stringl(new_array, "hostname", servers[x].hostname, strlen(servers[x].hostname), 1);
        add_assoc_long(new_array, "port", servers[x].port);
        add_index_zval(return_value, x, new_array);
    }
}
// }}}
// {{{ PHP_FUNCTION(memcached_server_list_append)
PHP_FUNCTION(memcached_server_list_append)
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
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);
    if (res_memc == NULL) {
        RETURN_FALSE;
    }

    memcached_return rc;
    LIBMEMCACHED_G(server_list) = memcached_server_list_append(LIBMEMCACHED_G(server_list), hostname, port, &rc);

    if (MEMCACHED_SUCCESS != rc) {
        RETURN_FALSE;
    }
    RETURN_TRUE;

}
// }}}
// {{{ PHP_FUNCTION(memcached_server_push)
PHP_FUNCTION(memcached_server_push)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);
    if (res_memc == NULL) {
        RETURN_FALSE;
    }

    memcached_return rc;
    rc = memcached_server_push(res_memc, LIBMEMCACHED_G(server_list));

    if (MEMCACHED_SUCCESS != rc) {
        RETURN_FALSE;
    }
    RETURN_TRUE;

}
// }}}
// {{{ PHP_FUNCTION(memcached_mget)
PHP_FUNCTION(memcached_mget)
{
    zval *obj = getThis();
    memcached_return rc;

    zval *var;
    char **keys;
    int *key_length, number_of_keys, i;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &var) == FAILURE) {
        return;
    }

    HashTable *hash;
    hash = Z_ARRVAL_P(var);
    number_of_keys = zend_hash_num_elements(hash);

    zval **data;
    keys = (char **)emalloc(number_of_keys * sizeof(char *));
    key_length = (int *)emalloc(number_of_keys * sizeof(int));
    for(i=0; i<number_of_keys; i++){
        if(zend_hash_get_current_data(hash, (void **)&data) == FAILURE) {
            return;
        }

        if (Z_TYPE_PP(data) != IS_STRING) {
            continue;
        }
        keys[i] = Z_STRVAL_PP(data);
        key_length[i] = strlen(keys[i]);
        zend_hash_move_forward(hash);
    }

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    rc = memcached_mget(res_memc, keys, key_length, number_of_keys);
    efree(keys);
    efree(key_length);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_fetch)
PHP_FUNCTION(memcached_fetch)
{
    zval *obj = getThis();
    memcached_return rc;

    zval *value;
    MAKE_STD_ZVAL(value);

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    memcached_result_st *result = NULL;
    result = memcached_fetch_result(res_memc, result, &rc);
    if (result != NULL ) {
        array_init(return_value);
        _php_libmemcached_get_value(value, result->value.string, result->flags TSRMLS_CC);
        add_assoc_string(return_value, "key", result->key, 1);
        add_assoc_zval(return_value, "value", value);
    }
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
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
