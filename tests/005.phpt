--TEST--
Check for set array value
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Memcached();
$ret = $memcached->addserver('localhost', 11211);
$ret = $memcached->set('key1', 'val1', 0);
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->set('key_array1', array('val1'), 0, MEMCACHED_COMPRESSED);
var_dump($ret);
$ret = $memcached->get('key_array1');
var_dump($ret);
?>
--EXPECT--
bool(true)
string(4) "val1"
bool(true)
array(1) {
  [0]=>
  string(4) "val1"
}
