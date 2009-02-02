--TEST--
Check for gets(), cas() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Libmemcached();
$ret = $memcached->addserver('localhost', 11211);

$memcached->delete('key1');
$ret = $memcached->gets('key1');
var_dump($ret);
$memcached->set('key1', 'val1');
$ret = $memcached->gets('key1');
$ret = $memcached->cas('key1', 'cas1', 0, 0, $ret['cas']);
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->cas('key1', 'cas2', 0, 0, 100);
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);


$memcached->delete('key1');
$ret = $memcached->gets('key1');
var_dump($ret);
$memcached->set('key1', 'val1', 0, MEMCACHED_COMPRESSED);
$ret = $memcached->gets('key1');
$ret = $memcached->cas('key1', 'cas1', 0, MEMCACHED_COMPRESSED, $ret['cas']);
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->cas('key1', 'cas2', 0, MEMCACHED_COMPRESSED, 100);
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);


?>
--EXPECT--
bool(false)
bool(true)
string(4) "cas1"
bool(false)
string(4) "cas1"
bool(false)
bool(true)
string(4) "cas1"
bool(false)
string(4) "cas1"
