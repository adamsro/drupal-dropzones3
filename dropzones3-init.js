(function($) {
  Drupal.behaviors.dzs3 = {
    attach: function(context, settings) {
      $.each(settings.dzs3, function(selector) {
        $(selector, context).once('dzs3', function() {
          var completed = 0,
            template = document.querySelector(settings.dzs3[selector].previewTemplate);

          settings.dzs3[selector].previewTemplate = template.innerText;

          settings.dzs3[selector].filesigned = function(file, auth, done) {
            var _ref0 = this.options.drupal.elementParents.slice(),
              field = _ref0.shift() + "[" + _ref0.join("][") + "][" + file.delta + "]";

            var key = file.previewElement.querySelectorAll("[data-drupal-key]");
            for (var i = key.length - 1; i >= 0; i--) {
              key[i].name = field + "[key]";
              key[i].value = auth.key;
            }
            var filename = file.previewElement.querySelectorAll("[data-drupal-filename]");
            for (var j = filename.length - 1; j >= 0; j--) {
              filename[j].name = field + "[filename]";
              filename[j].value = file.name;
            }
            done();
          };
          new DropzoneS3(selector, settings.dzs3[selector]);
        });
      });
    }
  };
})(jQuery);
