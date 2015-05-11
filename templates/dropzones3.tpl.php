<?php
/**
 * @file
 * Template file for Dropzone S3 Widget.
 */
?>
<div id="<?php print $element['#id']; ?>" class="dzs3">
  <div class="dzs3-message">
  <?php print $element['#message']; ?>
  </div>
  <div class="dzs3-previewlist">
  </div>
</div>
<noscript><div class="dzs3-noscript"><?php print t('File uploader requires JavaScript be enabled.'); ?></div></noscript>
<?php print render($element['#children']); ?>
