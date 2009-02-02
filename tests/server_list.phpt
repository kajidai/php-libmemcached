--TEST--
Check for server_list() method
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Libmemcached();
$ret = $memcached->addserver('localhost', 11211);
$ret = $memcached->addserver('localhost', 11212);
$ret = $memcached->addserver('localhost', 11213);
$ret = $memcached->addserver('localhost', 11214);
$ret = $memcached->addserver('localhost', 11215);
var_dump($memcached->server_list());
?>
--EXPECT--
array(5) {
  [0]=>
  array(2) {
    ["hostname"]=>
    string(9) "localhost"
    ["port"]=>
    int(11211)
  }
  [1]=>
  array(2) {
    ["hostname"]=>
    string(9) "localhost"
    ["port"]=>
    int(11212)
  }
  [2]=>
  array(2) {
    ["hostname"]=>
    string(9) "localhost"
    ["port"]=>
    int(11213)
  }
  [3]=>
  array(2) {
    ["hostname"]=>
    string(9) "localhost"
    ["port"]=>
    int(11214)
  }
  [4]=>
  array(2) {
    ["hostname"]=>
    string(9) "localhost"
    ["port"]=>
    int(11215)
  }
}
