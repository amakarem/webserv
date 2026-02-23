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
    font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Courier New", monospace;
    background: radial-gradient(circle at top, #07140b 0%, #050a06 55%, #020503 100%);
    color: #bbf7d0;
}

/* scanlines */
body::before {
    content: "";
    position: fixed;
    inset: 0;
    pointer-events: none;
    background: repeating-linear-gradient(
        to bottom,
        rgba(255,255,255,0.03),
        rgba(255,255,255,0.03) 1px,
        transparent 3px,
        transparent 6px
    );
    opacity: 0.25;
}

.container {
    max-width: 1100px;
    margin: 60px auto;
    padding: 28px;
    background: rgba(7, 20, 11, 0.55);
    border: 1px solid rgba(34,197,94,0.22);
    border-radius: 14px;
    box-shadow: 0 0 0 2px rgba(34,197,94,0.10), 0 25px 80px rgba(0,0,0,0.85);
}

h1 {
    text-align: center;
    color: #22c55e;
    margin: 0 0 22px 0;
    font-size: 22px;
    letter-spacing: 0.5px;
}

.subtitle {
    text-align: center;
    opacity: 0.7;
    margin-bottom: 22px;
}

.card {
    background: rgba(2,5,3,0.65);
    border: 1px solid rgba(34,197,94,0.2);
    padding: 18px;
    border-radius: 10px;
    margin-bottom: 16px;
}

.card h2 {
    margin: 0 0 12px 0;
    color: #86efac;
    font-size: 15px;
}

pre {
    background: #020503;
    padding: 14px;
    border-radius: 8px;
    overflow-x: auto;
    white-space: pre-wrap;
    word-break: break-word;
    color: #bbf7d0;
    border: 1px solid rgba(34,197,94,0.25);
    margin: 0;
    font-size: 13px;
    line-height: 1.35;
}

a {
    color: #86efac;
    text-decoration: none;
    font-weight: 700;
    transition: 0.2s;
}

a::before {
    content: "$ ";
    color: #22c55e;
}

a:hover {
    color: #ffffff;
    text-shadow: 0 0 12px rgba(34,197,94,0.6);
    transform: translateX(2px);
}

details {
    background: rgba(2,5,3,0.65);
    border: 1px dashed rgba(34,197,94,0.3);
    border-radius: 10px;
    padding: 14px 16px;
    margin-top: 16px;
}

summary {
    cursor: pointer;
    font-weight: 800;
    color: #22c55e;
    user-select: none;
}

summary:hover {
    color: #ffffff;
    text-shadow: 0 0 10px rgba(34,197,94,0.55);
}

.footer {
    margin-top: 18px;
    padding-top: 14px;
    border-top: 1px dashed rgba(34,197,94,0.3);
    text-align: center;
    opacity: 0.65;
    font-size: 12px;
}
</style>
</head>

<body>

<div class="container">

<h1>[webserv@42]$ php_diagnostics</h1>
<div class="subtitle">system status: <span style="color:#22c55e;">ONLINE</span> • mode: verbose</div>

<div class="card">
    <h2>⚙️ basic_info</h2>
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
    <h2>📦 PUT_raw_body</h2>
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
    <summary>🧬 show_phpinfo()</summary>
    <?php phpinfo(); ?>
</details>

<div class="card" style="margin-top:16px;">
    <a href="/">cd ..</a>
</div>

<div class="footer">
    42 Webserv • Terminal Mode • diagnostics complete
</div>

</div>

</body>
</html>
