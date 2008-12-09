--TEST--
Check for mget(), fetch() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Memcached();
$ret = $memcached->addserver('localhost', 11211);

$memcached->set('key1', array('val1'));
$memcached->set('key2', 'val2');
$memcached->set('key3', 'val3');

$ret = $memcached->mget(array('key1', 'key2'));
var_dump($ret);
$ret = $memcached->fetch();
var_dump($ret);
$ret = $memcached->fetch();
var_dump($ret);
$ret = $memcached->fetch();
var_dump($ret);
?>
--EXPECT--
bool(true)
array(1) {
  ["key1"]=>
  array(1) {
    [0]=>
    string(4) "val1"
  }
}
array(1) {
  ["key2"]=>
  string(4) "val2"
}
bool(false)
