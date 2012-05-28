<?php

/**
 * Start a batch ingest.
 */
function fjm_turina_batch_ingest() {
  module_load_include('inc', 'fjm_turina', 'ingest-help');
  
  $batch = array(
    'file' => 'fjm_turina.batch.inc',
    'finished' => 'fjm_turina_finish_batch_ingest',
  );
  
  $serial_prefix = 'serialized_book_';
  $info_dir = _fjm_turina_assemble_info($serial_prefix);
  
  $files = file_scan_directory($info_dir, "^$serial_prefix");
  while ($file = array_shift($files)) {
    $batch['operations'][] = array('_fjm_turina_unserialize_and_ingest', $file->filename);
  }
  
  batch_set($batch);
  batch_process('fedora/repository');
}

function fjm_turina_finish_batch_ingest($success, $results, $operations) {
  //Save results to a file.
  $file_name = file_create_filename('ingest-results.dat', file_directory_path());
  file_put_contents($file_name, serialize($results));
  watchdog('fjm_turina_batch', 'Wrote batch result data to %file_name.', array('%file_name' => $file_name), WATCHDOG_INFO);
  
  if (!$success) {
    $file_name = file_create_filename('ingest-unfinished.dat', file_directory_path());
    file_put_contents($file_name, serialize($operations));
    watchdog('fjm_turina_batch', 'Some operations failed; written to %file_name.', array('%file_name' => $file_name), WATCHDOG_WARNING);
  }
}

function _fjm_turina_unserialize_file($filename) {
  return unserialize(file_get_contents($filename));
}

function _fjm_turina_get_batch_pid(&$context) {
  static $namespace = TURINA_NAMESPACE; //Don't want/need to be able to change namespaces between calls.
  
  if (!isset($context['results']['remaining_allocated_pids'])) {
    $context['results']['remaining_allocated_pids'] = array();
  }
  
  $allocated_pids =& $context['results']['remaining_allocated_pids'];
  
  if (empty($allocated_pids)) {
    $pid_pool_size = 50;
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    $allocated_pids = Fedora_Item::get_next_PID_in_namespace($namespace, $pid_pool_size);
    $allocated_pids = (array)$allocated_pids; //Doesn't return as an array if there's only one...
  }
  
  return array_shift($allocated_pids);
}

function _fjm_turina_batch_ingest_object(&$object, $parent_pid, &context) {
  drupal_alter('fjm_turina_pre_ingest', $object, $parent_pid);
  
  $pid = NULL;
  $item = NULL;
  if (isset($object['pid']) && ($pid = $object['pid'])) {
    $item = new Fedora_Item($pid);
  }
  else {
    $pid = $object['pid'] = _fjm_turina_get_batch_pid($context);
    $item = Fedora_Item::ingest_new_item($pid, 'A', $object['label']);
  }
  
  foreach ($object['relationships'] as &$relationship) {
    if ($relationship[1] == '%parent%') {
      if ($parent_pid !== NULL) {
        $relationship[1] = $parent_pid;
      }
      else {
        $context['results'][] = t('WARNING: %parent% placeholder used, but no parent PID for @object_label (skipping relationship "@relationship")', array(
          '@object_label' => $object['label'],
          '@relationship' => print_r($relatioship, TRUE),
        ));
        continue;
      }
    }
  
    call_user_func_array(array($item, 'add_relationship'), $relationship);
  }
  
  foreach ($object['datastreams'] as $dsid => &$dsinfo) {
    $label = $dsinfo['label'];
    $mimetype = $dsinfo['mimetype'];
    $control_group = NULL;
    //XXX: Yeah...  This is pretty bad structure...  Assigning in the condition...  Anyway. :P
    if (!(isset($dsinfo['control_group']) && ($control_group = $dsinfo['control_group']))) {
      $control_group = $dsinfo['control_group'] = _fjm_turina_get_control_group_from_mime($mimetype);
    }
  
    if (isset($dsinfo['fileurl'])) {
      $item->add_datastream_from_url($dsinfo['fileurl'], $dsid, $label, $mimetype, $control_group, 'Added from Batch ingest script.');
    }
    elseif (isset($dsinfo['filepath'])) {
      $item->add_datastream_from_file($dsinfo['filepath'], $dsid, $label, $mimetype, $control_group, 'Added from Batch ingest script.');
    }
    elseif (isset($dsinfo['content'])) {
      $item->add_datastream_from_string($dsinfo['content'], $dsid, $label, $mimetype, $control_group, 'Added from Batch ingest script.');
    }
    else {
      $context['results'][] = t('ERROR: No content specified for datastream @dsid in @pid!', array(
        '@pid' => $pid,
        '@dsid' => $dsid,
      ));
    }
  }
  
  $context['results'][] t("Ingested @object_label with @pid.", array(
    '@object_label' => $object_label,
    '@pid' => $pid,
  ));
  
  drupal_alter('fjm_turina_post_ingest', $objects, $parent_pid);
  
  foreach ($object['children'] as &$child) {
    _fjm_turina_batch_ingest_object($child, $pid, $context);
  }
}

function _fjm_turina_unserialize_and_ingest($filename, &$context) {
  $object = _fjm_turina_unserialize($filename);
  file_delete($filename);
  _fjm_turina_batch_ingest_object($object, NULL, $context);
}
