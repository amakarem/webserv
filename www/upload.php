<?php

$target_dir = __DIR__ . "/tmp"; // folder to list files
if (!is_dir($target_dir)) {
    mkdir($target_dir, 0777, true);
}

// Handle delete request
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['delete_file'])) {
    $fileToDelete = basename($_POST['delete_file']); // sanitize
    $fullPath = $target_dir . "/" . $fileToDelete;
    if (is_file($fullPath)) {
        unlink($fullPath);
        $message = "Deleted file: $fileToDelete";
    } else {
        $message = "File not found: $fileToDelete";
    }
}

// Get list of files
$files = array_diff(scandir($target_dir), array('.', '..'));

$target_file = $target_dir . basename($_FILES["file"]["name"]);
$uploadOk = 1;
$imageFileType = strtolower(pathinfo($target_file,PATHINFO_EXTENSION));
// echo "<pre>";
// print_r($_POST);
// print_r($_FILES);
// echo "</pre>";
// Check if image file is a actual image or fake image
if(isset($_POST["submit"])) {
  $check = getimagesize($_FILES["file"]["tmp_name"]);
  if($check !== false) {
    echo "File is an image - " . $check["mime"] . ".";
    $uploadOk = 1;
  } else {
    echo "File is not an image.";
    $uploadOk = 0;
  }

  if (move_uploaded_file($_FILES["file"]["tmp_name"], $target_file)) {
    echo "The file ". htmlspecialchars( basename( $_FILES["file"]["name"])). " has been uploaded.";
  } else {
    echo "Sorry, there was an error uploading your file.";
  }
}
$folder = $target_dir;
?>

<!DOCTYPE html>
<html>
<head>
    <title>File List</title>
</head>
<body>
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
                <td><?= htmlspecialchars($file) ?></td>
                <td><?= filesize($fullPath) ?></td>
                <td>
                    <form method="POST" style="display:inline">
                        <input type="hidden" name="delete_file" value="<?= htmlspecialchars($file) ?>">
                        <button type="submit" onclick="return confirm('Delete <?= htmlspecialchars($file) ?>?')">DELETE</button>
                    </form>
                </td>
            </tr>
        <?php endif; ?>
    <?php endforeach; ?>
</table>
</body>
</html>