--TEST--
Check for memcached_append(), memcached_prepend() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 

function test_memcached_append($memcached, $key, $value, $append) {
    $memcached->delete($key);
    $ret = $memcached->get($key);
    var_dump($ret);
    $ret = $memcached->set($key, $value);
    var_dump($ret);
    $ret = $memcached->get($key);
    var_dump($ret);
    $ret = $memcached->append($key, $append);
    var_dump($ret);
    $ret = $memcached->get($key);
    var_dump($ret);
}

function test_memcached_prepend($memcached, $key, $value, $append) {
    $prepend = 'prepend';

    $memcached->delete($key);
    $ret = $memcached->get($key);
    var_dump($ret);
    $ret = $memcached->set($key, $value);
    var_dump($ret);
    $ret = $memcached->prepend($key, $prepend);
    var_dump($ret);
    $ret = $memcached->get($key);
    var_dump($ret);
    $ret = $memcached->get($key);
    var_dump($ret);
}

$memcached = new Memcached();
$ret = $memcached->addserver('localhost', 11211);

$master_key = 'master_key1';
$key = 'key1';
$value_string = 'val1';
$value_long = 1234567890;
$value_array = array(1,'string', true, array('string'));

$append = 'append';
$prepend = 'prepend';

test_memcached_append($memcached, $key, $value_string, $append);
test_memcached_append($memcached, $key, $value_long, $append);
test_memcached_append($memcached, $key, $value_array, $append);

test_memcached_prepend($memcached, $key, $value_string, $prepend);
test_memcached_prepend($memcached, $key, $value_long, $prepend);
test_memcached_prepend($memcached, $key, $value_array, $prepend);

?>
--EXPECT--

bool(false)
bool(true)
string(4) "val1"
bool(true)
string(10) "val1append"
bool(false)
bool(true)
string(10) "1234567890"
bool(true)
string(16) "1234567890append"
bool(false)
bool(true)
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
bool(true)
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
bool(true)
bool(true)
string(11) "prependval1"
string(11) "prependval1"
bool(false)
bool(true)
bool(true)
string(17) "prepend1234567890"
string(17) "prepend1234567890"
bool(false)
bool(true)
bool(true)
bool(false)
bool(false)
