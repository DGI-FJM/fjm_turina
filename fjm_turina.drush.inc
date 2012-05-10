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
  $objects = array();
  foreach (_fjm_turina_get_xml_files() as $file) {
    $objects += _fjm_turina_get_objects_from_mets_file($file->filename, array('isMemberOfCollection', 'jt:test'), NULL);
  }
  
  drush_print_r($objects);
}

/**
 * 
 * @param $filename string
 * @param $parent_relationship array
 *   Set of parameters to pass to Fedora_Item's add_relationship method; 
 *   %parent% may be used for object placeholder in children.
 */
function _fjm_turina_get_objects_from_mets_file($filename, $parent_relationship, $cm_relationship) {
  $mets_xpath = new DOMXPath(DOMDocument::load("./$filename"));
  $mets_xpath->registerNamespace('mets', 'http://www.loc.gov/METS/');
  $mets_xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
  
  $objects = array();
  foreach ($mets_xpath->query('//mets:structMap[@TYPE="physical"]/mets:div[@TYPE="item"]') as $item) {
    $objects[] = _fjm_turina_get_objects_from_mets_div($mets_xpath, $filename, $item, array($parent_relationship));
  }
  return $objects;
}

function _fjm_turina_get_objects_from_mets_div($mets_xpath, $filename, $item, $relationships) {
  $object = array(
    'relationships' => $relationships,
    'datastreams' => array(),
    'children' => array(),
  );
  
  $object['label'] = $item->getAttribute('LABEL');
  
  $dmdid = $item->getAttribute('DMDID');
  //Add datastreams to the given object, based on inline XML.
  $object['datastreams'] += _fjm_turina_get_datastream_from_dmdsec($mets_xpath, $dmdid);
  
  //Add datastream to the given object, base on file pointers.
  foreach ($mets_xpath->query('mets:fptr', $item) as $pointer) {
    $object['datastreams'] += _fjm_turina_get_datastream_from_file($mets_xpath, $filename, $pointer->getAttribute('FILEID'));
  }
  
  //Add children objects recursively.
  //TODO:  Add in the required relationships.
  foreach ($mets_xpath->query('mets:div', $item) as $sub_item) {
    //Assume pages for now...
    $page_relations = array(
      array('hasModel', 'islandora:pageCModel', 'info:fedora/fedora-system:def/model#'),
      array('isMemberOf', '%parent%'),
      array('isPageOf', '%parent%', ISLANDORA_PAGE_URI),
      array('isPageNumber', $sub_item->getAttribute('ORDER'), ISLANDORA_PAGE_URI),
    );
    $object['children'][] = _fjm_turina_get_objects_from_mets_div($mets_xpath, $filename, $sub_item, $page_relations);
  }

  return $object;
}

function _fjm_turina_get_datastream_from_dmdsec($mets_xpath, $dmdid) {
  $datastreams = array();
  
  foreach ($mets_xpath->query("//mets:dmdSec[@ID='$dmdid']/mets:mdWrap") as $top_wrapper) {
    $datastream = array();
    $dsid = $top_wrapper->getAttribute('MDTYPE');
    $datastream['label'] = "$dsid metadata record";
    $datastream['mimetype'] = $top_wrapper->getAttribute('MIMETYPE');
    
    //Get the xmlData element's content
    if (($results = $mets_xpath->query('mets:xmlData/*', $top_wrapper)) && $results->length > 0) {
      $datastream['content'] = $mets_xpath->document->saveXML($results->item(0));
      $datastreams[$dsid] = $datastream;
    }
    else {
      drush_print_r('Crazy time!');
    }
  }
  
  return $datastreams;
}

function _fjm_turina_get_datastream_from_file($mets_xpath, $mets_file, $fileid) {
  $datastreams = array();
  
  foreach ($mets_xpath->query("//mets:file[@ID='$fileid']/mets:FLocat") as $file) {
    $datastream = array();
    
    $path_components = explode(DIRECTORY_SEPARATOR, dirname($mets_file));
    $mets_path = $file->getAttributeNS('http://www.w3.org/1999/xlink', 'href');
    $file_components = array_filter(explode("\\", $mets_path));
    $path_components = array_filter(array_merge($path_components, $file_components));
    drush_print_r($path_components);
    $file_path = implode(DIRECTORY_SEPARATOR, $path_components);
    $file_ext = substr($file_path, strrpos($file_path, '.') + 1);
    $dsid = strtoupper($file_ext); //Just use the extension as the datastream id.
    
    $datastream['label'] = "Archival $dsid";
    $datastream['mimetype'] = mime_content_type($file_path);
    $datastream['fileurl'] = file_create_url($file_path);
    
    $datastreams[$dsid] = $datastream;
  }
  
  return $datastreams;
}
