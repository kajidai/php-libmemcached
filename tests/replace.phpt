--TEST--
Check for replace() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Memcached();
// $memcached->behavior_set(MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, 1);
$memcached->behavior_set(MEMCACHED_BEHAVIOR_SND_TIMEOUT, 10);
// $memcached->behavior_set(MEMCACHED_BEHAVIOR_RCV_TIMEOUT, 10);

$ret = $memcached->addserver('localhost', 11211);

$ret = $memcached->delete('key1');
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->set('key1', 'val1');
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->replace('key1', 'replace_val1');
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);

$ret = $memcached->delete('key1');
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->set('key1', 'val1', 0, MEMCACHED_COMPRESSED);
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->replace('key1', 'replace_val1', 0, MEMCACHED_COMPRESSED);
var_dump($ret);
$ret = $memcached->get('key1');
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
