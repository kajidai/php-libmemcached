--TEST--
Check for delete() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Memcached();
$ret = $memcached->addserver('localhost', 11211);
$memcached->server_push();

$ret = $memcached->set('key1', 'val1');
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->delete('key1');
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
?>
--EXPECT--
bool(true)
string(4) "val1"
bool(true)
bool(false)
