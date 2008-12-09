--TEST--
Check for decrement() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Memcached();
$ret = $memcached->addserver('localhost', 11211);

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
2
string(2) "10"
int(9)
int(8)
2
string(1) "8"
