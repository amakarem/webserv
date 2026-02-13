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
<html>

<head>
    <title>File List</title>
</head>

<body>
    <h2><a href="upload.html">Upload New file</a></h2>
    <h2>Files in folder: <?= htmlspecialchars($folder) ?></h2>

    <?php if (isset($message)): ?>
        <p><?= htmlspecialchars($message) ?></p>
    <?php endif; ?>

    <table border="1" cellpadding="5" cellspacing="0">
        <tr>
            <th>File Name</th>
            <th>Size (bytes)</th>
            <th>Action</th>
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
                            <button type="submit" onclick="return confirm('Delete <?= htmlspecialchars($file) ?>?')">PHP DELETE</button>
                        </form>
                    </td>
                    <td>
                    <td>
                        <button onclick="deleteFile('uploads/<?= htmlspecialchars($file) ?>')">
                            DELETE REQUEST
                        </button>
                    </td>

                    </td>
                </tr>
            <?php endif; ?>
        <?php endforeach; ?>
    </table>
</body>

</html>

<script>
    function deleteFile(path) {
        if (!confirm('Delete ' + path + '?'))
            return;

        fetch('/' + path, {
                method: 'DELETE'
            })
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