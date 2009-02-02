--TEST--
Check for libmemcached version
--SKIPIF--
<?php if (!extension_loaded("libmemcached")) print "skip"; ?>
--FILE--
<?php 
$a = new Libmemcached();
var_dump($a);
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
object(libmemcached)#1 (1) {
  ["memc"]=>
  resource(4) of type (memcached_st)
}
