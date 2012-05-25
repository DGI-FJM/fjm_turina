<?php ?>
<ul class="turina-letterer">
  <li class="<?php
  $class = 'letter';
  if (t('ALL') == $selected):
    $class .= ' selected';
  endif;
  echo $class;?>">
    <a href="<?php echo url($path);?>"><?php echo t('ALL');?></a>
<?php foreach ($queries as $query => $count):
  $disp = $map[$query];
  $class = 'letter';
  if ($disp == $selected):
    $class .= ' selected';
  endif;?>
  <li class="<?php echo $class; ?>">
<?php 
  if ($count > 0):?>
    <a href="<?php echo url("$path/$disp");?>"><?php echo $disp ;?></a>
<?php else:
      echo $disp;
    endif;?>
  </li>
<?php endforeach;?>
</ul>
