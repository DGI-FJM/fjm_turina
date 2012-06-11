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
        'fjm_turina_derived' => TRUE,
      ),
    );
  }
  else {
    watchdog('fjm_turina-ingest', 'Error producing derivative for %filename ', array(
      '%filename' => $filename,), WATCHDOG_WARNING);
    return array();
  }
}

function _fjm_turina_add_derivatives(&$object) {
  //XXX: Was escaping accented characters...
  setlocale(LC_CTYPE, "en_US.UTF-8");
  
  $new_datastreams = array();
  foreach ($object['datastreams'] as $dsid => $datastream) {
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
    $object['datastreams'] = $new_datastreams;
  }
}

function _fjm_turina_delete_derived_files(&$object) {
  foreach ($object['datastreams'] as $dsid => $datastream) {
    if (isset($datastream['fjm_turina_derived']) && $datastream['fjm_turina_derived']) {
      file_delete($datastream['filepath']);
    }
  }
}

function _fjm_turina_datastream_from_file($filename) {
  $period_pos = strrpos($filename, '.');
  $file_ext = substr($filename, $period_pos + 1);
  $dsid = $base_dsid = strtoupper($file_ext);
  $mimetype = _fjm_turina_get_mimetype($filename);

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
      'control_group' => _fjm_turina_get_control_group_from_mime($mimetype),
    ),
  );
}

function _fjm_turina_get_mimetype($filename) {
  $mt = NULL;
  if (class_exists('finfo')) {
    $finfo = new finfo();
    $mt = $finfo->file($filename);
  }
  
  if ((!$mt || $mt == 'application/octet-stream') && function_exists('mime_content_type')) {
    $mt = mime_content_type($filename);
  }
  
  if (!$mt || $mt == 'application/octet-stream') {
    module_load_include('inc', 'fedora_repository', 'MimeClass');
    $mc = new MimeClass();
    $mt = $mc->get_mimetype($filename);
  }
  
  return $mt;
}

/**
 *
 * @return array
 */
function _fjm_turina_get_xml_files() {
  $files = file_directory_path() . '/data';
  $xml_files = file_scan_directory($files, '\.[xX][mM][lL]');
  return $xml_files;
}

function _fjm_turina_assemble_info($serial_prefix = 'serialized_book_', $number = -1) {
  module_load_include('inc', 'fjm_turina', 'mets_ingest');
  module_load_include('inc', 'fjm_turina', 'mods_ingest');
  
  $relations = array(
    array('isMemberOfCollection', 'jt:bookCollection'),
    array('hasModel', 'jt:bookCModel', 'info:fedora/fedora-system:def/model#'),
  );

  $info_dir = file_create_filename('ingest_dir', file_directory_path());
  $count = 0;
  mkdir($info_dir);
  
  foreach (_fjm_turina_get_xml_files() as $filename => $file) {
    if (($number > 0) && ($count >= $number)) {
      break;
    }
    
    $xml_xpath = new DOMXPath(DOMDocument::load("./$filename"));
    $xml_xpath->registerNamespace('mets', 'http://www.loc.gov/METS/');
    $xml_xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
  
    $new_objects = array();
    $new_objects = array_merge($new_objects, _fjm_turina_get_objects_from_mets_file($filename, $xml_xpath, $relations));
  
    if (count($new_objects) === 0) {
      $new_objects = array_merge($new_objects, _fjm_turina_get_objects_from_mods_file($filename, $xml_xpath, $relations));
    }
  
    //Serialize out to a file in $info_dir
    while (($object = array_shift($new_objects)) && (($number < 0) || (($number > 0) && ($count < $number)))) {
      $serial_file = file_create_filename($serial_prefix . $count++, $info_dir);
      file_put_contents($serial_file, json_encode($object));
    }
  }
  
  return $info_dir;
}

/**
 * Determines a control group, given a mimetype.
 *
 * @param $mime string
 *   A datastream's mimetype.
 * @return string
 *   - 'X' if $mime is one of:
 *     - 'text/xml'
 *     - 'application/xml'
 *     - 'application/rdf+xml'
 *   - 'M' otherwise.
 *
 */
function _fjm_turina_get_control_group_from_mime($mime) {
  $inline_xml_streams = array(
    'text/xml',
    'application/xml',
    'application/rdf+xml',
  );

  if (in_array($mime, $inline_xml_streams)) {
    return 'X';
  }
  else {
    return 'M';
  }
}
