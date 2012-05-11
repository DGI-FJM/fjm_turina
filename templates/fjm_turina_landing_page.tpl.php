<?php

/**
 * @file
 * template file for fjm_turina landing page 
 */

?>
<div id="turina-landing-page">
  <div class="border-top"></div>
  <div class="front-wrapper-top">
    <div class="front-bio">
      <div class="bio-image"></div>
      <p><?php print $bio; ?></p>
    </div>
    <div class="front-postcard"></div>
  </div>
  <div class="front-wrapper-bottom">
    <div class="front-wrapper-bottom-inner">
      <div class="front-content col-1">
        <h2 class="front-title"><?php print t('Content'); ?></h2>
        <?php print $content; ?>
      </div>
      <div class="front-collections col-1">
        <h2 class="front-title"><?php print t('Collections'); ?></h2>
        <?php print $collections; ?>
      </div>
      <div class="front-browse col-1">
        <h2 class="front-title"><?php print t('Browse'); ?></h2>
        <?php print $browse; ?>
      </div>
      <div class="front-search col-2">
        <h2 class="front-title"><?php print t('Search'); ?></h2>
        <?php print $search; ?>
      </div>
    </div>
  </div>
</div>