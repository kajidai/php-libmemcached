/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2008 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Daisuke Kajiwara                                             |
  +----------------------------------------------------------------------+
*/
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
static void _php_libmemcached_create(zval *, const char * TSRMLS_DC);
static int _php_libmemcached_get_value(zval *, char *, size_t, uint32_t TSRMLS_DC);
static char* _get_value_from_zval(smart_str *, zval *, size_t *, uint32_t * TSRMLS_DC);

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
    PHP_FE(memcached_gets, NULL)
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
    PHP_FE(memcached_fetchall, NULL)
    PHP_FE(memcached_server_list_append, NULL)
    PHP_FE(memcached_server_push, NULL)
    PHP_FE(memcached_stat, NULL)
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
    PHP_FALIAS(gets, memcached_gets, NULL)
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
    PHP_FALIAS(fetchall, memcached_fetchall, NULL)
    PHP_FALIAS(stat, memcached_stat, NULL)
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
    le_memc = zend_register_list_destructors_ex(_php_libmemcached_server_resource_dtor, _php_libmemcached_connection_resource_dtor, "memcached_st", module_number);
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
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_SND_TIMEOUT", MEMCACHED_BEHAVIOR_SND_TIMEOUT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_BEHAVIOR_RCV_TIMEOUT", MEMCACHED_BEHAVIOR_RCV_TIMEOUT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_DISTRIBUTION_MODULA", MEMCACHED_DISTRIBUTION_MODULA, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_DISTRIBUTION_CONSISTENT", MEMCACHED_DISTRIBUTION_CONSISTENT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA", MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_HASH_DEFAULT", MEMCACHED_HASH_DEFAULT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_HASH_MD5", MEMCACHED_HASH_MD5, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_HASH_CRC", MEMCACHED_HASH_CRC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_HASH_FNV1_64", MEMCACHED_HASH_FNV1_64, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_HASH_FNV1A_64", MEMCACHED_HASH_FNV1A_64, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_HASH_FNV1_32", MEMCACHED_HASH_FNV1_32, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_HASH_FNV1A_32", MEMCACHED_HASH_FNV1A_32, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_HASH_HSIEH", MEMCACHED_HASH_HSIEH, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("MEMCACHED_HASH_MURMUR", MEMCACHED_HASH_MURMUR, CONST_CS | CONST_PERSISTENT);

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
static void _php_libmemcached_create(zval *obj, const char *key TSRMLS_DC)
{
    memcached_st *memc = NULL;
    zend_rsrc_list_entry *le;
    char *hash_key = NULL;
    if (key != NULL) {
        hash_key = (char *) emalloc(strlen("persistent_memc_") + strlen(key) + 1);
        sprintf(hash_key, "persistent_memc_%s", key);

        if (SUCCESS == zend_hash_find(&EG(persistent_list), hash_key, strlen(hash_key)+1, (void*)&le)) {
            if (Z_TYPE_P(le) == le_memc) {
                memc = (memcached_st *)le->ptr;
            }
        }
    }

    if (memc == NULL) {
        memc = memcached_create(NULL);
        if (memc == NULL) {
        }
        zend_rsrc_list_entry le;
        le.type = le_memc;
        le.ptr = memc;
        if (key != NULL) {
            if (FAILURE == zend_hash_update(&EG(persistent_list),
                        hash_key, strlen(hash_key)+1, (void*)&le,
                        sizeof(le), NULL)) {
                php_error_docref(NULL TSRMLS_CC, E_ERROR, "Failed to register persistent entry");
            }
        }
    }
    int ret = zend_list_insert(memc, le_memc);
    add_property_resource(obj, "memc", ret);
    if (key != NULL) {
        efree(hash_key);
    }

    // unset global value
    memcached_server_list_free(LIBMEMCACHED_G(server_list));
    LIBMEMCACHED_G(server_list) = NULL;
}
// }}}
// {{{ _php_libmemcached_get_value()
static int _php_libmemcached_get_value(zval *var, char* ret, size_t ret_len, uint32_t flags TSRMLS_DC) {
    if (ret == NULL) {
        return -1;
    }
    if (flags & MEMCACHED_COMPRESSED) {
        // from PHP_FUNCTION(gzuncompress)
        unsigned int factor=1, maxfactor=16;
        unsigned long length;
        int status;
        char *s2 = NULL;
        do {
            length = (unsigned long)ret_len * (1 << factor++);
            s2 = (char *) erealloc(s2, length+1);
            memset(s2, 0, length+1);
            status = uncompress((unsigned char *)s2, &length, (unsigned const char *)ret, ret_len);
        } while ((status==Z_BUF_ERROR) && (factor < maxfactor));

        ret = (char *)emalloc(length+1);
        memset(ret, 0, length+1);
        memcpy(ret, s2, length);
        ret_len = length;
        efree(s2);
        if (status != Z_OK) {
            efree(ret);
            return -1;
        }
    }

    if (flags & MEMCACHED_SERIALIZED) {
        const unsigned char *value_tmp = (unsigned char *)ret;
        php_unserialize_data_t var_hash;
        PHP_VAR_UNSERIALIZE_INIT(var_hash);
        if (!php_var_unserialize(&var, &value_tmp, value_tmp + ret_len, &var_hash TSRMLS_CC)) {
            ZVAL_FALSE(var);
            PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
            if (flags & MEMCACHED_COMPRESSED) {
                efree(ret);
            }
            return -1;
        }
        PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
    } else {
        ret[ret_len] = '\0';
        ZVAL_STRINGL(var, ret, ret_len, 1);
    }

    if (flags & MEMCACHED_COMPRESSED) {
        efree(ret);
    }
    return 0;
}
// }}}
// {{{ _get_value_from_zval()
static char* _get_value_from_zval(smart_str *buf, zval *var, size_t *var_len, uint32_t *flags TSRMLS_DC)
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
            smart_str_0(buf);
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
        if(compress((unsigned char *)compbuf, &compsize, (unsigned const char *)buf->c, buf->len) != Z_OK) {
            free(compbuf);
            return NULL;
        }
        *var_len = compsize;
        return compbuf;
    }

    *var_len = buf->len;
    char *compbuf = (char *)emalloc(buf->len+1);
    memcpy(compbuf, buf->c, buf->len);
    compbuf[buf->len] = '\0';
    return compbuf;
}
// }}}

