--TEST--
Check for libmemcached version
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Memcached();
$ret = $memcached->addserver('localhost', 11211);
$ret = $memcached->set('key1', 'val1');
var_dump($ret);
$ret = $memcached->get('key1');
var_dump($ret);
$ret = $memcached->set('key_array1', array('val1'));
var_dump($ret);
$ret = $memcached->get('key_array1');
var_dump($ret);

/*
	you can add regression tests for your extension here

  the output of your test code has to be equal to the
  text in the --EXPECT-- section below for the tests
  to pass, differences between the output and the
  expected text are interpreted as failure

	see php5/README.TESTING for further information on
  writing regression tests
*/
?>
--EXPECT--
bool(true)
string(4) "val1"
bool(true)
array(1) {
  [0]=>
  string(4) "val1"
}
