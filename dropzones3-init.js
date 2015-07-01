(function($) {
  Drupal.behaviors.dzs3 = {
    attach: function(context, settings) {
      $.each(settings.dzs3, function(selector) {
        $(selector, context).once('dzs3', function() {
          // Use a delta that never decrements so a newer file will never get
          // the delta of an older file if an intermediate file is deleted.
          var drupalMinDelta = 0;
          var template = document.querySelector(settings.dzs3[selector].previewTemplate);
          settings.dzs3[selector].previewTemplate = template.innerHTML;

          settings.dzs3[selector].dictConnectionError = "Connection Error. Click to resume.";
          settings.dzs3[selector].dictResponseError = "Fatal Error. Please retry later.";
          settings.dzs3[selector].dictDrupalFormSubmit = "Files in the process of being uploaded will not be saved. Continue?";

          settings.dzs3[selector].init = function() {
            var _this = this;
            // Add prepopulated files from a previous submission, if any.
            var containers = this.element.querySelectorAll("[data-drupal-preview-container]");
            for (var i = containers.length - 1; i >= 0; i--) {
              var file = {
                'filename': containers[i].querySelector("[data-drupal-filename]").value,
                'previewElement': containers[i],
                'fid': containers[i].querySelector("[data-drupal-fid]").value
              };

              var removeButton = containers[i].querySelector('[data-dzs3-remove]');
              removeButton.onclick = (function(_this, file) {
                return function() {
                  _this.dummyFiles = _this.dummyFiles.filter(function(f) {
                    return f.fid !== file.fid;
                  });
                  _this.emit("removedfile", file);
                  if (_this.dummyFiles.length === 0 && _this.files.length === 0) {
                    return _this.emit("reset");
                  }
                };
              })(this, file);
              this.dummyFiles.push(file);
              drupalMinDelta++;
            }

            // Ask before allowing form submission if files uploading.
            if (this.options.drupal.formSubmitConfirmation) {
              var form = document.getElementById(this.options.drupal.formId);
               form.addEventListener("submit", function(e) {
                var files = _this.getActiveFiles();
                if (files.length) {
                  e.preventDefault();
                  return DropzoneS3.confirm(_this.options.dictDrupalFormSubmit, function() {
                    return form.submit();
                  });
                }
              });
            }
          };

          settings.dzs3[selector].filesigned = function(file, auth, done) {
            var _ref0 = this.options.drupal.elementParents.slice(),
              field = _ref0.shift() + "[" + _ref0.join("][") + "][" + drupalMinDelta + "]";

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
            drupalMinDelta++;
            done();
          };
          new DropzoneS3(selector, settings.dzs3[selector]);
        });
      });
    }
  };
})(jQuery);
