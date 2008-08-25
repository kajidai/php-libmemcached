dnl $Id$
dnl config.m4 for extension libmemcached

PHP_ARG_ENABLE(libmemcached, whether to enable libmemcached support,
Make sure that the comment is aligned:
[  --enable-libmemcached           Enable libmemcached support])

if test "$PHP_LIBMEMCACHED" != "no"; then
  PHP_ADD_LIBRARY_WITH_PATH(memcached, /usr/local/lib, LIBMEMCACHED_SHARED_LIBADD)
  export CFLAGS=$CFLAGS" -Werror"
  PHP_NEW_EXTENSION(libmemcached, libmemcached.c, $ext_shared)
  PHP_SUBST(LIBMEMCACHED_SHARED_LIBADD)
fi
