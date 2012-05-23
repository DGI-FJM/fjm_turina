<?php ?>
<ul class="turina-letterer">
  <li>
    <a href="<?php echo url('names');?>" class="letter"><?php echo t('ALL');?></a>
<?php foreach ($queries as $query => $count):?>
  <li>
  <?php 
  $disp = $map[$query];
  $class = 'letter';
  if ($disp == $selected):
    $class .= ' selected';
  endif;
  
  if ($count > 0):?>
    <a href="<?php echo url("names/$disp");?>" class="<?php echo $class; ?>"><?php echo $disp ;?></a>
<?php else:?>
    <span class="<?php echo $class; ?>"><?php echo $disp; ?></span>
<?php endif;?>
  </li>
<?php endforeach;?>
</ul>
