<?php
function h($s) {
    return htmlspecialchars((string)$s, ENT_QUOTES, 'UTF-8');
}

$raw = file_get_contents("php://input");
?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>PHP Debug Info</title>

<style>
body {
    margin: 0;
    font-family: Arial, Helvetica, sans-serif;
    background: #0f172a;
    color: #f1f5f9;
}

.container {
    max-width: 1100px;
    margin: 60px auto;
    padding: 20px;
}

h1 {
    text-align: center;
    color: #38bdf8;
    margin-bottom: 30px;
}

.card {
    background: #1e293b;
    padding: 20px;
    border-radius: 12px;
    margin-bottom: 20px;
    box-shadow: 0 10px 30px rgba(0,0,0,0.4);
}

.card h2 {
    margin-top: 0;
    color: #7dd3fc;
}

pre {
    background: #0b1220;
    padding: 15px;
    border-radius: 8px;
    overflow-x: auto;
    white-space: pre-wrap;
    word-break: break-word;
    color: #e2e8f0;
}

a {
    color: #7dd3fc;
    text-decoration: none;
    font-weight: 600;
}

a:hover {
    color: white;
}

details {
    background: #1e293b;
    border-radius: 12px;
    padding: 10px 20px;
}

summary {
    cursor: pointer;
    font-weight: bold;
    color: #7dd3fc;
}
</style>
</head>

<body>

<div class="container">

<h1>🐘 PHP Debug Panel</h1>

<div class="card">
    <h2>Basic Info</h2>
    <pre>
Request Method: <?= h($_SERVER['REQUEST_METHOD'] ?? '') ?>

Request URI: <?= h($_SERVER['REQUEST_URI'] ?? '') ?>

upload_max_filesize: <?= h(ini_get("upload_max_filesize")) ?>

post_max_size: <?= h(ini_get("post_max_size")) ?>
    </pre>
</div>

<div class="card">
    <h2>GET</h2>
    <pre><?php print_r($_GET); ?></pre>
</div>

<div class="card">
    <h2>POST</h2>
    <pre><?php print_r($_POST); ?></pre>
</div>

<div class="card">
    <h2>PUT / Raw Body</h2>
    <pre><?= h($raw); ?></pre>
</div>

<div class="card">
    <h2>FILES</h2>
    <pre><?php print_r($_FILES); ?></pre>
</div>

<div class="card">
    <h2>ENV</h2>
    <pre><?php print_r($_ENV); ?></pre>
</div>

<div class="card">
    <h2>SERVER</h2>
    <pre><?php print_r($_SERVER); ?></pre>
</div>

<details>
    <summary>Show phpinfo()</summary>
    <?php phpinfo(); ?>
</details>

<div class="card">
    <a href="/">⬅ Back to Index</a>
</div>

</div>

</body>
</html>