// {{{ PHP_FUNCTION(memcached_ctor)
PHP_FUNCTION(memcached_ctor) 
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    const char *key = NULL;
    size_t key_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &key, &key_len) == FAILURE) {
        return;
    }

    _php_libmemcached_create(obj, key TSRMLS_CC);
}
// }}}
// {{{ PHP_FUNCTION(memcached_get)
PHP_FUNCTION(memcached_get)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    memcached_return rc;

    uint32_t flags;
    const char *key = NULL;
    size_t key_len, value_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len) == FAILURE) {
        return;
    }
    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    char *ret = NULL;
    ret = memcached_get(res_memc, key, key_len, &value_len, &flags, &rc);
    if ((ret == NULL) || (rc != MEMCACHED_SUCCESS)) {
        RETURN_FALSE
    }
    if (_php_libmemcached_get_value(return_value, ret, value_len, flags TSRMLS_CC) < 0) {
        RETURN_FALSE
    }
}
// }}}
// {{{ PHP_FUNCTION(memcached_gets)
PHP_FUNCTION(memcached_gets)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    memcached_return rc;

    uint32_t flags;
    char *key = NULL;
    size_t key_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len) == FAILURE) {
        return;
    }
    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    size_t number_of_keys = 1;
    char **keys = (char **)emalloc(1 * sizeof(char *));
    size_t *key_length = (size_t *)emalloc(1 * sizeof(size_t));
    keys[0] = key;
    key_length[0] = key_len;

    memcached_behavior_set(res_memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, 1);
    rc = memcached_mget(res_memc, keys, key_length, number_of_keys);
    efree(keys);
    efree(key_length);

    memcached_result_st *results;
    memcached_result_st results_obj;
    results= memcached_result_create(res_memc, &results_obj);
    results= memcached_fetch_result(res_memc, &results_obj, &rc);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE
    }

    flags = memcached_result_flags(results);

    uint64_t cas = memcached_result_cas(results);

    zval *value;
    MAKE_STD_ZVAL(value);
    if (_php_libmemcached_get_value(value, memcached_result_value(results), 
                memcached_result_length(results), flags TSRMLS_CC) < 0) {
        RETURN_FALSE
    }
    array_init(return_value);
    add_assoc_zval(return_value, "value", value);
    add_assoc_long(return_value, "cas", cas);
}
// }}}
// {{{ PHP_FUNCTION(memcached_get_by_key)
PHP_FUNCTION(memcached_get_by_key)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    memcached_return rc;
    uint32_t flags;
    const char *key, *master_key = NULL;
    int key_len, master_key_len = 0;
    size_t value_len = 0;
    char *ret;

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &master_key, &master_key_len, &key, &key_len) == FAILURE) {
        return;
    }

    ret = memcached_get_by_key(res_memc, master_key, master_key_len, key, key_len, &value_len, &flags, &rc);
    if ((ret == NULL) || (rc != MEMCACHED_SUCCESS)) {
        RETURN_FALSE;
    }

    if (_php_libmemcached_get_value(return_value, ret, value_len, flags TSRMLS_CC) < 0) {
        RETURN_FALSE
    }
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
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    const char *key = NULL;
    int key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|ll", &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    char * val;
    size_t len;
    val = _get_value_from_zval(&buf, var, &len, &flags TSRMLS_CC);
    if (val == NULL) {
        RETURN_FALSE;
    }

    memcached_return rc;
    rc = memcached_set(res_memc, key, strlen(key), val, len, (time_t)expiration, (uint16_t)flags);
    if (buf.c != NULL) {
        smart_str_free(&buf);
    }
    efree(val);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_set_by_key)
