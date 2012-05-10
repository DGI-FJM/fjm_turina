<?php 

function fjm_turina_drush_command() {
  $command = array();
  
  $command['fjm_turina_ingest'] = array(
    'callback' => 'fjm_turina_ingest',
    'description' => 'Ingest objects.',
    'example' => array(
      'drush -l <drupal site URL> fjm_turina_ingest',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  
  return $command;
}

/**
 * 
 * @return array
 */
function _fjm_turina_get_xml_files() {
  $files = file_directory_path() . '/data';
  $xml_files = file_scan_directory($files, '\.xml');
  return $xml_files;
}

function fjm_turina_ingest() {
  module_load_include('inc', 'fjm_turina', 'mets_ingest');
  module_load_include('inc', 'fjm_turina', 'mods_ingest');
  
  $objects = array();
  
  foreach (_fjm_turina_get_xml_files() as $filename => $file) {
    $xml_xpath = new DOMXPath(DOMDocument::load("./$filename"));
    $xml_xpath->registerNamespace('mets', 'http://www.loc.gov/METS/');
    $xml_xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
    
    $collection_relation = array('isMemberOfCollection', 'jt:test');
    
    $new_objects = array();
    $new_objects = array_merge($new_objects, _fjm_turina_get_objects_from_mets_file($filename, $xml_xpath, $collection_relation, NULL));
    
    if (count($new_objects) === 0) {
      $new_objects = array_merge($new_objects, _fjm_turina_get_objects_from_mods_file($filename, $xml_xpath, $collection_relation, NULL));
    }
    
    $objects = array_merge($objects, $new_objects);
  }
  
  drush_print_r($objects);
}
