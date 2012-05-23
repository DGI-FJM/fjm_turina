<?php

module_load_include('php', 'islandora_solr_search', 'SolrPhpClient/Apache/Solr/Service');

function _ft_nf() {
  return 'mods_name_primary_personal_ms';
}

function _fjm_turina_letterer($solr, $prefix) {
  $name_field = _ft_nf();
  
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
  );
  
  $query = "$name_field:[* TO *]";
  $result_object = json_decode($solr->search($query, 0, 0, $params)->getRawResponse());
  
  $names = array();
  foreach ($result_object->facet_counts->facet_fields->$name_field as $name => $count) {
    $names[] = $name;
  }
  
  return theme('turina_letterer', $result_object->facet_counts->facet_queries, $fq_map, $prefix);
}

function fjm_turina_names($prefix = 'A') {
  //Use Solr faceting to get list of names...
  $parsed_solr_url = parse_url(variable_get('islandora_solr_search_block_url', 'http://localhost:8080/solr'));
  $solr = new Apache_Solr_Service($parsed_solr_url['host'], $parsed_solr_url['port'], $parsed_solr_url['path']);

  //Get the 'letterer' (as opposed to a 'pager'); breaks up the results by letter.
  $letterer = _fjm_turina_letterer($solr, $prefix);
  
  //Get the actual results.
  $name_field = _ft_nf();
  $query = "$name_field:[* TO *]";
  if ($q !== NULL) {
    $query .= "AND $q";
  }
  
  $params = array(
    'facet' => 'true',
    'facet.field' => $name_field,
    'facet.sort' => 'index',
    'facet.mincount' => 1,
  );
  if ($prefix != 'ALL') {
    $params['facet.prefix'] = $prefix;
  }
  
  $result_object = json_decode($solr->search($query, 0, 0, $params)->getRawResponse());

  $names = array();
  foreach ($result_object->facet_counts->facet_fields->$name_field as $name => $count) {
    $names[] = $name;
  }
  
  $output = array(
    array(
      '#prefix' => '<div class="turina-names">',
      '#suffix' => '</div>',
      array(
        '#value' => $letterer,
      ),
      array(
        '#value' => theme('turina_name_list', $names, $name_field),
      ),
      array(
        '#value' => $letterer,
      )
    )
  );
  
  return drupal_render($output);
}
