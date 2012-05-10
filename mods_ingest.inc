<?php

/**
 *
 * @param $filename string
 * @param $parent_relationship array
 *   Set of parameters to pass to Fedora_Item's add_relationship method;
 *   %parent% may be used for object placeholder in children.
 */
function _fjm_turina_get_objects_from_mods_file($filename, $mods_xpath, $parent_relationship, $cm_relationship) {


  $objects = array();
  foreach ($mods_xpath->query('/mods:mods | /mods:modsCollection/mods:mods') as $item) {
    $objects[] = _fjm_turina_get_objects_from_mods_doc($mods_xpath, $filename, $item, array($parent_relationship));
  }
  return $objects;
}


function _fjm_turina_get_objects_from_mods_doc($mods_xpath, $filename, $item, $relationships) {
  $object = array(
    'relationships' => $relationships,
    'datastreams' => array(),
    'children' => array(),
  );

  $title_info = $mods_xpath->query('mods:titleInfo', $item)->item(0);
  
  $object['label'] = $mods_xpath->evaluate('string(mods:title/text())', $title_info);

  //Add datastreams to the given object, based on inline XML.
  $object['datastreams']['MODS'] = array(
    'label' => 'MODS metadata record',
    'mimetype' => 'text/xml',
    'content' => $mods_xpath->document->saveXML($item),
  );

  //Add datastream to the given object, base on files.
  $path = dirname($filename);
  $basename = basename($filename);
  $file_base = substr($basename, 0, strrpos($basename, '.'));
  $pattern = '^' . $file_base . '.*\.(jpg|tiff)$';
  drush_print($pattern);
  $files = file_scan_directory($path, $pattern);
  drush_print_r($files);
  foreach ($files as $file) {
    $object['children'][] = _fjm_turina_get_child_from_file($file);
  }

  return $object;
}

function _fjm_turina_get_child_from_file($file) {
  $page_number = 0;
  $filename = $file->filename;
  $file_ext = substr($filename, strrpos($filename, '.') + 1);
  $dsid = strtoupper($file_ext);
  
  $object = array(
    'relations' => array(
      array('hasModel', 'islandora:pageCModel', 'info:fedora/fedora-system:def/model#'),
      array('isMemberOf', '%parent%'),
      array('isPageOf', '%parent%', ISLANDORA_PAGE_URI),
      array('isPageNumber', $page_number, ISLANDORA_PAGE_URI),
    ),
    'datastreams' => array(
      $dsid => array(
        'label' => "Archival $dsid",
        'mimetype' => mime_content_type($filename),
        'fileurl' => file_create_url($filename),
      ),
    ),
    'children' => array(),
  );

  return $object;
}