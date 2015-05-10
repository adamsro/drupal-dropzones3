(function($) {
  Drupal.behaviors.initFileUploadWidget = {
    attach: function(context, settings) {
      if (settings.dropzones3 && settings.dropzones3.elements) {
        $.each(settings.dropzones3.elements, function(selector) {
          var options = settings.dropzones3.elements[selector];
          new DropzoneS3(settings.dropzones3.elements[selector], settings.dropzones3.elements[selector]);
        });
      }
    }
  };
})(jQuery);
