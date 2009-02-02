--TEST--
Check for increment() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Libmemcached();
// $memcached->behavior_set(MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, 1);
$memcached->behavior_set(MEMCACHED_BEHAVIOR_SND_TIMEOUT, 10);
// $memcached->behavior_set(MEMCACHED_BEHAVIOR_RCV_TIMEOUT, 10);

$ret = $memcached->addserver('localhost', 11211);

$ret = $memcached->delete('key1');
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->set('key1', 9);
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->increment('key1', 1);
var_dump($ret);
$ret = $memcached->increment('key1', 1);
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);

?>
--EXPECT--
bool(true)
bool(false)
bool(true)
string(1) "9"
int(10)
int(11)
string(2) "11"
