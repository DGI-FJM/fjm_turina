<?php 

function _fjm_turina_produce_tiff_derivatives($filename) {
  $additional_datastreams = array();

  //Produce a "FULL_JPG"
  $additional_datastreams += _fjm_turina_derive_image($filename, 'FULL_JPG', 'Derived full-sized JPEG');

  //Produce a medium-sized web version
  $additional_datastreams += _fjm_turina_derive_image($filename, 'WEB_JPG', 'Derived web-sized JPEG', array(1024, 1024));

  //Produce a thumbnail.
  $additional_datastreams += _fjm_turina_derive_image($filename, 'TN', 'Derived thumbnail-sized JPEG', array(200, 200));

  return $additional_datastreams;
}
function _fjm_turina_produce_jpeg_derivatives($filename) {
  $additional_datastreams = array();

  //Create medium-sized (1280x1280, 1024x1024?) derivative for web use.
  $additional_datastreams += _fjm_turina_derive_image($filename, 'WEB_JPG', 'Derived web-sized JPEG', array(1024, 1024));

  //Create thumbnail.
  $additional_datastreams += _fjm_turina_derive_image($filename, 'TN', 'Derived thumbnail-sized JPEG', array(200, 200));

  return $additional_datastreams;
}
function _fjm_turina_derive_image($filename, $dsid, $label = 'Derived image', $dimensions = NULL) {
  $image = imageapi_image_open($filename, 'imageapi_imagemagick');

  if ($dimensions !== NULL) {
    list($x, $y) = $dimensions;
    imageapi_image_scale($image, $x, $y, FALSE);
  }

  $jpeg_file = file_create_filename(basename($filename) . ".$dsid.jpg", file_directory_path());
  if (imageapi_image_close($image, $jpeg_file)) {
    return array(
      $dsid => array(
        'mimetype' => 'image/jpeg',
        'label' => $label,
        'filepath' => $jpeg_file,
        'fjm_turina_derived_file' => TRUE,
      ),
    );
  }
  else {
    drush_print('Error producing web derivative.');
    return array();
  }
}
function _fjm_turina_add_derivatives(&$object) {
  //XXX: Was escaping accented characters...
  setlocale(LC_CTYPE, "en_US.UTF-8");
  
  $new_datastreams = array();
  foreach ((array)$object['datastreams'] as $dsid => $datastream) {
    if ($dsid == 'FULL_JPG') {
      $new_datastreams += _fjm_turina_produce_jpeg_derivatives($datastream['filepath']);
    }
    elseif ($dsid == 'TIFF') {
      $new_datastreams += _fjm_turina_produce_tiff_derivatives($datastream['filepath']);
    }
  }
  
  if (array_key_exists('datastreams', $object)) {
    $object['datastreams'] += $new_datastreams;
  }
  else {
    drush_print_r($object);
    $object['datastreams'] = $new_datastreams;
  }

  foreach ($object['children'] as &$child) {
    _fjm_turina_add_derivatives($child);
  }
}

function _fjm_turina_delete_derived_files(&$object) {
  foreach ($object['datastreams'] as $dsid => $datastream) {
    if (isset($datastream['fjm_turina_derived']) && $datastream['fjm_turina_derived']) {
      file_delete($datastream['filename']);
    }
  }

  foreach ($object['children'] as &$child) {
    _fjm_turina_delete_derived_files($child);
  }
}

function _fjm_turina_datastream_from_file($filename) {
  $period_pos = strrpos($filename, '.');
  $file_ext = substr($filename, $period_pos + 1);
  $dsid = $base_dsid = strtoupper($file_ext);
  $mimetype = mime_content_type($filename);

  if ($mimetype == 'image/jpeg') {
    $dsid = 'FULL_JPG';
  }
  elseif ($mimetype == 'image/tiff') {
    $dsid = 'TIFF';
  }

  return array(
    $dsid => array(
      'label' => "$base_dsid datastream",
      'mimetype' => $mimetype,
      'filepath' => $filename,
    ),
  );
}
