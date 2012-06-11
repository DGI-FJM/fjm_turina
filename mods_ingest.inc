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
function _fjm_turina_get_objects_from_mods_file($filename, $mods_xpath, $relationships) {
  $objects = array();
  foreach ($mods_xpath->query('/mods:mods | /mods:modsCollection/mods:mods') as $item) {
    $objects[] = _fjm_turina_get_objects_from_mods_doc($mods_xpath, $filename, $item, $relationships);
  }
  return $objects;
}


function _fjm_turina_get_objects_from_mods_doc($mods_xpath, $filename, $item, $relationships) {
  static $count = 0;
  $object = array(
    'relationships' => $relationships,
    'datastreams' => array(),
    'children' => array(),
  );

  $title_info = $mods_xpath->query('mods:titleInfo', $item)->item(0);
  
  $object['label'] = truncate_utf8($mods_xpath->evaluate('string(mods:title/text())', $title_info), 255, TRUE, TRUE);
  
  $bare_mods = new DOMDocument('1.0', 'utf-8');
  $mods_element = $bare_mods->importNode($item, TRUE);
  $bare_mods->appendChild($mods_element);
  
  $file_path = file_create_filename('ingest_MODS_tmp_' . $count++ . '.xml', file_directory_path());
   
  $bare_mods->save($file_path);

  //Add datastreams to the given object, based on inline XML.
  $object['datastreams']['MODS'] = array(
    'label' => 'MODS metadata record',
    'mimetype' => 'text/xml',
    'control_group' => 'X',
    'filepath' => $file_path,
    'fjm_turina_derived' => TRUE,
  );

  //Add datastream to the given object, base on files.
  $path = dirname($filename);
  $basename = basename($filename);
  $file_base = substr($basename, 0, strrpos($basename, '.'));
  
  if ((($file_path = "$path/$file_base/$file_base.pdf") && is_readable($file_path)) ||
    (($file_path = "$path/$file_base/$file_base.PDF") && is_readable($file_path))) {
    $object['datastreams'] += _fjm_turina_datastream_from_file($file_path);
  }
  
  $pattern = '^' . $file_base . '.*\.([jJ][pP][eE]?[gG]|[tT][iI][fF]{1,2})$';
  $files = file_scan_directory($path, $pattern);
  foreach ($files as $file) {
    $object['children'][] = _fjm_turina_get_child_from_file($file);
  }

  return $object;
}

function _fjm_turina_get_child_from_file($file) {
  $filename = $file->filename;

  $period_pos = strrpos($filename, '.');
  $file_base = substr($filename, 0, $period_pos);
  $page_number = intval(substr($file_base, strrpos($file_base, '_') + 1));

  
  $object = array(
    'label' => "Página-$page_number",
    'relationships' => array(
      array('hasModel', 'islandora:pageCModel', 'info:fedora/fedora-system:def/model#'),
      array('isMemberOf', '%parent%'),
      array('isPageOf', '%parent%', ISLANDORA_PAGE_URI),
      array('isPageNumber', $page_number, ISLANDORA_PAGE_URI, RELS_TYPE_INT),
    ),
    'datastreams' => _fjm_turina_datastream_from_file($filename),
    'children' => array(),
  );

  return $object;
}
