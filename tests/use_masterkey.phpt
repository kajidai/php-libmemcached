--TEST--
Check for memcached_*_by_key() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 

function memcached_by_key($memcached, $master_key, $key, $value) {
    $memcached->delete_by_key($master_key, $key);
    $ret = $memcached->get_by_key($master_key, $key);
    var_dump($ret);
    $memcached->set_by_key($master_key, $key, $value);
    $ret = $memcached->get_by_key($master_key, $key);
    var_dump($ret);

    $memcached->delete_by_key($master_key, $key);
    $ret = $memcached->get_by_key($master_key, $key);
    var_dump($ret);
    $memcached->set_by_key($master_key, $key, $value, 0, MEMCACHED_COMPRESSED);
    $ret = $memcached->get_by_key($master_key, $key);
    var_dump($ret);
}

$memcached = new Libmemcached();
$ret = $memcached->addserver('localhost', 11211);

$master_key = 'master_key1';
$key = 'key1';
$value_string = 'val1';
$value_long = 1234567890;
$value_array = array(1,'string', true, array('string'));

memcached_by_key($memcached, $master_key, $key, $value_string);
memcached_by_key($memcached, $master_key, $key, $value_long);
memcached_by_key($memcached, $master_key, $key, $value_array);

?>
--EXPECT--
bool(false)
string(4) "val1"
bool(false)
string(4) "val1"
bool(false)
string(10) "1234567890"
bool(false)
string(10) "1234567890"
bool(false)
array(4) {
  [0]=>
  int(1)
  [1]=>
  string(6) "string"
  [2]=>
  bool(true)
  [3]=>
  array(1) {
    [0]=>
    string(6) "string"
  }
}
bool(false)
array(4) {
  [0]=>
  int(1)
  [1]=>
  string(6) "string"
  [2]=>
  bool(true)
  [3]=>
  array(1) {
    [0]=>
    string(6) "string"
  }
}
