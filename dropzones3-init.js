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
            file.delta = this.files.length - 1;
            var _this = this, xhr = new XMLHttpRequest(), _ref0 = this.options.drupal.elementParents.slice();
            var endpoint = _this.options.notifying.endpoint + "/" + _ref0.join("/") + "/" + file.delta;
            var field = _ref0.shift() + "[" + _ref0.join("][") + "][" + file.delta + "]";

            formData = new FormData(document.getElementById(this.options.drupal.formId));
            formData.append(field + "[filename]", file.name);
            formData.append(field + "[filesize]", file.size);
            formData.append(field + "[filemime]", file.type);
            formData.append(field + "[key]", file.upload.auth.key);
            formData.append(field + "[fid]", 0);
            formData.append(field + "[_weight]", file.delta);

            formData.append('_triggering_element_name', this.options.drupal.triggeringName);
            formData.append('_triggering_element_value', this.options.drupal.triggeringValue);

            xhr.onload = function() {
              if (xhr.status / 100 == 2) {
                try {
                  var item = JSON.parse(xhr.responseText);
                  file.fid = item.fid;
                  done(file);
                } catch (ex) {
                  _this._fatalError(file, ex.message);
                }
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
            xhr.open("POST", endpoint, true);
            // xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
            xhr.send(formData);
          };

          // Add the fid so file is associated with form.
          settings.dzs3[selector].success = function(file) {
            var _ref0 = this.options.drupal.elementParents.slice(),
              field = _ref0.shift() + "[" + _ref0.join("][") + "][" + file.delta + "]";
            if (file.previewElement) {
              file.previewElement.classList.add("dzs3-success");
            }
            _ref = file.previewElement.querySelectorAll("[data-dzs3-name]");
            var fid = file.previewElement.querySelectorAll("[data-drupal-fid]");
            for (var i = fid.length - 1; i >= 0; i--) {
              fid[i].name = field;
              fid[i].value = file.fid;
            }
            for (_i = 0, _len = _ref.length; _i < _len; _i++) {
              node = _ref[_i];
              node.textContent = file.name;
            }
          };
          new DropzoneS3(selector, settings.dzs3[selector]);
        });
      });
    }
  };
})(jQuery);
