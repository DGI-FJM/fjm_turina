<?php

module_load_include('php', 'islandora_solr_search', 'SolrPhpClient/Apache/Solr/Service');

function _fjm_turina_letterer($solr, $prefix, $name_field = 'mods_name_ms', $path = 'names') {
  $fq = array();
  $fq_map = array();
  foreach (range('A', 'Z') as $letter) {
    $value = "$name_field:$letter*";
    $fq_map[$value] = $letter;
    $fq[] =  $value;
  }
  
  $params = array(
    'facet' => 'true',
    'facet.field' => $name_field,
    'facet.sort' => 'index',
    'facet.mincount' => 1,
    'facet.query' => $fq,
    'qt' => variable_get('islandora_solr_search_block_request_handler', 'standard'),
  );
  
  $query = "$name_field:[* TO *]";
  $result_object = json_decode($solr->search($query, 0, 0, $params)->getRawResponse());
  
  $names = array();
  foreach ($result_object->facet_counts->facet_fields->$name_field as $name => $count) {
    $names[] = $name;
  }
  
  return theme('turina_letterer', $result_object->facet_counts->facet_queries, $fq_map, $prefix, $path);
}

function _fjm_turina_facet_page($name_field, $prefix, $page_class = 'turina-names', $path = 'names') {
  //Use Solr faceting to get list of names...
  $parsed_solr_url = parse_url(variable_get('islandora_solr_search_block_url', 'http://localhost:8080/solr'));
  $solr = new Apache_Solr_Service($parsed_solr_url['host'], $parsed_solr_url['port'], $parsed_solr_url['path']);
  
  //Get the 'letterer' (as opposed to a 'pager'); breaks up the results by letter.
  $letterer = _fjm_turina_letterer($solr, $prefix, $name_field, $path);
  
  //Get the actual results.
  $query = "$name_field:[* TO *]";
  if ($q !== NULL) {
    $query .= "AND $q";
  }
  
  $params = array(
    'facet' => 'true',
    'facet.field' => $name_field,
    'facet.sort' => 'index',
    'facet.mincount' => 1,
    'qt' => variable_get('islandora_solr_search_block_request_handler', 'standard'),
  );
  if ($prefix != t('ALL')) {
    $params['facet.prefix'] = $prefix;
  }
  
  $result_object = json_decode($solr->search($query, 0, 0, $params)->getRawResponse());
  
  $names = array();
  foreach ($result_object->facet_counts->facet_fields->$name_field as $name => $count) {
    $names[] = $name;
  }
  
  return array(
    array(
      '#prefix' => "<div class=\"$page_class\">",
      '#suffix' => '</div>',
      array(
        '#value' => $letterer,
      ),
      array(
        '#value' => theme('turina_facet_list', $names, $name_field),
      ),
      array(
        '#value' => $letterer,
      )
    )
  );
}

function fjm_turina_names($prefix = NULL) {
  if ($prefix === NULL) {
    $prefix = t('ALL');
  }
  $name_field = variable_get('fjm_turina_solr_name_field', 'mods_name_primary_personal_ms');
  $output = _fjm_turina_facet_page($name_field, $prefix, 'facet-list turina-names', 'names');
  
  return drupal_render($output);
}

function fjm_turina_subjects($prefix = NULL) {
  if ($prefix === NULL) {
    $prefix = t('ALL');
  }
  
  $name_field = variable_get('fjm_turina_solr_subject_field', 'mods_subject_ms');
  $output = _fjm_turina_facet_page($name_field, $prefix, 'facet-list turina-subjects', 'subjects');
  
  return drupal_render($output);
}

function fjm_turina_genres($prefix = NULL) {
  if ($prefix === NULL) {
    $prefix = t('ALL');
  }

  $name_field = variable_get('fjm_turina_solr_genre_field', 'mods_genre_ms');
  $output = _fjm_turina_facet_page($name_field, $prefix, 'facet-list turina-genres', 'genres');

  return drupal_render($output);
}

function fjm_turina_titles($prefix = NULL) {
  if ($prefix === NULL) {
    $prefix = t('ALL');
  }

  $name_field = variable_get('fjm_turina_solr_title_field', 'mods_subject_title_ms');
  $output = _fjm_turina_facet_page($name_field, $prefix, 'facet-list turina-titles', 'titles');

  return drupal_render($output);
}
