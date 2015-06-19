(function($) {
  Drupal.behaviors.dzs3 = {
    attach: function(context, settings) {
      $.each(settings.dzs3, function(selector) {
        $(selector, context).once('dzs3', function() {
          var completed = 0,
            template = document.querySelector(settings.dzs3[selector].previewTemplate);
          settings.dzs3[selector].previewTemplate = template.innerText;

          // Add the fid so file is associated with form.
          settings.dzs3[selector].success = function(file) {
            var _ref0 = this.options.drupal.elementParents.slice(),
              field = _ref0.shift() + "[" + _ref0.join("][") + "][" + file.delta + "]";

            if (file.previewElement) {
              file.previewElement.classList.add("dzs3-success");
            }

            var fid = file.previewElement.querySelectorAll("[data-drupal-uploadid]");
            for (var i = fid.length - 1; i >= 0; i--) {
              fid[i].name = field + "[uploadid]";
              fid[i].value = file.upload.auth.fid;
            }
            var filename = file.previewElement.querySelectorAll("[data-drupal-filename]");
            for (var j = filename.length - 1; j >= 0; j--) {
              filename[j].name = field + "[filename]";
              filename[j].value = file.name;
            }
          };
          new DropzoneS3(selector, settings.dzs3[selector]);
        });
      });
    }
  };
})(jQuery);
