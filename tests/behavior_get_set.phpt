--TEST--
Check for behavior_get(), behavior_set()
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Libmemcached();
$ret = $memcached->behavior_get(MEMCACHED_BEHAVIOR_NO_BLOCK);
var_dump($ret);
$memcached->behavior_set(MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
$ret = $memcached->behavior_get(MEMCACHED_BEHAVIOR_NO_BLOCK);
var_dump($ret);
?>
--EXPECT--
float(0)
float(1)
