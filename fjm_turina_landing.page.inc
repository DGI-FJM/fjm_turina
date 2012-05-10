<?php

function fjm_turina_landing_page() {
  $bio = _fjm_turina_landing_page_bio();
  $content = _fjm_turina_landing_page_content();
  $collections = _fjm_turina_landing_page_collections();
  $browse = _fjm_turina_landing_page_browse();
  $search = '';

  return theme('fjm_turina_landing', $bio, $content, $collections, $browse, $search);
}

function _fjm_turina_landing_page_bio() {
  $output .= t('Bio placeholder - English');
  
  return $output;
}

function _fjm_turina_landing_page_content() {
  $content = menu_navigation_links('menu-content');
  return theme('links', $content, array('class' => 'fjm-turina-links-content'));
}

function _fjm_turina_landing_page_collections() {
  $music = menu_navigation_links('menu-music');
  $personal_archive = menu_navigation_links('menu-personal-archive');

  return theme('fjm_turina_landing_collections', $music, $personal_archive );
}

function _fjm_turina_landing_page_browse() {
  $browse = menu_navigation_links('menu-browse');
  return theme('links', $browse, array('class' => 'fjm-turina-links-browse'));
}

function _fjm_turina_landing_page_search() {

}
