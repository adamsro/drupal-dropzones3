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
              _ref0 = this.options.drupal.elementParents.slice(),
              _ref1 = this.options.drupal.elementParents.slice(),
              field = _ref0.shift() + "[" + _ref0.join("][") + "][" + file.delta + "]",
              // field = _ref0.shift() + "[" + _ref0.join("][") + "]",
              endpoint = _this.options.notifying.endpoint + "/" + _ref1.join("/") + "/" + file.delta;

            formData = new FormData(document.getElementById(this.options.drupal.formId));
            formData.append(field + "[filename]", file.name);
            formData.append(field + "[filesize]", file.size);
            formData.append(field + "[key]", file.upload.auth.key);
            // formData.append(field + "[delta]", file.delta);
            // formData.append(field + "[fid]", 0);

            formData.append('_triggering_element_name', _ref1.join("_") + "_upload_button");
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

            var fid = file.previewElement.querySelectorAll("[data-drupal-fid]");
            for (var i = fid.length - 1; i >= 0; i--) {
              fid[i].name = field + "[fid]";
              fid[i].value = file.fid;
            }
            var filename = file.previewElement.querySelectorAll("[data-drupal-filename]");
            for (var j = filename.length - 1; j >= 0; j--) {
              filename[j].name = field + "[filename]";
              filename[j].value = file.name;
            }
            var filesize = file.previewElement.querySelectorAll("[data-drupal-filesize]");
            for (var k = filesize.length - 1; k >= 0; k--) {
              filesize[k].name = field + "[filesize]";
              filesize[k].value = file.size;
            }
            var filemime = file.previewElement.querySelectorAll("[data-drupal-filemime]");
            for (var l = filemime.length - 1; l >= 0; l--) {
              filemime[l].name = field + "[filemime]";
              filemime[l].value = file.type;
            }
          };
          new DropzoneS3(selector, settings.dzs3[selector]);
        });
      });
    }
  };
})(jQuery);
