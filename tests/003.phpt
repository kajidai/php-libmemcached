--TEST--
Check for libmemcached version
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$memcached = new Memcached();
$ret = $memcached->behavior_get(MEMCACHED_BEHAVIOR_NO_BLOCK);
var_dump($ret);
$memcached->behavior_set(MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
$ret = $memcached->behavior_get(MEMCACHED_BEHAVIOR_NO_BLOCK);
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
float(0)
float(1)
