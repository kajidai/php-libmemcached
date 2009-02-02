--TEST--
Check for decrement() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Libmemcached();
// $memcached->behavior_set(MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
// $memcached->behavior_set(MEMCACHED_BEHAVIOR_SND_TIMEOUT, 10);
// $memcached->behavior_set(MEMCACHED_BEHAVIOR_RCV_TIMEOUT, 10);
$ret = $memcached->addserver('10.0.26.1', 11211);
// $ret = $memcached->addserver('12.3.4.6', 11211);
// $ret = $memcached->addserver('12.3.5.6', 11211);

$ret = $memcached->delete('key1');
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->set('key1', 10);
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->decrement('key1', 1);
var_dump($ret);
$ret = $memcached->decrement('key1', 1);
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);

?>
--EXPECT--
bool(true)
bool(false)
bool(true)
string(2) "10"
int(9)
int(8)
string(1) "8"
