--TEST--
Check for set(), get() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Memcached();
$ret = $memcached->addserver('localhost', 11211);

$memcached->delete('key1');
$ret = $memcached->get('key1');
var_dump($ret);
$memcached->set('key1', true);
$ret = $memcached->get('key1');
var_dump($ret);

$memcached->delete('key1');
$ret = $memcached->get('key1');
var_dump($ret);
$memcached->set('key1', false);
$ret = $memcached->get('key1');
var_dump($ret);

exit;
$memcached->delete('key1');
$ret = $memcached->get('key1');
var_dump($ret);
$memcached->set('key1', true, 0, MEMCACHED_COMPRESSED);
$ret = $memcached->get('key1');
var_dump($ret);

$memcached->delete('key1');
$ret = $memcached->get('key1');
var_dump($ret);
$memcached->set('key1', false, 0, MEMCACHED_COMPRESSED);
$ret = $memcached->get('key1');
var_dump($ret);

--EXPECT--
bool(false)
string(1) "1"
bool(false)
string(0) ""
bool(false)
string(1) "1"
bool(false)
string(0) ""
