<?php

// page callback
function fjm_turina_landing_page() {
  $bio = _fjm_turina_landing_page_bio();
  $content = _fjm_turina_landing_page_content();
  $more = _fjm_turina_landing_page_more();
  $collections = _fjm_turina_landing_page_collections();
  $browse = _fjm_turina_landing_page_browse();
  $search = _fjm_turina_landing_page_search();

  return theme('fjm_turina_landing', $bio, $content, $more, $collections, $browse, $search);
}

// bio text
function _fjm_turina_landing_page_bio() {
  $output = '<p class="quote">' . t('“For me, music is a “diary” of life itself” (1944)') . '</p>';
  $output .= '<p>' . t('The Archive of the composer Joaquín Turina is a passionate journey through his life and his work, and a faithful portrait of the time of great changes that he lived through.') . '</p>';
  
  return $output;
}

// menu content
function _fjm_turina_landing_page_content() {
  if (module_exists('i18nmenu')) {
    $content = i18nmenu_menu_navigation_links('menu-content');
  }
  else {
    $content = menu_navigation_links('menu-content');
  }
  
  return theme('links', $content, array('class' => 'fjm-turina-links-content'));
}

// menu more
function _fjm_turina_landing_page_more() {
  if (module_exists('i18nmenu')) {
    $content = i18nmenu_menu_navigation_links('menu-more');
  }
  else {
    $content = menu_navigation_links('menu-more');
  }
  
  return theme('links', $content, array('class' => 'fjm-turina-links-more'));
}

// menu collections
function _fjm_turina_landing_page_collections() {
  if (module_exists('i18nmenu')) {
    $music = i18nmenu_menu_navigation_links('menu-music');
    $personal_archive = i18nmenu_menu_navigation_links('menu-personal-archive');
  }
  else {
    $music = menu_navigation_links('menu-music');
    $personal_archive = menu_navigation_links('menu-personal-archive');
  }

  return theme('fjm_turina_landing_collections', $music, $personal_archive );
}

// menu browse
function _fjm_turina_landing_page_browse() {
  if (module_exists('i18nmenu')) {
    $browse = i18nmenu_menu_navigation_links('menu-browse');
  }
  else {
    $browse = menu_navigation_links('menu-browse');
  }

  return theme('links', $browse, array('class' => 'fjm-turina-links-browse'));
}

// search blocks
function _fjm_turina_landing_page_search() {
  $block = module_invoke('quicktabs', 'block', 'view', 1);
  return $block['content'];
}
