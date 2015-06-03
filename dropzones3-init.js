(function($) {
  Drupal.behaviors.dzs3 = {
    attach: function(context, settings) {
      $.each(settings.dzs3, function(selector) {
        new DropzoneS3(selector, settings.dzs3[selector]);
      });
    }
  };
})(jQuery);
