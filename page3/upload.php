<?php
$folder = __DIR__ . "/uploads";
if (!is_dir($folder)) {
    mkdir($folder, 0777, true);
}

$upload_msg = null;
$upload_ok = null;
$message = null;

$debug = isset($_GET["debug"]);

// Handle upload
if (isset($_POST["submit"])) {
    if (isset($_FILES["file"]) && isset($_FILES["file"]["tmp_name"]) && $_FILES["file"]["tmp_name"] !== "") {

        $target_file = $folder . "/" . basename($_FILES["file"]["name"]);

        // Check if it's an image (optional info)
        $check = @getimagesize($_FILES["file"]["tmp_name"]);
        if ($check !== false) {
            $upload_msg = "File looks like an image (" . $check["mime"] . ").";
        } else {
            $upload_msg = "File uploaded (not detected as an image).";
        }

        if (move_uploaded_file($_FILES["file"]["tmp_name"], $target_file)) {
            $upload_ok = true;
            $upload_msg .= " Saved as: " . htmlspecialchars(basename($_FILES["file"]["name"]));
        } else {
            $upload_ok = false;
            $upload_msg = "Sorry, there was an error uploading your file.";
        }
    } else {
        $upload_ok = false;
        $upload_msg = "No file selected.";
    }
}

// Handle PHP delete
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['delete_file'])) {
    $fileToDelete = basename($_POST['delete_file']); // sanitize
    $fullPath = $folder . "/" . $fileToDelete;

    if (is_file($fullPath)) {
        unlink($fullPath);
        $message = "Deleted file: $fileToDelete";
    } else {
        $message = "File not found: $fileToDelete";
    }
}

// Get list of files
$files = array_diff(scandir($folder), array('.', '..'));
?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Uploads | Webserv</title>

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
    max-width: 980px;
    margin: 60px auto;
    padding: 28px;
    background: rgba(7, 20, 11, 0.55);
    border: 1px solid rgba(34,197,94,0.22);
    border-radius: 14px;
    box-shadow: 0 0 0 2px rgba(34,197,94,0.10), 0 25px 80px rgba(0,0,0,0.85);
}

.header {
    text-align: left;
    margin-bottom: 16px;
}

.header h1 {
    margin: 0;
    font-size: 20px;
    color: #22c55e;
}

.header p {
    margin: 8px 0 0 0;
    opacity: 0.75;
    font-size: 13px;
}

code {
    background: rgba(34,197,94,0.10);
    border: 1px solid rgba(34,197,94,0.18);
    padding: 2px 6px;
    border-radius: 8px;
    color: #86efac;
}

.card {
    background: rgba(2,5,3,0.65);
    border: 1px solid rgba(34,197,94,0.2);
    border-radius: 10px;
    padding: 18px;
    margin-bottom: 16px;
}

.card h2 {
    margin: 0 0 10px 0;
    font-size: 15px;
    color: #86efac;
}

.small {
    opacity: 0.7;
    font-size: 13px;
    margin: 0 0 12px 0;
}

.row {
    display: flex;
    gap: 10px;
    flex-wrap: wrap;
    margin-top: 10px;
}

.btn {
    appearance: none;
    border-radius: 6px;
    padding: 8px 14px;
    font-weight: 800;
    cursor: pointer;
    transition: 0.2s;
    background: rgba(34,197,94,0.15);
    border: 1px solid rgba(34,197,94,0.35);
    color: #86efac;
    text-decoration: none;
    display: inline-block;
}

.btn:hover {
    background: rgba(34,197,94,0.35);
    color: #ffffff;
    box-shadow: 0 0 12px rgba(34,197,94,0.6);
    transform: translateY(-1px);
}

.btn-secondary {
    background: transparent;
    border: 1px dashed rgba(34,197,94,0.40);
}

.btn-danger {
    background: rgba(248,113,113,0.06);
    border: 1px solid rgba(248,113,113,0.35);
    color: #fecaca;
}

.btn-danger:hover {
    background: rgba(248,113,113,0.18);
    box-shadow: 0 0 14px rgba(248,113,113,0.25);
}

.note {
    border-radius: 10px;
    padding: 12px 14px;
    background: #020503;
    border: 1px solid rgba(34,197,94,0.25);
    margin-top: 10px;
    font-size: 13px;
}

.note.ok {
    border-color: rgba(34,197,94,0.35);
}

.note.err {
    border-color: rgba(248,113,113,0.35);
    color: #fecaca;
}

.table-wrap {
    overflow-x: auto;
    border-radius: 10px;
    border: 1px solid rgba(34,197,94,0.25);
    background: #020503;
}

