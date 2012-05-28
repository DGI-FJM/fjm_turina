<?php 

define(TURINA_NAMESPACE, 'jt-test');

function fjm_turina_drush_command() {
  $command = array();
  
  $command['fjm_turina_ingest'] = array(
    'callback' => 'drush_fjm_turina_ingest',
    'description' => 'Ingest objects.',
    'example' => array(
      'drush -l <drupal site URL> fjm_turina_ingest',
    ),
    //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  
  $command['fjm_turina_purge'] = array(
    'callback' => 'drush_fjm_turina_purge',
    'description' => 'Purge the defined namespace.',
    'example' => array(
      'drush [-l <drupal site URL>] fjm_turina_purge [fedora namespace]',
    ),
    //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  
  $command['fjm_turina_reingest'] = array(
    'callback' => 'drush_fjm_turina_reingest',
    'description' => 'Convenience; purges and then ingests',
    'example' => array(
      'drush [-l <drupal site URL>] fjm_turina_reingest',
    ),
    //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  
  $command['fjm_turina_reindex'] = array(
    'callback' => 'drush_fjm_turina_reindex',
    'description' => 'Reindex everything in the given namespace',
    'example' => array(
      'drush [-l <drupal site URL>] fjm_turina_reindex <namespace> [limit]',
    ),
    //'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  
  return $command;
}

function drush_fjm_turina_reingest() {
  drush_fjm_turina_purge();
  drush_fjm_turina_ingest();
}

function drush_fjm_turina_reindex($namespace = TURINA_NAMESPACE) {
  $fedora_url = variable_get('fedora_base_url', 'http://localhost:8080/fedora');
  $parsed_fedora = $pf = parse_url($fedora_url);
  $gsearch_url = "{$pf['scheme']}://{$pf['host']}:{$pf['port']}/fedoragsearch/rest";
  
  global $user;
  $headers = array(
    'Authorization' => "Basic " . base64_encode("{$user->name}:{$user->pass}"),
  );
  
  foreach (_fjm_turina_select_namespace($namespace) as $result) {
    $url = url($gsearch_url, array(
      'query' => array(
        'operation' => 'updateIndex',
        'action' => 'fromPid',
        'value' => $result['sub'],
      ),
    ));

    $stat = drupal_http_request($url, $headers);
    
    if ($stat->code >= 200 && $stat->code < 300) {
      //Success?
      drush_log("Reindexed {$result['sub']}");
    }
    else {
      //Fail...
      drush_print_r($stat);
      break;
    }
  }
}

function drush_fjm_turina_purge($namespace = TURINA_NAMESPACE) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  foreach (_fjm_turina_select_namespace($namespace) as $result) {
    $item = new Fedora_Item($result['sub']);
    $item->move_to_trash();
    $item->modify_object($item->objectProfile->objLabel, 'D');
    $item->purge('Nuked via Drush script');
    $item->forget();
    drush_log("Purged {$item->pid}.");
  }
}

function _fjm_turina_select_namespace($namespace = TURINA_NAMESPACE, $limit = -1) {
  $namespace_query = <<<END_QUERY
SELECT ?sub
FROM <#ri>
WHERE {
  ?sub <info:fedora/fedora-system:def/model#hasModel> ?obj .
  FILTER(regex(str(?sub), '^info:fedora/$namespace:'))
}
END_QUERY;
  if (strlen($namespace) > 1) {
    module_load_include('inc', 'fedora_repository', 'ObjectHelper');
    return ObjectHelper::performSparqlQuery($namespace_query, $limit);
  }
  else {
    return array();
  }
}



/**
 * Command callback; kick off the ingest process.
 */
function drush_fjm_turina_ingest() {
  module_load_include('inc', 'fjm_turina', 'ingest-help');
  
  $info_dir = _fjm_turina_assemble_info();
  
  /*
  foreach (file_scan_directory($info_dir, "^$serial_prefix") as $serial_file) {
    $objects = unserialize(file_get_contents($serial_file->filename));
    
    foreach ($objects as &$object) {
      _fjm_turina_ingest_object_drush($object);
    }
    
    file_delete($serial_file->filename);
  }
  */
}



module_load_include('inc', 'fedora_repository', 'api/fedora_item');

/**
 * Get an unused PID from Fedora.
 * 
 * Allocates multiple, and returns one at a time.
 */
function _fjm_turina_get_pid($allocation_size = 20) {
  static $allocated_pids = array();
  static $namespace = TURINA_NAMESPACE; //Don't want/need to be able to change namespaces between calls.
  
  if (!$allocated_pids) {
    $pid_pool_size = $allocation_size;
    $allocated_pids = Fedora_Item::get_next_PID_in_namespace($namespace, $pid_pool_size);
    $allocated_pids = (array)$allocated_pids; //Doesn't return as an array if there's only one...
  }
  
  return array_shift($allocated_pids);
}

/**
 * Ingest the given object,
 * 
 * @param $object &array
 *   An associative array describing the object, comprised of:
 *   - 'label': A string containing a label for the object.
 *   - 'relationships': An array of arrays, containing sets of parameters to
 *     pass to Fedora_Item->add_datastream.
 *   - 'datastreams': An associative array of arrays; keys are datastream IDs,
 *     and the values are associative arrays describing a datastream, 
 *     containing:
 *     - 'label': A string containing a label for this datastream.
 *     - 'mimetype': A string containing the mimetype of the content.
 *     One of:
 *     - 'fileurl': A string containing an absolute URL to a resource.
 *     - 'filepath': A string containing a relative path to a resource
 *       (relative to Drupal's root directory).
 *     - 'content': A string containing the resource content.
 *     And optionally:
 *     - 'control_group': A single character indicating the control group for
 *       the datastream (one of X, M, R, E); defaults to 'X' or 'M' as
 *       appropriate (based on mimetype)
 *   - 'children': An array of child object, described in a similar manner to
 *     the current being ingested--used against this function recursively.
 * @param $parent_pid string
 *   A string containing the PID of the 'parent' object, used to fill in the
 *   '%parent%' placeholder in relationships.
 */
function _fjm_turina_ingest_object_drush(array &$object, $parent_pid = NULL) {
  drupal_alter('fjm_turina_pre_ingest', $objects);
  
  $pid = NULL;
  $item = NULL;
  if (isset($object['pid']) && ($pid = $object['pid'])) {
    $item = new Fedora_Item($pid);
  }
  else {
    $pid = $object['pid'] = _fjm_turina_get_pid();
    $item = Fedora_Item::ingest_new_item($pid, 'A', $object['label']);
  }
  
  foreach ($object['relationships'] as &$relationship) {
    if ($relationship[1] == '%parent%') {
      if ($parent_pid !== NULL) {
        $relationship[1] = $parent_pid;
      }
      else {
        drush_print_r('%parent% placeholder used, but no parent PID!');
        drush_print_r($object);
        exit(2);
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
      $item->add_datastream_from_url($dsinfo['fileurl'], $dsid, $label, $mimetype, $control_group, 'Added from Drush ingest script.');
    }
    elseif (isset($dsinfo['filepath'])) {
      $item->add_datastream_from_file($dsinfo['filepath'], $dsid, $label, $mimetype, $control_group, 'Added from Drush ingest script.');
    }
    elseif (isset($dsinfo['content'])) {
      $item->add_datastream_from_string($dsinfo['content'], $dsid, $label, $mimetype, $control_group, 'Added from Drush ingest script.');
    }
    else {
      drush_log("No content specified for datastream $dsid!", 'warning');
    }
  }
  
  drush_log("Ingested $pid.");
  
  foreach ($object['children'] as &$child) {
    _fjm_turina_ingest_object_drush($child, $pid);
  }
  
  drupal_alter('fjm_turina_post_ingest', $objects);
}
