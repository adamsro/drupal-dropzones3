<?php
/**
 * @file
 * Template file for Dropzone S3 Widget.
 */
?>
<div id="<?php print $element['#id']; ?>" tabindex="0" <?php print drupal_attributes($element['#wrapper_attributes']); ?>>
  <?php print drupal_render_children($element); ?>
</div>
<noscript><div class="dzs3-noscript"><?php print t('File uploader requires JavaScript be enabled.'); ?></div></noscript>
<?php print $element['#description']; ?>
