<?php
#function for streaming file to client
function streamFile($location, $filename, $mimeType='application/octet-stream')
{
  if(!file_exists($location)) {
    header ("HTTP/1.0 404 Not Found");
    return;
  }
  
  $size=filesize($location);
  $time=date('r',filemtime($location));
  #html response header
  header('Content-Description: File Transfer');	
  header("Content-Type: $mimeType"); 
  header('Cache-Control: public, must-revalidate, max-age=0');
  header('Pragma: no-cache');  
  header('Accept-Ranges: bytes');
  header('Content-Length:'.($size));
  header("Content-Disposition: inline; filename=$filename");
  header("Content-Transfer-Encoding: binary\n");
  header("Last-Modified: $time");
  header('Connection: close');      

  ob_clean();
  flush();
  readfile($location);
}

#**********************************************************
#Main script
#**********************************************************

#<1>set target path for storing photo uploads on the server
$photo_upload_path = "./upload/";
$photo_upload_path = $photo_upload_path. basename( $_FILES['uploaded_file']['name']); 

#<2>set target path for storing result on the server
$processed_photo_output_path = "./output/processed_";
#$processed_photo_output_path = $processed_photo_output_path. basename( $_FILES['uploadedFile']['name']); 
$processed_photo_output_path = $processed_photo_output_path. basename( pathinfo($_FILES['uploaded_file']['name'], PATHINFO_FILENAME)) . '.png';
$downloadFileName = 'processed_' . basename( pathinfo($_FILES['uploaded_file']['name'], PATHINFO_FILENAME)) . '.png';

#echo "ILYA DEBUG: " . $processed_photo_output_path . "<br>";
#echo "ILYA DEBUG: " . $downloadFileName . "<br>";

#<3>modify maximum allowable file size to 10MB and timeout to 300s
ini_set('upload_max_filesize', '10M');  
ini_set('post_max_size', '10M');  
ini_set('max_input_time', 300);  
ini_set('max_execution_time', 300);  

# Used for debug
#if ($_FILES["file"]["error"] > 0) {
#    echo "Error: " . $_FILES["uploadedFile"]["error"] . "<br>";
#} else {
#    echo "Upload: " . $_FILES["uploaded_file"]["name"] . "<br>";
#    echo "Type: " . $_FILES["uploaded_file"]["type"] . "<br>";
#    echo "Size: " . ($_FILES["uploaded_file"]["size"] / 1024) . " kB<br>";
#    echo "Stored in: " . $_FILES["uploaded_file"]["tmp_name"] . "<br>";
#}

#<4>Get and stored uploaded photos on the server
if (move_uploaded_file($_FILES['uploaded_file']['tmp_name'], $photo_upload_path)) {
    $command = "LD_LIBRARY_PATH=/usr/lib ./facecut -i $photo_upload_path -o $processed_photo_output_path -r &> /dev/null";
    system($command);

    #streamFile($processed_photo_output_path, $downloadFileName, "application/octet-stream");

    # Displaying the input / output image in HTML format
    $input_img_data = base64_encode(file_get_contents($photo_upload_path));
    $output_img_data = base64_encode(file_get_contents($processed_photo_output_path));

    $src = 'data: '.mime_content_type($photo_upload_path).';base64,'.$input_img_data;
    $dest = 'data: '.mime_content_type($processed_photo_output_path).';base64,'.$output_img_data;

    echo '<img src="',$src,'" align="top"><img src="',$dest,'">';
} else {
    echo "There was an error copying the file to $photo_upload_path !";
}

?>
