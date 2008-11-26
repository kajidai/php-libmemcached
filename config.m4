dnl $Id$
dnl config.m4 for extension libmemcached

PHP_ARG_WITH(libmemcached,  whether to enable the libmemcached extension,
[  --with-libmemcached[=DIR]   Enables the libmemcached extension. DIR is the prefix to libmemcached installation directory.], no)

if test "$PHP_LIBMEMCACHED" != "no"; then
	
  AC_MSG_CHECKING([for the location of libmemcached])

  if test "$PHP_LIBMEMCACHED" != yes; then
    if test -f "$PHP_LIBMEMCACHED/include/libmemcached/memcached.h"; then
      PHP_LIBMEMCACHED_PREFIX="$PHP_LIBMEMCACHED"
    else
      AC_MSG_ERROR(Can not find libmemcached headers in $PHP_LIBMEMCACHED)
    fi
  else 
    for i in /usr/local /usr; do
      if test -f "$i/include/libmemcached/memcached.h"; then
        PHP_LIBMEMCACHED_PREFIX="$i"
        break;
      fi
    done
  
    if test -z "$PHP_LIBMEMCACHED_PREFIX"; then
      AC_MSG_ERROR(Can not find libmemcached headers)
    fi
  fi

  PHP_LIBMEMCACHED_DIR="$PHP_LIBMEMCACHED_PREFIX"
  PHP_LIBMEMCACHED_INCDIR="$PHP_LIBMEMCACHED_PREFIX/include"

  AC_MSG_RESULT([$PHP_LIBMEMCACHED_PREFIX])
  PHP_ADD_INCLUDE($PHP_LIBMEMCACHED_INCDIR)
  PHP_ADD_LIBRARY_WITH_PATH(z, $PHP_LIBMEMCACHED_DIR/$PHP_LIBDIR, ZIP_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(memcached, $PHP_LIBMEMCACHED_DIR/lib, LIBMEMCACHED_SHARED_LIBADD)
  PHP_NEW_EXTENSION(libmemcached, libmemcached.c, $ext_shared)
  PHP_SUBST(LIBMEMCACHED_SHARED_LIBADD)

fi
