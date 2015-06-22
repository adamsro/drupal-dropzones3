(function($) {
  Drupal.behaviors.dzs3 = {
    attach: function(context, settings) {
      $.each(settings.dzs3, function(selector) {
        $(selector, context).once('dzs3', function() {
          var template = document.querySelector(settings.dzs3[selector].previewTemplate);

          settings.dzs3[selector].previewTemplate = template.innerText;

          settings.dzs3[selector].init = function() {
            var containers = this.element.querySelectorAll("[data-drupal-preview-container]");
            for (var i = containers.length - 1; i >= 0; i--) {
              var file = new File([""], containers[i].querySelector("[data-drupal-filename]").value);
              file.previewElement = containers[i];
              file.fid = containers[i].querySelector("[data-drupal-fid]").value;
              var removeButton = containers[i].querySelector('[data-dzs3-remove]');
              removeButton.onclick = (function(file) {
                return function() {
                  file.previewElement.parentNode.removeChild(file.previewElement);
                };
              })(file);
              this.dummyFiles.push(file);
            }
          };

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
