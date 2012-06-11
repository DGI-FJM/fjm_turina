<?php

module_load_include('inc', 'fedora_repository', 'api/fedora_item'); //For the PAGE_URI constant
module_load_include('inc', 'fjm_turina', 'ingest-help');

/**
 *
 * @param $filename string
 * @param $parent_relationship array
 *   Set of parameters to pass to Fedora_Item's add_relationship method;
 *   %parent% may be used for object placeholder in children.
 */
function _fjm_turina_get_objects_from_mets_file($filename, $mets_xpath, $relationships) {
  $objects = array();
  foreach ($mets_xpath->query('//mets:structMap[@TYPE="physical"]/mets:div[@TYPE="item"]') as $item) {
    $objects[] = _fjm_turina_get_objects_from_mets_div($mets_xpath, $filename, $item, $relationships);
  }
  return $objects;
}

function _fjm_turina_get_objects_from_mets_div($mets_xpath, $filename, $item, $relationships) {
  $object = array(
    'relationships' => $relationships,
    'datastreams' => array(),
    'children' => array(),
  );

  $object['label'] = truncate_utf8($item->getAttribute('LABEL'), 255, TRUE, TRUE);

  $dmdid = $item->getAttribute('DMDID');
  //Add datastreams to the given object, based on inline XML.
  $object['datastreams'] += _fjm_turina_get_datastream_from_dmdsec($mets_xpath, $dmdid);

  //Add datastream to the given object, base on file pointers.
  foreach ($mets_xpath->query('mets:fptr', $item) as $pointer) {
    $object['datastreams'] += _fjm_turina_get_datastream_from_mets_file($mets_xpath, $filename, $pointer->getAttribute('FILEID'));
  }
  
  //Add datastream to the given object, base on files.
  $path = dirname($filename);
  $basename = basename($filename);
  $file_base = substr($basename, 0, strrpos($basename, '-metsmods.'));
  
  if ((($file_path = "$path/$file_base/$file_base.pdf") && is_readable($file_path)) ||
    (($file_path = "$path/$file_base/$file_base.PDF") && is_readable($file_path))) {
    $object['datastreams'] += _fjm_turina_datastream_from_file($file_path);
  }

  //Add children objects recursively.
  //TODO:  Add in the required relationships.
  foreach ($mets_xpath->query('mets:div', $item) as $sub_item) {
    //Assume pages for now...
    $page_relations = array(
      array('hasModel', 'islandora:pageCModel', 'info:fedora/fedora-system:def/model#'),
      array('isMemberOf', '%parent%'),
      array('isPageOf', '%parent%', ISLANDORA_PAGE_URI),
      array('isPageNumber', $sub_item->getAttribute('ORDER'), ISLANDORA_PAGE_URI, RELS_TYPE_INT),
    );
    $object['children'][] = _fjm_turina_get_objects_from_mets_div($mets_xpath, $filename, $sub_item, $page_relations);
  }

  return $object;
}

function _fjm_turina_get_datastream_from_dmdsec($mets_xpath, $dmdid) {
  static $count = 0; //XXX: Used to help avoid naming collisions (and the resulting looping)
  $datastreams = array();

  foreach ($mets_xpath->query("//mets:dmdSec[@ID='$dmdid']/mets:mdWrap") as $top_wrapper) {
    $datastream = array();
    $dsid = $top_wrapper->getAttribute('MDTYPE');
    $datastream['label'] = "$dsid metadata record";
    $datastream['mimetype'] = $top_wrapper->getAttribute('MIMETYPE');

    //Get the xmlData element's content
    if (($results = $mets_xpath->query('mets:xmlData/*', $top_wrapper)) && $results->length > 0) {
      $bare_metadata_xml = new DOMDocument('1.0', 'utf-8');
      $metadata_element = $bare_metadata_xml->importNode($results->item(0), TRUE);
      $bare_metadata_xml->appendChild($metadata_element);
      
      $file_path = file_create_filename('ingest_mets_MODS_tmp_' . $count++ . '.xml', file_directory_path());
   
      $bare_metadata_xml->save($file_path);
      
      $datastream['filepath'] = $file_path;
      //XXX:  Bit of a hack...  Flag this "temp" file get deleted later... (via a hook in the module proper)
      $datastream['control_group'] = _fjm_turina_get_control_group_from_mime($datastream['mimetype']);
      $datastream['fjm_turina_derived'] = TRUE;
      $datastreams[$dsid] = $datastream;
    }
    else {
      drush_print_r('Crazy time!');
    }
  }

  return $datastreams;
}

function _fjm_turina_get_datastream_from_mets_file($mets_xpath, $mets_file, $fileid) {
  $datastreams = array();

  foreach ($mets_xpath->query("//mets:file[@ID='$fileid']/mets:FLocat") as $file) {
    $datastream = array();

    $path_components = explode(DIRECTORY_SEPARATOR, dirname($mets_file));
    $mets_path = $file->getAttributeNS('http://www.w3.org/1999/xlink', 'href');
    $file_components = array_filter(explode("\\", $mets_path));
    $path_components = array_filter(array_merge($path_components, $file_components));

    $file_path = implode(DIRECTORY_SEPARATOR, $path_components);
    $file_ext = substr($file_path, strrpos($file_path, '.') + 1);
    $dsid = strtoupper($file_ext); //Just use the extension as the datastream id.

    $datastream['label'] = "Archival $dsid";
    $mimetype = $datastream['mimetype'] = _fjm_turina_get_mimetype($file_path);
    
    if ($mimetype == 'image/jpeg') {
    $dsid = 'FULL_JPG';
    }
    elseif ($mimetype == 'image/tiff') {
      $dsid = 'TIFF';
    }
    
    $datastream['control_group'] = _fjm_turina_get_control_group_from_mime($mimetype);
    $datastream['filepath'] = $file_path;

    $datastreams[$dsid] = $datastream;
  }

  return $datastreams;
}
