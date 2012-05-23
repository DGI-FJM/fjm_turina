<div class="left">
  <span class="title"><?php echo $title; ?></span>
  <span class="desc"><?php if ($description):
    echo $description;
  else:
    echo t('No abstract.');
  endif;?></span>
  <!-- <span class="subtitle"><?php echo $subtitle; ?></span> -->
  <div class="desc-list">
    <?php foreach ($desc_list as $info):
      extract($info, EXTR_OVERWRITE | EXTR_PREFIX_ALL, 'desc');
      if ($desc_value): ?>
    <label><?php echo $desc_label; ?></label>
    <span><?php echo $desc_value; ?></span>
    <?php endif; 
    endforeach; ?>
  </div>
  <?php if ($pdf_url):?>
  <a class="pdf-link" href="<?php echo $pdf_url; ?>"><?php echo t('Download as PDF !icon', array('!icon' => '<span class="icon">&nbsp;</span>')); ?></a>
  <?php endif; ?>
</div>
<div class="right">
  <div class="fullview-link-wrapper">
    <a class="fullview-link" href="<?php echo $fullview_url; ?>"><?php echo t('View Full !icon', array('!icon' => '<span class="icon">&nbsp;</span>')); ?></a>
  </div>
  <img class="image" src="<?php echo $image_url; ?>"></img>
</div>