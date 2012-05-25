<?php

function fjm_turina_landing_page() {
  $bio = _fjm_turina_landing_page_bio();
  $content = _fjm_turina_landing_page_content();
  $collections = _fjm_turina_landing_page_collections();
  $browse = _fjm_turina_landing_page_browse();
  $search = _fjm_turina_landing_page_search();

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

function _fjm_turina_landing_page_render_links(array $links, $level = 0) {
  $output = '<ul class="fjm-turina-links-browse link-level-' . $level . '">';
  foreach ($links as $link) {
    if (isset($link['link']['hidden']) && $link['link']['hidden'] == '0') {
      $output .= '<li>' . theme('menu_item_link', $link['link']);
      if (isset($link['below']) && !empty($link['below'])) {
        $output .= _fjm_turina_landing_page_render_links($link['below'], $level + 1);
      }
      $output .= '</li>';
    }
  }
  $output .= '</ul>';
  return $output;
}

function _fjm_turina_landing_page_browse() {
  $browse = menu_tree_page_data('menu-browse');
  return _fjm_turina_landing_page_render_links($browse);
}

function _fjm_turina_landing_page_search() {
  $block = module_invoke('quicktabs', 'block', 'view', 1);
  return $block['content'];
}
