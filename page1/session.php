<?php

session_start();

if (!isset($_SESSION['count'])) {
    $_SESSION['count'] = 1;
} else {
    $_SESSION['count']++;
}

echo "<!DOCTYPE html>";
echo "<html>";
echo "<head><title>PHP Session Test</title></head>";
echo "<body>";
echo "<h1>PHP Session Working ✅</h1>";
echo "<p>Session ID: " . session_id() . "</p>";
echo "<p>Visit count: " . $_SESSION['count'] . "</p>";
echo "<pre>";
print_r($_SESSION);
print_r($_COOKIE);
echo "</pre>";
echo "</body>";
echo "</html>";