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
    font-family: "Poppins", Arial, sans-serif;
    background: linear-gradient(135deg, #1e1b4b, #0f0f1a);
    color: #ffffff;
}

.container {
    max-width: 1100px;
    margin: 60px auto;
    padding: 40px;
}

h1 {
    text-align: center;
    color: #d8b4fe;
    margin-bottom: 40px;
    font-size: 36px;
    text-shadow: 0 0 20px rgba(192,132,252,0.4);
}

.card {
    background: rgba(255, 255, 255, 0.06);
    backdrop-filter: blur(18px);
    padding: 28px;
    border-radius: 20px;
    margin-bottom: 30px;
    border: 1px solid rgba(255,255,255,0.15);
    box-shadow: 0 20px 60px rgba(0,0,0,0.65);
}

.card h2 {
    margin-top: 0;
    color: #ffffff;
    font-size: 20px;
    margin-bottom: 18px;
}

pre {
    background: rgba(0,0,0,0.65);
    padding: 20px;
    border-radius: 16px;
    overflow-x: auto;
    white-space: pre-wrap;
    word-break: break-word;
    border: 1px solid rgba(255,255,255,0.18);
    font-size: 14px;
    line-height: 1.6;
    color: #ffffff;
    font-weight: 500;
}

a {
    display: inline-block;
    padding: 12px 20px;
    border-radius: 14px;
    background: rgba(192,132,252,0.15);
    border: 1px solid rgba(192,132,252,0.35);
    color: #ffffff;
    font-weight: 700;
    text-decoration: none;
    transition: 0.25s ease;
}

a:hover {
    background: rgba(192,132,252,0.35);
    transform: translateY(-3px);
    box-shadow: 0 10px 30px rgba(192,132,252,0.5);
}

details {
    background: rgba(255,255,255,0.06);
    border-radius: 20px;
    padding: 25px;
    border: 1px solid rgba(255,255,255,0.15);
    box-shadow: 0 20px 60px rgba(0,0,0,0.65);
    margin-bottom: 30px;
}

summary {
    cursor: pointer;
    font-weight: bold;
    font-size: 18px;
    color: #d8b4fe;
    margin-bottom: 15px;
}

summary:hover {
    color: #ffffff;
}

.footer {
    text-align: center;
    margin-top: 40px;
    opacity: 0.75;
    font-size: 14px;
}
</style>
</head>

<body>

<div class="container">

<h1>🟣 PHP Debug Console</h1>

<div class="card">
    <h2>⚙️ Basic Info</h2>
    <pre>
Request Method: <?= h($_SERVER['REQUEST_METHOD'] ?? '') ?>

Request URI: <?= h($_SERVER['REQUEST_URI'] ?? '') ?>

upload_max_filesize: <?= h(ini_get("upload_max_filesize")) ?>

post_max_size: <?= h(ini_get("post_max_size")) ?>
    </pre>
</div>

<div class="card">
    <h2>📥 GET</h2>
    <pre><?php print_r($_GET); ?></pre>
</div>

<div class="card">
    <h2>📤 POST</h2>
    <pre><?php print_r($_POST); ?></pre>
</div>

<div class="card">
    <h2>📦 PUT / Raw Body</h2>
    <pre><?= h($raw); ?></pre>
</div>

<div class="card">
    <h2>📁 FILES</h2>
    <pre><?php print_r($_FILES); ?></pre>
</div>

<div class="card">
    <h2>🌍 ENV</h2>
    <pre><?php print_r($_ENV); ?></pre>
</div>

<div class="card">
    <h2>🧪 SERVER</h2>
    <pre><?php print_r($_SERVER); ?></pre>
</div>

<details>
    <summary>🐘 Show phpinfo()</summary>
    <?php phpinfo(); ?>
</details>

<div class="card" style="text-align:center;">
    <a href="/">⬅ Back to Index</a>
</div>

<div class="footer">
    42 Webserv Project • Purple Glass High Contrast Mode
</div>

</div>

</body>
</html>