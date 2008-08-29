dnl $Id$
dnl config.m4 for extension libmemcached

PHP_ARG_ENABLE(libmemcached, whether to enable libmemcached support,
Make sure that the comment is aligned:
[  --enable-libmemcached           Enable libmemcached support])

if test -z "$PHP_LIBMEMCACHED_DIR"; then
PHP_ARG_WITH(libmemcached-dir, for the location of libmemcached (--with specified),
[  --with-libmemcached-dir[=DIR]   Set the path to libmemcached install prefix.], no, no)
fi

if test "$PHP_LIBMEMCACHED" != "no"; then
  if test "$PHP_LIBMEMCACHED_DIR" != "no" && test "$PHP_LIBMEMCACHED_DIR" != "yes"; then
    if test -f "$PHP_LIBMEMCACHED_DIR/include/libmemcached/memcached.h"; then
      PHP_LIBMEMCACHED_DIR="$PHP_LIBMEMCACHED_DIR"
      PHP_LIBMEMCACHED_INCDIR="$PHP_LIBMEMCACHED_DIR/include"
    else
      AC_MSG_ERROR([Can't find libmemcached headers under "$PHP_LIBMEMCACHED_DIR"])
    fi
  else
    for i in /usr/local /usr; do
      if test -f "$i/include/libmemcached/memcached.h"; then
        PHP_LIBMEMCACHED_DIR="$i"
        PHP_LIBMEMCACHED_INCDIR="$i/include"
      fi
    done
  fi

  dnl # libmemcached
  AC_MSG_CHECKING([for the location of libmemcached])
  if test "$PHP_LIBMEMCACHED_DIR" = "no"; then
    AC_MSG_ERROR([libmemcached support requires libmemcached installation. use --with-libmemcached-dir=<DIR> to specify prefix where libmemcached include and library are located])
  else
    AC_MSG_RESULT([$PHP_LIBMEMCACHED_DIR])
    PHP_ADD_LIBRARY_WITH_PATH(z, $PHP_LIBMEMCACHED_DIR/$PHP_LIBDIR, ZIP_SHARED_LIBADD)
    PHP_ADD_INCLUDE($PHP_LIBMEMCACHED_INCDIR)
  fi

  PHP_ADD_LIBRARY_WITH_PATH(memcached, $PHP_LIBMEMCACHED_DIR/lib, LIBMEMCACHED_SHARED_LIBADD)
  export CFLAGS=$CFLAGS" -Werror"
  PHP_NEW_EXTENSION(libmemcached, libmemcached.c, $ext_shared)
  PHP_SUBST(LIBMEMCACHED_SHARED_LIBADD)
fi
