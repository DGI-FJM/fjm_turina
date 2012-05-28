<?php

define(TURINA_NAMESPACE, 'jt-test');
module_load_include('inc', 'fedora_repository', 'api/fedora_item');
module_load_include('inc', 'fjm_turina', 'ingest-help');

/**
 * Start a batch ingest.
 */
function fjm_turina_batch_ingest($do_batch = TRUE) {
  $batch = array(
    'title' => 'Turina batch ingest',
    'file' => drupal_get_path('module', 'fjm_turina') . '/fjm_turina.batch.inc',
    'finished' => 'fjm_turina_finish_batch_ingest',
    'operations' => array(),
  );
  
  $serial_prefix = 'serialized_book_';
  $info_dir = _fjm_turina_assemble_info($serial_prefix);
  
  $files = file_scan_directory($info_dir, "^$serial_prefix");
  while ($file = array_shift($files)) {
    $batch['operations'][] = array('_fjm_turina_unserialize_and_ingest', array($file->filename));
  }
  $batch['operations'][] = array('_fjm_turina_delete_empty_dir', array($info_dir));

  if ($do_batch) {
    batch_set($batch);
    batch_process('fedora/repository');
  }
  else {
    $asdf = array();
    
    foreach ($batch['operations'] as $op) {
      list($func, $args) = $op;
      $args = array_merge($args, array(&$asdf));
      call_user_func_array($func, $args);
    }
  }
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

function _fjm_turina_delete_empty_dir($dir, &$context) {
  $context['message'] = t('Cleaning up...');
  $dir_contents = scandir($dir);
  if (empty($dir_contents)) {
    rmdir($dir);
    $context['results'][] = t('Removed empty staging directory.');
  }
  else {
    $context['results'][] = t('Staging directory %dir not empty!  Leaving as-is. Contents: @contents', array(
      '%dir' => $dir,
      '@contents' => print_r($dir_contents, TRUE),
    ));
  }
}

function _fjm_turina_unserialize_file($filename) {
  return unserialize(file_get_contents($filename));
}

function _fjm_turina_get_batch_pid(&$context) {
  static $namespace = TURINA_NAMESPACE; //Don't want/need to be able to change namespaces between calls.
  static $var = 'remaining_allocated_pids';
  if (!isset($context['results'][$var])) {
    $context['results'][$var] = array();
  }
  
  $allocated_pids =& $context['results'][$var];
  
  if (empty($allocated_pids)) {
    $pid_pool_size = 4;
    $allocated_pids = Fedora_Item::get_next_PID_in_namespace($namespace, $pid_pool_size);
    $allocated_pids = (array)$allocated_pids; //Doesn't return as an array if there's only one...
  }
  
  return array_shift($allocated_pids);
}

function _fjm_turina_batch_ingest_object(&$object, $parent_pid, &$context) {
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
          '@relationship' => print_r($relationship, TRUE),
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
  
  $context['results'][] = t('Ingested object @object_label with @pid.', array(
    '@object_label' => $object['label'],
    '@pid' => $pid,
  ));
  
  drupal_alter('fjm_turina_post_ingest', $object, $parent_pid);
  
  foreach ($object['children'] as &$child) {
    _fjm_turina_batch_ingest_object($child, $pid, $context);
  }
}

function _fjm_turina_unserialize_and_ingest($filename, &$context) {
  $object = _fjm_turina_unserialize_file($filename);
  file_delete($filename);
  
  $time = microtime(TRUE);
  _fjm_turina_batch_ingest_object($object, NULL, $context);
  $delta = microtime(TRUE) - $time;
  
  $context['message'] = $context['results'][] = t('Ingested %label in @time seconds.', array(
    '%label' => $object['label'],
    '@time' => $delta,
  ));
}
