<?php
echo "hello world";
echo "<hr><h1>GET</h1>";
print_r($_GET);
echo "<hr><h1>POST</h1>";
echo "<pre>";
var_dump($_POST);
echo "</pre>";
echo "<hr><h1>PUT</h1>";
$raw = file_get_contents("php://input");
echo "raw"  . PHP_EOL . PHP_EOL . "<pre>";
var_dump($raw);
echo "</pre>";
echo "<hr>";
echo "<pre>";
echo PHP_EOL . '$_ENV' . PHP_EOL;
print_r($_ENV);
echo PHP_EOL . '$_SERVER' . PHP_EOL;
print_r($_SERVER);
echo "</pre>";

echo PHP_EOL . '$_FILES' . PHP_EOL;
print_r($_FILES);
echo "</pre>";
echo "upload_max_filesize: " . ini_get("upload_max_filesize") . "\n";
echo "post_max_size: " . ini_get("post_max_size") . "\n";

phpinfo();
