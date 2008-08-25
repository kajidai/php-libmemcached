#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_libmemcached.h"

#include "libmemcached/memcached.h"

/* If you declare any globals in php_libmemcached.h uncomment this:
*/
ZEND_DECLARE_MODULE_GLOBALS(libmemcached)

/* True global resources - no need for thread safety here */
static int le_memc;
static int le_servers;
static zend_class_entry *memcached_entry_ptr = NULL;

static void _libmemcached_connection_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC);

/* {{{ libmemcached_functions[]
 *
 * Every user visible function must have an entry in libmemcached_functions[].
 */
zend_function_entry libmemcached_functions[] = {
	PHP_FE(memcached_add, NULL)
	PHP_FE(memcached_get, NULL)
	PHP_FE(memcached_server_add, NULL)
	PHP_FE(memcached_set, NULL)
	PHP_FE(memcached_ctor, NULL)
	{NULL, NULL, NULL}	/* Must be the last line in libmemcached_functions[] */
};
zend_function_entry memcached_functions[] = {
	PHP_FALIAS(memcached, memcached_ctor, NULL)
	PHP_FALIAS(create, memcached_create, NULL)
	PHP_FALIAS(addserver, memcached_server_add, NULL)
	PHP_FALIAS(set, memcached_set, NULL)
	PHP_FALIAS(get, memcached_get, NULL)
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
	PHP_RINIT(libmemcached),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(libmemcached),	/* Replace with NULL if there's nothing to do at request end */
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

static void _libmemcached_connection_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
	memcached_free((memcached_st *)rsrc->ptr TSRMLS_CC);
}

// {{{ PHP_FUNCTION(memcached_ctor)
PHP_FUNCTION(memcached_ctor){}
// }}}
// {{{ PHP_FUNCTION(memcached_create)
PHP_FUNCTION(memcached_create)
{
	zval *obj = LIBMEMCACHED_GET_THIS(memcached_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

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
// {{{ PHP_FUNCTION(memcached_get)
PHP_FUNCTION(memcached_get)
{
	zval *obj = getThis();
	memcached_return rc;

	uint32_t flags;
	const char *key = NULL;
	int key_len, val_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len) == FAILURE) {
		return;
	}

	memcached_st *res_memc = NULL;
	res_memc = (memcached_st *)_libmemcached_get_memcached_connection(obj);

	char *ret;
	ret = memcached_get(res_memc, key, strlen(key), &val_len, &flags, &rc);
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
	const char *key, *val = NULL;
	int key_len, val_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &key, &key_len, &val, &val_len) == FAILURE) {
		return;
	}

	zval *obj = getThis();
	memcached_st *res_memc = _libmemcached_get_memcached_connection(obj);

	memcached_return rc;
	rc = memcached_set(res_memc, key, strlen(key), val, strlen(val), (time_t)0, (uint16_t)0);
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
