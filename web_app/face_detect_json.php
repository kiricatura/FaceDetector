<?php
#function for streaming file to client in JSON format
function streamFile($location, $filename, $mimeType='application/json')
{
  header("Content-Type: $mimeType"); 

  if(file_exists($location)) {
      $data     = file_get_contents($location);
      $bin_data = base64_encode($data);
      $arr      = array('status' => 'true', 'name' => $filename, 'mime' => 'image/png', 'data' => $bin_data);

  } else {
      $arr      = array('status' => 'false');
  }

  echo json_encode($arr);
}

#**********************************************************
#Main script
#**********************************************************

#<1>set target path for storing photo uploads on the server
$photo_upload_path = "./upload/";
$photo_upload_path = $photo_upload_path. basename( $_FILES['uploaded_file']['name']); 

#<2>set target path for storing result on the server
$processed_photo_output_path = "./output/processed_";
$processed_photo_output_path = $processed_photo_output_path. basename( pathinfo($_FILES['uploaded_file']['name'], PATHINFO_FILENAME)) . '.png';
$downloadFileName = 'processed_' . basename( pathinfo($_FILES['uploaded_file']['name'], PATHINFO_FILENAME)) . '.png';

#<3>modify maximum allowable file size to 10MB and timeout to 300s
ini_set('upload_max_filesize', '10M');  
ini_set('post_max_size', '10M');  
ini_set('max_input_time', 300);  
ini_set('max_execution_time', 300);  

#<4>Get and stored uploaded photos on the server
if (move_uploaded_file($_FILES['uploaded_file']['tmp_name'], $photo_upload_path)) {
    $command = "LD_LIBRARY_PATH=/usr/lib ./facecut -i $photo_upload_path -o $processed_photo_output_path -r &> /dev/null";
    exec($command);

    streamFile($processed_photo_output_path, $downloadFileName, "application/json");
} else {
    echo "There was an error copying the file to $photo_upload_path !";
}

?>