PHP_FUNCTION(memcached_set_by_key)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz|ll", &master_key, &master_key_len, &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    size_t len;
    char *val;
    val = _get_value_from_zval(&buf, var, &len, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_set_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), val, len, (time_t)expiration, (uint16_t)flags);
    efree(val);
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
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    const char *key = NULL;
    int key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|ll", &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    size_t len;
    char *val;
    val = _get_value_from_zval(&buf, var, &len, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_add(res_memc, key, strlen(key), val, len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    efree(val);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_add_by_key)
PHP_FUNCTION(memcached_add_by_key)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz|ll", &master_key, &master_key_len, &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    size_t len;
    char *val;
    val = _get_value_from_zval(&buf, var, &len, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_add_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), val, len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    efree(val);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_append)
PHP_FUNCTION(memcached_append)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    const char *key = NULL;
    int key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|ll", &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    size_t len;
    char *val;
    val = _get_value_from_zval(&buf, var, &len, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_append(res_memc, key, strlen(key), val, len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    efree(val);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_append_by_key)
PHP_FUNCTION(memcached_append_by_key)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz|ll", &master_key, &master_key_len, &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    size_t len;
    char *val;
    val = _get_value_from_zval(&buf, var, &len, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_append_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), val, len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    efree(val);
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

    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }
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

    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }
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
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    const char *key = NULL;
    int key_len = 0;
    time_t expiration = 0;
    uint32_t flags, cas = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "szlll", &key, &key_len, &var, &expiration, &flags, &cas) == FAILURE) {
        return;
    }

    size_t len;
    char *val;
    val = _get_value_from_zval(&buf, var, &len, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_cas(res_memc, key, strlen(key), val, len, (time_t)expiration, (uint16_t)flags, cas);
    smart_str_free(&buf);
    efree(val);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_cas_by_key)
PHP_FUNCTION(memcached_cas_by_key)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }
    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;
    uint32_t flags, cas = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz|ll", &master_key, &master_key_len, &key, &key_len, &var, &expiration, &flags, &cas) == FAILURE) {
        return;
    }

    size_t len;
    char *val;
    val = _get_value_from_zval(&buf, var, &len, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_cas_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), val, len, (time_t)expiration, (uint16_t)flags, cas);
    smart_str_free(&buf);
    efree(val);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_delete)
PHP_FUNCTION(memcached_delete)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

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
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }
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
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }
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
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }
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
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    const char *key = NULL;
    int key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|ll", &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    size_t len;
    char *val = NULL;
    val = _get_value_from_zval(&buf, var, &len, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_prepend(res_memc, key, key_len, val, len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    efree(val);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_prepend_by_key)
PHP_FUNCTION(memcached_prepend_by_key)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz|ll", &master_key, &master_key_len, &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    size_t len;
    char *val = NULL;
    _get_value_from_zval(&buf, var, &len, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_prepend_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), val, len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    efree(val);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_replace)
PHP_FUNCTION(memcached_replace)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    const char *key = NULL;
    int key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|ll", &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    size_t len;
    char *val = NULL;
    _get_value_from_zval(&buf, var, &len, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_replace(res_memc, key, strlen(key), val, len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    efree(val);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_replace_by_key)
PHP_FUNCTION(memcached_replace_by_key)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    const char *master_key, *key = NULL;
    int master_key_len, key_len = 0;
    time_t expiration = 0;
    uint32_t flags = 0;
    zval *var;
    smart_str buf = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz|ll", &master_key, &master_key_len, &key, &key_len, &var, &expiration, &flags) == FAILURE) {
        return;
    }

    size_t len;
    char *val = NULL;
    _get_value_from_zval(&buf, var, &len, &flags TSRMLS_CC);
    memcached_return rc;
    rc = memcached_replace_by_key(res_memc, master_key, strlen(master_key), key, strlen(key), val, len, (time_t)expiration, (uint16_t)flags);
    smart_str_free(&buf);
    efree(val);
    if (rc != MEMCACHED_SUCCESS) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
