--TEST--
Check for addServer()
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Libmemcached();
$ret = $memcached->addserver('localhost', 11211);
var_dump($ret);
?>
--EXPECT--
bool(true)
