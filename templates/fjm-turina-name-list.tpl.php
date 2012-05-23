<ul class="turina-name-list">
  <?php foreach ($names as $name): ?>
  <li><?php echo l($name, "islandora/solr/search/*:*/$field:\"$name\"")?></li>
  <?php endforeach; ?>
</ul>