// }}}
// {{{ PHP_FUNCTION(memcached_server_list)
PHP_FUNCTION(memcached_server_list)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    memcached_st *res_memc = _php_libmemcached_get_memcached_connection(obj TSRMLS_CC);
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
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    memcached_return rc;

    zval *var;
    char **keys;
    size_t *key_length, number_of_keys;
    int i;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &var) == FAILURE) {
        return;
    }

    HashTable *hash;
    hash = Z_ARRVAL_P(var);
    number_of_keys = zend_hash_num_elements(hash);

    zval **data;
    keys = (char **)emalloc(number_of_keys * sizeof(char *));
    key_length = (size_t *)emalloc(number_of_keys * sizeof(size_t));
    for(i=0; i<number_of_keys; i++){
        if(zend_hash_get_current_data(hash, (void **)&data) == FAILURE) {
            return;
        }

        if (Z_TYPE_PP(data) != IS_STRING) {
            continue;
        }
        keys[i] = Z_STRVAL_PP(data);
        key_length[i] = Z_STRLEN_PP(data);
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
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    char return_key[MEMCACHED_MAX_KEY];
    size_t return_key_length;
    char *ret;
    size_t return_value_length;
    uint32_t flags;
    memcached_return rc;

    ret = memcached_fetch(res_memc, return_key, &return_key_length, &return_value_length, &flags, &rc);
    if ((ret == NULL) || (rc != MEMCACHED_SUCCESS)) {
        RETURN_FALSE
    }
    return_key[return_key_length] = '\0';

    zval *value;
    MAKE_STD_ZVAL(value);
    if (_php_libmemcached_get_value(value, ret, return_value_length, flags TSRMLS_CC) < 0) {
        RETURN_FALSE
    }

    array_init(return_value);
    add_assoc_zval(return_value, return_key, value);
}
// }}}
// {{{ PHP_FUNCTION(memcached_fetchall)
PHP_FUNCTION(memcached_fetchall)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    memcached_return rc;

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    char return_key[MEMCACHED_MAX_KEY];
    size_t return_key_length;
    char *ret;
    size_t return_value_length;
    uint32_t flags;
    array_init(return_value);

    while ((ret= memcached_fetch(res_memc, return_key, &return_key_length, &return_value_length, &flags, &rc)) != NULL)
    {
        return_key[return_key_length] = '\0';

        zval *value;
        MAKE_STD_ZVAL(value);
        if (_php_libmemcached_get_value(value, ret, return_value_length, flags TSRMLS_CC) < 0) {
            RETURN_FALSE
        }

        add_assoc_zval(return_value, return_key, value);
    }
}
// }}}
// {{{ PHP_FUNCTION(memcached_stat)
PHP_FUNCTION(memcached_stat)
{
    zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
    if (!obj) {
        RETURN_FALSE;
    }

    char *key = NULL;
    size_t key_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &key, &key_len) == FAILURE) {
        return;
    }

    memcached_st *res_memc = NULL;
    res_memc = (memcached_st *)_php_libmemcached_get_memcached_connection(obj TSRMLS_CC);

    memcached_stat_st *stats;
    memcached_return rc;
    stats = memcached_stat(res_memc, key, &rc);
    array_init(return_value);
    add_assoc_long(return_value, "pid", stats->pid);
    add_assoc_long(return_value, "uptime", stats->uptime);
    add_assoc_long(return_value, "threads", stats->threads);
    add_assoc_long(return_value, "time", stats->time);
    add_assoc_long(return_value, "pointer_size", stats->pointer_size);
    add_assoc_long(return_value, "rusage_user_seconds", stats->rusage_user_seconds);
    add_assoc_long(return_value, "rusage_user_microseconds", stats->rusage_user_microseconds);
    add_assoc_long(return_value, "rusage_system_seconds", stats->rusage_system_seconds);
    add_assoc_long(return_value, "rusage_system_microseconds", stats->rusage_system_microseconds);
    add_assoc_long(return_value, "curr_items", stats->curr_items);
    add_assoc_long(return_value, "total_items", stats->total_items);
    add_assoc_long(return_value, "limit_maxbytes", stats->limit_maxbytes);
    add_assoc_long(return_value, "curr_connections", stats->curr_connections);
    add_assoc_long(return_value, "total_connections", stats->total_connections);
    add_assoc_long(return_value, "connection_structures", stats->connection_structures);
    add_assoc_long(return_value, "bytes", stats->bytes);
    add_assoc_long(return_value, "cmd_get", stats->cmd_get);
    add_assoc_long(return_value, "cmd_set", stats->cmd_set);
    add_assoc_long(return_value, "get_hits", stats->get_hits);
    add_assoc_long(return_value, "get_misses", stats->get_misses);
    add_assoc_long(return_value, "evictions", stats->evictions);
    add_assoc_long(return_value, "bytes_read", stats->bytes_read);
    add_assoc_long(return_value, "bytes_written", stats->bytes_written);
    add_assoc_stringl(return_value, "version", stats->version, strlen(stats->version), 1);
    free(stats);
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
