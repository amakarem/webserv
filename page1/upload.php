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
    font-family: Arial, Helvetica, sans-serif;
    background: #0f172a;
    color: #e2e8f0;
}

.container {
    max-width: 900px;
    margin: 80px auto;
    padding: 40px;
    background: #1e293b;
    border-radius: 12px;
    box-shadow: 0 10px 40px rgba(0,0,0,0.5);
}

.header {
    text-align: center;
    margin-bottom: 28px;
}

.header h1 {
    margin: 0 0 8px 0;
    font-size: 32px;
    color: #38bdf8;
}

.header p {
    margin: 0;
    opacity: 0.7;
    font-size: 14px;
}

.card {
    background: #0b1220;
    border: 1px solid rgba(96,165,250,0.18);
    border-radius: 12px;
    padding: 18px;
    margin-bottom: 18px;
}

.card h2 {
    margin: 0 0 12px 0;
    font-size: 18px;
    color: #60a5fa;
}

.note {
    border-radius: 12px;
    padding: 12px 14px;
    border: 1px solid rgba(226,232,240,0.12);
    background: #0f172a;
    margin-top: 10px;
}

.note.ok {
    border-color: rgba(56,189,248,0.35);
    box-shadow: 0 0 0 3px rgba(56,189,248,0.08);
}

.note.err {
    border-color: rgba(248,113,113,0.35);
    box-shadow: 0 0 0 3px rgba(248,113,113,0.08);
}

.row {
    display: flex;
    gap: 12px;
    flex-wrap: wrap;
    align-items: center;
    margin-top: 12px;
}

.btn {
    appearance: none;
    border: none;
    border-radius: 10px;
    padding: 10px 16px;
    font-weight: 700;
    cursor: pointer;
    transition: 0.2s;
    background: #38bdf8;
    color: #0f172a;
    text-decoration: none;
    display: inline-block;
}

.btn:hover {
    background: #60a5fa;
    transform: translateY(-1px);
}

.btn-secondary {
    background: transparent;
    color: #60a5fa;
    border: 1px solid rgba(96,165,250,0.35);
}

.btn-secondary:hover {
    background: rgba(96,165,250,0.12);
    color: #ffffff;
    transform: translateY(-1px);
}

.btn-danger {
    background: transparent;
    color: #fca5a5;
    border: 1px solid rgba(248,113,113,0.35);
}

.btn-danger:hover {
    background: rgba(248,113,113,0.10);
    color: #ffffff;
    transform: translateY(-1px);
}

.table-wrap {
    overflow-x: auto;
    border-radius: 12px;
    border: 1px solid rgba(226,232,240,0.12);
}

table {
    width: 100%;
    border-collapse: collapse;
    background: #0f172a;
}

th, td {
    padding: 12px 10px;
    border-bottom: 1px solid rgba(226,232,240,0.10);
    text-align: left;
    font-size: 14px;
}

th {
    color: #60a5fa;
    font-size: 13px;
    letter-spacing: 0.3px;
    background: rgba(96,165,250,0.06);
}

td a {
    color: #60a5fa;
    text-decoration: none;
    font-weight: 600;
}

td a:hover {
    color: #ffffff;
}

.small {
    opacity: 0.7;
    font-size: 13px;
}

code {
    background: rgba(226,232,240,0.08);
    padding: 2px 6px;
    border-radius: 8px;
}

pre {
    margin: 0;
    padding: 12px;
    background: #0f172a;
    border: 1px solid rgba(226,232,240,0.12);
    border-radius: 12px;
    white-space: pre-wrap;
    word-wrap: break-word;
    max-height: 280px;
    overflow: auto;
}

.bottom-nav {
    margin-top: 26px;
    padding-top: 18px;
    border-top: 1px solid rgba(226,232,240,0.12);
    display: flex;
    justify-content: center;
    gap: 12px;
    flex-wrap: wrap;
}

.footer {
    text-align: center;
    margin-top: 18px;
    font-size: 13px;
    opacity: 0.6;
}
</style>
</head>

<body>
<div class="container">

    <div class="header">
        <h1>📂 Uploaded Files</h1>
        <p>Folder: <code><?= htmlspecialchars($folder) ?></code></p>
    </div>

    <div class="card">
        <h2>➕ Upload new file (PHP)</h2>
        <div class="row">
            <a class="btn btn-secondary" href="upload.html">⬅ Back to Upload Tester</a>
            <?php if ($debug): ?>
                <a class="btn btn-secondary" href="upload.php">Disable Debug</a>
            <?php else: ?>
                <a class="btn btn-secondary" href="upload.php?debug=true">Enable Debug</a>
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
        <h2>📄 File list</h2>
        <p class="small">Click a filename to download/open it. Use PHP DELETE or a real HTTP DELETE request.</p>

        <div class="table-wrap">
            <table>
                <tr>
                    <th>File</th>
                    <th>Size</th>
                    <th>PHP DELETE</th>
                    <th>DELETE REQUEST</th>
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
                                        PHP DELETE
                                    </button>
                                </form>
                            </td>
                            <td>
                                <button class="btn btn-danger" type="button"
                                    onclick="deleteFile('uploads/<?= htmlspecialchars($file) ?>')">
                                    DELETE REQUEST
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
            <h2>🧪 Debug</h2>
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
        <a class="btn btn-secondary" href="upload.html">⬅ Back to Upload Tester</a>
        <a class="btn" href="/">⬅ Back to Index</a>
    </div>

    <div class="footer">
        42 Webserv Project
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
