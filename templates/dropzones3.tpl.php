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
    <?php print drupal_render_children($element); ?>
  </div>
</div>
<noscript><div class="dzs3-noscript"><?php print t('File uploader requires JavaScript be enabled.'); ?></div></noscript>
<?php print render($element['#description']); ?>
