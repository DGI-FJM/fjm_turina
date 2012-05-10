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
  $files = file_directory_path() . '/data/turina';
  $xml_files = file_scan_directory($files, '\.xml');
  return $xml_files;
}

function fjm_turina_ingest() {
  
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
  
  
  foreach ($mets_xpath->query('//mets:structMap[@TYPE="physical"]/mets:div[@TYPE="item"]') as $item) {
    $object = array(
      'relationships' => array(
        //Sets of parameters to pass to Fedora_Item's add_relationship method.
        //$relationship, $object, $predicate namespaceURI, $literal
        $parent_relationship,
        $cm_relationship,
      ),
    );
    $object = array_merge($object, _fjm_turina_get_objects_from_mets_struct_div($mets_xpath, $item));
  }
}

function _fjm_turina_get_objects_from_mets_div($mets_xpath, $item, $relationships) {
  $object = array();
  $object['label'] = $item->getAttribute('LABEL');
  
  $dmdid = $item->getAttribute('DMDID');
  
  foreach ($mets_xpath->query('mets:div', $item) as $sub_item) {
    $object['children'][] = _fjm_turina_get_objects_from_mets_div($mets_xpath, $item);
  }
  return $object;
}
