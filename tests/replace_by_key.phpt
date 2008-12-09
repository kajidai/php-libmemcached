--TEST--
Check for replace_by_key() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Memcached();
$ret = $memcached->addserver('localhost', 11211);

$ret = $memcached->delete_by_key('master1', 'key1');
var_dump($ret);
$ret = $memcached->get_by_key('master1', 'key1');
var_dump($ret);
$ret = $memcached->set_by_key('master1', 'key1', 'val1');
var_dump($ret);
$ret = $memcached->get_by_key('master1', 'key1');
var_dump($ret);
$ret = $memcached->replace_by_key('master1', 'key1', 'replace_val1');
var_dump($ret);
$ret = $memcached->get_by_key('master1', 'key1');
var_dump($ret);

$ret = $memcached->delete_by_key('master1', 'key1');
var_dump($ret);
$ret = $memcached->get_by_key('master1', 'key1');
var_dump($ret);
$ret = $memcached->set_by_key('master1', 'key1', 'val1', 0, MEMCACHED_COMPRESSED);
var_dump($ret);
$ret = $memcached->get_by_key('master1', 'key1');
var_dump($ret);
$ret = $memcached->replace_by_key('master1', 'key1', 'replace_val1', 0, MEMCACHED_COMPRESSED);
var_dump($ret);
$ret = $memcached->get_by_key('master1', 'key1');
var_dump($ret);
?>
--EXPECT--
bool(true)
bool(false)
bool(true)
string(4) "val1"
bool(true)
string(12) "replace_val1"
bool(true)
bool(false)
bool(true)
string(4) "val1"
bool(true)
string(12) "replace_val1"
