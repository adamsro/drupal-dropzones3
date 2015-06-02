(function($) {
  Drupal.behaviors.dzs3 = {
    attach: function(context, settings) {
      $.each(settings.dzs3, function(selector) {
        settings.dzs3[selector].validation.maxFilesize = 1000 * 100; // 100 GB
        new DropzoneS3(selector, settings.dzs3[selector]);
      });
    }
  };
})(jQuery);
