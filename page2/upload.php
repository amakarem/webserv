<?php
$folder = __DIR__ . "/uploads"; // folder to list files
if (!is_dir($folder)) {
    mkdir($folder, 0777, true);
}
if (isset($_GET["debug"])) {
    echo "<pre>";
    print_r($_POST);
    print_r($_FILES);
    print_r($_SERVER);
    echo "</pre>";
}
// Check if image file is a actual image or fake image
if (isset($_POST["submit"])) {
    $target_file = $folder . "/" . basename($_FILES["file"]["name"]);
    $imageFileType = strtolower(pathinfo($target_file, PATHINFO_EXTENSION));
    $check = getimagesize($_FILES["file"]["tmp_name"]);
    if ($check !== false) {
        echo "File is an image - " . $check["mime"] . ".<br>";
    } else {
        echo "File is not an image.<br>";
    }
    if (move_uploaded_file($_FILES["file"]["tmp_name"], $target_file)) {
        echo "The file " . htmlspecialchars(basename($_FILES["file"]["name"])) . " has been uploaded.";
        echo '<a href=upload.php>click here to refresh the list of uploaded files</a>';
    } else {
        echo "Sorry, there was an error uploading your file.";
    }
}

// Handle delete request
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['delete_file'])) {
    $fileToDelete = basename($_POST['delete_file']); // sanitize
    echo $fileToDelete;
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
    <title>File List</title>

    <style>
    body {
        margin: 0;
        font-family: "Poppins", Arial, sans-serif;
        background: linear-gradient(135deg, #1e1b4b, #0f0f1a);
        color: #f3f4f6;
    }

    .container {
        max-width: 950px;
        margin: 80px auto;
        padding: 50px;
        background: rgba(255, 255, 255, 0.05);
        backdrop-filter: blur(14px);
        border-radius: 20px;
        border: 1px solid rgba(255,255,255,0.1);
        box-shadow: 0 20px 60px rgba(0,0,0,0.6);
    }

    h1 {
        margin: 0 0 10px 0;
        font-size: 34px;
        color: #c084fc;
        letter-spacing: 1px;
        text-align: center;
    }

    .subtitle {
        text-align: center;
        opacity: 0.7;
        margin-bottom: 25px;
    }

    .top-actions {
        display: flex;
        justify-content: center;
        gap: 12px;
        flex-wrap: wrap;
        margin-bottom: 20px;
    }

    a {
        color: #e9d5ff;
        text-decoration: none;
        font-weight: 600;
    }

    .btn-link {
        display: inline-block;
        padding: 10px 18px;
        border-radius: 12px;
        background: rgba(192,132,252,0.10);
        border: 1px solid rgba(192,132,252,0.30);
        transition: 0.25s ease;
    }

    .btn-link:hover {
        background: rgba(192,132,252,0.25);
        color: #ffffff;
        transform: translateY(-2px);
        box-shadow: 0 8px 25px rgba(192,132,252,0.4);
    }

    .note {
        margin: 18px auto 22px auto;
        max-width: 760px;
        padding: 12px 14px;
        border-radius: 12px;
        background: rgba(255,255,255,0.05);
        border: 1px solid rgba(255,255,255,0.10);
        text-align: center;
    }

    hr {
        border: none;
        height: 1px;
        background: rgba(255,255,255,0.1);
        margin: 22px 0;
    }

    .table-wrap {
        overflow-x: auto;
        border-radius: 14px;
        border: 1px solid rgba(255,255,255,0.10);
        background: rgba(0,0,0,0.15);
    }

    table {
        width: 100%;
        border-collapse: collapse;
        min-width: 700px;
    }

    th, td {
        padding: 12px 10px;
        border-bottom: 1px solid rgba(255,255,255,0.07);
        text-align: left;
        font-size: 14px;
    }

    th {
        color: #c084fc;
        background: rgba(192,132,252,0.08);
        font-size: 13px;
        letter-spacing: 0.3px;
    }

    td a:hover {
        color: #ffffff;
    }

    button {
        padding: 9px 14px;
        border-radius: 12px;
        border: 1px solid rgba(192,132,252,0.30);
        background: rgba(192,132,252,0.10);
        color: #e9d5ff;
        font-weight: 600;
        cursor: pointer;
        transition: 0.25s ease;
    }

    button:hover {
        background: rgba(192,132,252,0.25);
        transform: translateY(-2px);
        box-shadow: 0 8px 25px rgba(192,132,252,0.4);
        color: #ffffff;
    }

    .danger {
        border-color: rgba(248,113,113,0.35);
        background: rgba(248,113,113,0.08);
        color: #fecaca;
    }

    .danger:hover {
        background: rgba(248,113,113,0.18);
        box-shadow: 0 8px 25px rgba(248,113,113,0.22);
    }

    .footer {
        text-align: center;
        margin-top: 28px;
        opacity: 0.6;
        font-size: 13px;
    }

    /* make existing debug pre look nicer without changing logic */
    pre {
        padding: 14px;
        border-radius: 14px;
        background: rgba(0,0,0,0.25);
        border: 1px solid rgba(255,255,255,0.10);
        overflow: auto;
    }
    </style>
</head>

<body>
<div class="container">

    <h1>📂 Uploaded Files</h1>
    <div class="subtitle">Files in folder: <?= htmlspecialchars($folder) ?></div>

    <div class="top-actions">
        <a class="btn-link" href="upload.html">⬅ Upload Page</a>
        <a class="btn-link" href="/">⬅ Back to Index</a>
    </div>

    <?php if (isset($message)): ?>
        <div class="note">✅ <?= htmlspecialchars($message) ?></div>
    <?php endif; ?>

    <div class="table-wrap">
        <table>
            <tr>
                <th>📄 File Name</th>
                <th>📦 Size (bytes)</th>
                <th>🗑 PHP DELETE</th>
                <th>🗑 DELETE REQUEST</th>
            </tr>

            <?php foreach ($files as $file): ?>
                <?php $fullPath = $folder . "/" . $file; ?>
                <?php if (is_file($fullPath)): ?>
                    <tr>
                        <td><a href="uploads/<?= htmlspecialchars($file) ?>"><?= htmlspecialchars($file) ?></a></td>
                        <td><?= filesize($fullPath) ?></td>

                        <td>
                            <form method="POST" style="display:inline">
                                <input type="hidden" name="delete_file" value="<?= htmlspecialchars($file) ?>">
                                <button class="danger" type="submit" onclick="return confirm('Delete <?= htmlspecialchars($file) ?>?')">PHP DELETE</button>
                            </form>
                        </td>

                        <td>
                            <button class="danger" onclick="deleteFile('uploads/<?= htmlspecialchars($file) ?>')">DELETE REQUEST</button>
                        </td>
                    </tr>
                <?php endif; ?>
            <?php endforeach; ?>
        </table>
    </div>

    <div class="footer">
        42 Webserv Project • Purple Theme
    </div>

</div>

<script>
function deleteFile(path) {
    if (!confirm('Delete ' + path + '?'))
        return;

    fetch('/' + path, { method: 'DELETE' })
        .then(response => response.text())
        .then(data => {
            alert(data);
            location.reload(); // refresh after delete
        })
        .catch(err => {
            alert('Error: ' + err);
        });
}
</script>

</body>
</html>
