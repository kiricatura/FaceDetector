<html>
<body>

<form enctype="multipart/form-data" action="face_detect_json.php" method="POST">
<input type="hidden" name="MAX_FILE_SIZE" value="10000000" />
Choose a file to upload: <input name="uploaded_file" type="file" id="uploaded_file"/><br />
<input type="submit" value="Upload File" />
</form>

<?php

echo "<br><br>FILES UPLOADED:<br>";
echo "-------------------------<br>";
system("ls -sltrh ./upload | rev | cut -d\" \" -f1-6 | rev | sed 's/$/<br>/g'");

echo "<br><br>FILES PROCESSED:<br>";
echo "--------------------------<br>";
system("ls -sltrh ./output | rev | cut -d\" \" -f1-6 | rev | sed 's/$/<br>/g'");

?>

</body>
</html>
