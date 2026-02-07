<?php
echo "hello world";
echo "<hr><h1>GET</h1>";
print_r($_GET);
echo "<hr><h1>POST</h1>";
print_r($_POST);
echo "<hr><h1>PUT</h1>";
$handle  = fopen('php://input', 'r');
print_r($handle);

// phpinfo();