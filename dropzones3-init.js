(function($) {
  Drupal.behaviors.dzs3 = {
    attach: function(context, settings) {
      $.each(settings.dzs3, function(selector) {
        $(selector, context).once('dzs3', function() {
          var completed = 0,
              template = document.querySelector(settings.dzs3[selector].previewTemplate);
          settings.dzs3[selector].previewTemplate = template.innerText;

          // Custom notify function called from emitter in DropzoneS3.prototype.finishUpload.
          settings.dzs3[selector].notify = function(file, done) {
            var _this = this,
              xhr = new XMLHttpRequest(),
              field = this.options.drupalElementParents + "[" + completed + "]";
              formData = new FormData(document.getElementById(this.options.drupalFormId));

              formData.append(field + "[filename]", file.name);
              formData.append(field + "[filesize]", file.size);
              formData.append(field + "[key]", file.upload.auth.key);
              formData.append(field + "[fid]", 0);
              formData.append(field + "[display]", 1);
              formData.append(field + "[_weight]", completed);

            xhr.onload = function() {
            if (xhr.status / 100 == 2) {
              completed++;
              done(file);
            } else if (xhr.status / 100 == 5) {
              // Hopefully a temporary server error
              return _this._recoverableError(file, xhr);
            } else {
              return _this._fatalError(file, _this.options.dictResponseError.replace("{{statusCode}}", xhr.status), xhr);
            }
            };
            xhr.onerror = xhr.ontimeout = function(e) {
              return _this._recoverableError(file, e.target);
            };

            xhr.timeout = 20000; // 20 seconds
            xhr.open("POST", this.options.notifying.endpoint, true);
            // xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
            xhr.send(formData);
          };

          new DropzoneS3(selector, settings.dzs3[selector]);
        });
      });
    }
  };
})(jQuery);
