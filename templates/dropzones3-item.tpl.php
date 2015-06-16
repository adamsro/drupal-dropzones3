<div class="dzs3-preview dzs3-file-preview">
  <div class="dzs3-image dzs3-ext dzs3-ext-<?php print pathinfo($file->filename, PATHINFO_EXTENSION);?>" data-dzs3-thumb-container><img data-dzs3-thumbnail /></div>
  <div class="dzs3-details">
    <div class="dzs3-size"><span data-dzs3-size><?php print $file->filesize; ?></span></div>
    <div class="dzs3-filename"><span data-dzs3-name><?php print $file->filename; ?></span></div>
  </div>
  <div class="dzs3-error-message"><span data-dzs3-errormessage></span></div>
  <div class="dzs3-pause-message"><span data-dzs3-pausemessage></span></div>
  <div class="dzs3-success-mark">
    <svg xmlns="http://www.w3.org/2000/svg" width="54" height="54" viewBox="0 0 54 54" version="1.1"><title>Check</title><g stroke="none" stroke-width="1" fill="none" fill-rule="evenodd"><path d="M23.5 31.8L17.6 25.9C16 24.4 13.5 24.4 11.9 25.9 10.4 27.5 10.4 30 11.9 31.6L20.4 40.1C20.5 40.2 20.6 40.3 20.7 40.3 22.3 41.9 24.8 41.9 26.3 40.3L43.3 23.3C44.9 21.8 44.9 19.2 43.3 17.7 41.8 16.1 39.2 16.1 37.7 17.7L23.5 31.8ZM27 53C41.4 53 53 41.4 53 27 53 12.6 41.4 1 27 1 12.6 1 1 12.6 1 27 1 41.4 12.6 53 27 53Z" stroke-opacity="0.2" stroke="#747474" fill-opacity="0.8" fill="#FFFFFF"/></g></svg>
  </div>
  <div class="dzs3-error-mark">
    <svg xmlns="http://www.w3.org/2000/svg" width="54" height="54" viewBox="0 0 54 54" version="1.1"><title>Error</title><g stroke="none" stroke-width="1" fill="none" fill-rule="evenodd"><g stroke="#747474" stroke-opacity="0.2" fill="#FFFFFF" fill-opacity="0.8"><path d="M32.7 29L38.3 23.3C39.9 21.8 39.9 19.2 38.3 17.7 36.8 16.1 34.2 16.1 32.7 17.7L27 23.3 21.3 17.7C19.8 16.1 17.2 16.1 15.7 17.7 14.1 19.2 14.1 21.8 15.7 23.3L21.3 29 15.7 34.7C14.1 36.2 14.1 38.8 15.7 40.3 17.2 41.9 19.8 41.9 21.3 40.3L27 34.7 32.7 40.3C34.2 41.9 36.8 41.9 38.3 40.3 39.9 38.8 39.9 36.2 38.3 34.7L32.7 29ZM27 53C41.4 53 53 41.4 53 27 53 12.6 41.4 1 27 1 12.6 1 1 12.6 1 27 1 41.4 12.6 53 27 53Z"/></g></g></svg>
  </div>
  <div class="dzs3-resume-mark">
    <svg xmlns="http://www.w3.org/2000/svg" width="54" height="54" viewBox="0 0 54 54" version="1.1"><title>Continue</title><g stroke="none" stroke-width="1" fill="none" fill-rule="evenodd"><path d="M27 53C41.4 53 53 41.4 53 27 53 12.6 41.4 1 27 1 12.6 1 1 12.6 1 27 1 41.4 12.6 53 27 53ZM16.9 19.9L13.2 19.9 13.2 33.9 16.9 33.9 16.9 19.9 16.9 19.9ZM19.9 19.9L29.1 19.9 29 13.6C29 12 30 11.5 31.3 12.6L46.4 25.1C47.7 26.1 47.7 27.8 46.4 28.9L31.6 41.1C30.3 42.1 29.2 41.6 29.2 40L29.2 33.9 29.2 33.9 19.9 33.9 19.9 19.9 19.9 19.9ZM10.2 19.9L10 19.9C8.3 19.9 7 21.2 7 22.9L7 30.9C7 32.5 8.3 33.9 10 33.9L10.2 33.9 10.2 19.9Z" stroke-opacity="0.2" stroke="#747474" fill-opacity="0.8" fill="#FFFFFF"/></g></svg>
  </div>
  <?php print drupal_render_children($element); ?>
</div>
