<ul class="turina-facets">
  <?php foreach ($names as $name): ?>
  <li><?php echo l(truncate_utf8($name, 72, TRUE, TRUE), "islandora/solr/search/*:*/$field:\"$name\"")?></li>
  <?php endforeach; ?>
</ul>
