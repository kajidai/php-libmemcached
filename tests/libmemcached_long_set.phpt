--TEST--
Check for server_list() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Memcached();
$ret = $memcached->addserver('localhost', 11211);

$memcached->delete('key1');
$ret = $memcached->get('key1');
var_dump($ret);
$memcached->set('key1', 100);
$ret = $memcached->get('key1');
var_dump($ret);

$memcached->delete('key1');
$ret = $memcached->get('key1');
var_dump($ret);
$memcached->set('key1', 100, 0, MEMCACHED_COMPRESSED);
$ret = $memcached->get('key1');
var_dump($ret);
--EXPECT--
bool(false)
string(3) "100"
bool(false)
string(3) "100"