table {
    width: 100%;
    border-collapse: collapse;
    min-width: 720px;
}

th, td {
    padding: 12px 10px;
    border-bottom: 1px solid rgba(34,197,94,0.12);
    text-align: left;
    font-size: 13px;
}

th {
    color: #22c55e;
    background: rgba(34,197,94,0.08);
    letter-spacing: 0.2px;
}

td a {
    color: #86efac;
    text-decoration: none;
    font-weight: 800;
}

td a:hover {
    color: #ffffff;
    text-shadow: 0 0 10px rgba(34,197,94,0.6);
}

pre {
    margin: 0;
    padding: 12px;
    background: #020503;
    border: 1px solid rgba(34,197,94,0.25);
    border-radius: 10px;
    white-space: pre-wrap;
    word-wrap: break-word;
    max-height: 280px;
    overflow: auto;
    font-size: 12px;
}

.bottom-nav {
    margin-top: 18px;
    padding-top: 14px;
    border-top: 1px dashed rgba(34,197,94,0.3);
    display: flex;
    justify-content: center;
    gap: 12px;
    flex-wrap: wrap;
}

.footer {
    text-align: center;
    margin-top: 14px;
    opacity: 0.65;
    font-size: 12px;
}
</style>
</head>

<body>
<div class="container">

    <div class="header">
        <h1>[webserv@42]$ file_system_monitor</h1>
        <p>mount: <code><?= htmlspecialchars($folder) ?></code></p>
    </div>

    <div class="card">
        <h2>$ upload_new_file (php)</h2>
        <div class="row">
            <a class="btn btn-secondary" href="upload.html">cd upload_console</a>
            <?php if ($debug): ?>
                <a class="btn btn-secondary" href="upload.php">debug: OFF</a>
            <?php else: ?>
                <a class="btn btn-secondary" href="upload.php?debug=true">debug: ON</a>
            <?php endif; ?>
        </div>

        <?php if ($upload_msg !== null): ?>
            <div class="note <?= $upload_ok ? "ok" : "err" ?>">
                <?= htmlspecialchars($upload_msg) ?>
            </div>
        <?php endif; ?>

        <?php if (isset($message)): ?>
            <div class="note ok" style="margin-top:12px;">
                <?= htmlspecialchars($message) ?>
            </div>
        <?php endif; ?>
    </div>

    <div class="card">
        <h2>$ ls -la uploads/</h2>
        <p class="small">tip: click filename to open • use php_delete or http_delete</p>

        <div class="table-wrap">
            <table>
                <tr>
                    <th>file</th>
                    <th>size</th>
                    <th>php_delete</th>
                    <th>http_delete</th>
                </tr>

                <?php foreach ($files as $file): ?>
                    <?php $fullPath = $folder . "/" . $file; ?>
                    <?php if (is_file($fullPath)): ?>
                        <tr>
                            <td>
                                <a href="uploads/<?= htmlspecialchars($file) ?>"><?= htmlspecialchars($file) ?></a>
                            </td>
                            <td><?= filesize($fullPath) ?> bytes</td>
                            <td>
                                <form method="POST" style="margin:0;">
                                    <input type="hidden" name="delete_file" value="<?= htmlspecialchars($file) ?>">
                                    <button class="btn btn-danger" type="submit"
                                        onclick="return confirm('Delete <?= htmlspecialchars($file) ?>?')">
                                        rm (php)
                                    </button>
                                </form>
                            </td>
                            <td>
                                <button class="btn btn-danger" type="button"
                                    onclick="deleteFile('uploads/<?= htmlspecialchars($file) ?>')">
                                    rm (http)
                                </button>
                            </td>
                        </tr>
                    <?php endif; ?>
                <?php endforeach; ?>
            </table>
        </div>
    </div>

    <?php if ($debug): ?>
        <div class="card">
            <h2>$ debug_dump</h2>
            <div class="note">
                <pre><?php
print_r($_POST);
print_r($_FILES);
print_r($_SERVER);
                ?></pre>
            </div>
        </div>
    <?php endif; ?>

    <div class="bottom-nav">
        <a class="btn btn-secondary" href="upload.html">cd upload_console</a>
        <a class="btn" href="/">cd ..</a>
    </div>

    <div class="footer">
        42 Webserv • Terminal Mode • monitor complete
    </div>

</div>

<script>
function deleteFile(path) {
    if (!confirm('Delete ' + path + '?')) return;

    fetch('/' + path, { method: 'DELETE' })
        .then(response => response.text())
        .then(data => {
            alert(data);
            location.reload();
        })
        .catch(err => {
            alert('Error: ' + err);
        });
}
</script>
</body>
</html>
