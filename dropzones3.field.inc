<?php
/**
 * @file
 * Functions for Jquery File Upload Element.
 */

/**
 * Implements hook_field_widget_info().
 */
function dropzones3_field_widget_info() {
  return array(
    'dropzones3' => array(
      'label' => t('Dropzone S3'),
      'field types' => array('file', 'image'),
      'settings' => array(
        'autostart' => TRUE,
        'ssencrypt' => TRUE,
        'message' => t('Drop files here or click to upload.'),
        'preview_image_style' => 'medium',
      ),
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_CUSTOM,
        'default value' => FIELD_BEHAVIOR_NONE,
      ),
    ),
  );
}

/**
 * Implements hook_form_FORM_ID_alter() for field_ui_field_edit_form().
 */
function dropzones3_form_field_ui_field_edit_form_alter(&$form, &$form_state, $form_id) {
  if ($form['#instance']['widget']['type'] === 'dropzones3') {
    // Repurpose the file_directory field.
    $form['instance']['settings']['file_directory']['#description'] = t(
      'Optional subdirectory within the S3 bucket in which to place files uploaded through this field. Do not include preceding or trailing slashes.'
    );
    // Max filesize no longer dependent on PHP/server configuration limitations.
    $form['instance']['settings']['max_filesize']['#description'] = t("You may enter a value greater than the default maximum upload size above. It can exceed PHP's maximum post and file upload sizes as well.");
  }
}

/**
 * Implements hook_field_widget_settings_form().
 */
function dropzones3_field_widget_settings_form($field, $instance) {
  $settings = $instance['widget']['settings'];

  $form = array();
  $form['autostart'] = array(
    '#type' => 'checkbox',
    '#title' => t('Start uploading as soon as files are added?'),
    '#default_value' => $settings['autostart'],
    '#description' => t('When checked, upload will start as soon as files are added without requiring to click <em>Upload</em>, unless some of the added files did not pass validation.'),
  );
  $form['ssencrypt'] = array(
    '#type' => 'checkbox',
    '#title' => t('Server Side Encryption'),
    '#default_value' => $settings['ssencrypt'],
    '#description' => t("Send header 'x-amz-server-side-encryption':'AES256' with requests."),
  );
  $form['message'] = array(
    '#type' => 'textarea',
    '#title' => t('Text in drop area'),
    '#default_value' => $settings['message'],
  );
  $form['preview_image_style'] = array(
    '#title' => t('Preview image style'),
    '#type' => 'select',
    '#options' => image_style_options(FALSE, PASS_THROUGH),
    '#empty_option' => '<' . t('no preview') . '>',
    '#default_value' => $settings['preview_image_style'],
    '#description' => t('The preview image will be shown while editing the content.'),
    '#weight' => 15,
  );
  return $form;
}

/**
 * Implements hook_field_widget_form().
 */
function dropzones3_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {

  foreach ($items as $item) {
    $element[$delta]['#file'] = $item;
    $element[$delta]['#weight'] = $delta;
    $element[$delta]['#theme'] = 'dropzones3_item';
    $delta++;
  }
  $element += array(
    '#file_upload_delta' => $delta,
    '#upload_location' => dropzones3_widget_uri($field, $instance),
    '#upload_validators' => array(
      'file_validate_name_length' => array(),
    ),
    // Allows this field to return an array instead of a single value.
    '#extended' => TRUE,
    // Tell form API that this element will have input values to be saved.
    '#input' => TRUE,
    '#value_callback' => 'dropzones3_field_widget_value',
    '#element_validate' => array('dropzones3_field_widget_validate'),
    '#theme' => 'dropzones3_container',
    '#process' => array('dropzones3_widget_process'),
    '#pre_render' => array('dropzones3_widget_pre_render'),
    '#attached' => array(
      'js' => array(
        drupal_get_path('module', 'dropzones3') . '/dropzones3-init.js',
      ),
    ),
  );
  if (!empty($instance['settings']['file_extensions'])){
    $element['#upload_validators']['file_validate_extensions'] = array($instance['settings']['file_extensions']);
  }
  if (!empty($instance['settings']['max_filesize'])){
    $element['#upload_validators']['file_validate_size'] = array(parse_size($instance['settings']['max_filesize']));
  }
  return $element;
}

/**
 * Callback #value_callback.
 */
function dropzones3_field_widget_value($element, $input = FALSE, $form_state = array()) {
  global $user;
  $return = array();
  $field = field_widget_field($element, $form_state);
  $instance = field_widget_instance($element, $form_state);

  if (empty($input)) {
    return array();
  }

  if ($field['cardinality'] != FIELD_CARDINALITY_UNLIMITED && $field['cardinality'] < count($input)) {
    form_error($element, t('Exceeded number of allowed files.'));
    return array();
  }

  foreach ($input as $value) {
    // Ensure that all the required properties are returned, even if empty.
    $value += array(
      'fid' => 0,
      'description' => '',
    // Checkboxes lose their value when empty.
    // If the display field is present make sure its unchecked value is saved.
      'display' => $field['settings']['display_field'] ? 0 : 1,
    );

    // If there's valid input, save the new upload.
    if (empty($value['fid']) && !empty($value['key'])) {

      // Remove root folder since s3fs adds it in getExternalUrl()
      // instead of saving it in the uri column for some reason...
      if (($config = _s3fs_get_config()) && !empty($config['root_folder'])) {
        $key = implode('/', array_slice(explode('/', $value['key']), 1));
      }
      else {
        $key = $value['key'];
      }
      $key = $field['settings']['uri_scheme'] . "://" . $key;

      // A protection against trying to resave a file if form is resubmitted.
      $files = file_load_multiple(array(), array('uri' => $key));
      if ($file = reset($files)) {
        continue;
      }

      $wrapper = new S3fsStreamWrapper();
      try {
        $wrapper->writeUriToCache($key);
      } catch (Exception $e) {
        // File not found in s3. Mostly likely the form submitted before upload
        // was completed.
        continue;
      }
      $record = db_select('s3fs_file', 's')
        ->fields('s')
        ->condition('uri', $key, '=')
        ->execute()
        ->fetchAssoc();

      // Construct a Drupal file object.
      $file = new stdClass();
      $file->uid = $user->uid;
      $file->filename = $value['filename'];
      $file->filesize = $value['filesize'] = $record['filesize'];
      $file->filemime = file_get_mimetype($file->filename);
      $file->uri = $key;
      $file->status = 0;
      $file->timestamp = REQUEST_TIME;

      _dropzones3_filename_safty($file, $element['#upload_validators']);

      // Save the uploaded file to the file_managed table.
      module_invoke_all('file_presave', $file);
      module_invoke_all('entity_presave', $file, 'file');

      drupal_write_record('file_managed', $file);
      // Inform modules about the newly added file.
      module_invoke_all('file_insert', $file);
      module_invoke_all('entity_insert', $file, 'file');

      // Clear the static loading cache.
      entity_get_controller('file')->resetCache(array($file->fid));

      // Call the validation functions specified by this function's caller.
      if($errors = file_validate($file, $element['#upload_validators'])) {
        form_error($element, t('File validation error.'));
        file_delete($file);
        continue;
      }
      $value['fid'] = $file->fid;
    }

    if (!empty($value['fid'])) {
      $value += (array) file_load($value['fid']);
    }

    $return[] = $value;
  }
  return $return;
}

/**
 * Callback #element_validate.
 */
function dropzones3_field_widget_validate(&$element, &$form_state) {
  if ($element['#required'] && !element_children($element)) {
    form_error($element, t('!name field is required.', array('!name' => $element['#title'])));
  }
}

function _dropzones3_filename_safty(&$file, $validators) {
    $extensions = '';
  if (isset($validators['file_validate_extensions'])) {
    if (isset($validators['file_validate_extensions'][0])) {
      // Build the list of non-munged extensions if the caller provided them.
      $extensions = $validators['file_validate_extensions'][0];
    }
    else {
      // If 'file_validate_extensions' is set and the list is empty then the
      // caller wants to allow any extension. In this case we have to remove the
      // validator or else it will reject all extensions.
      unset($validators['file_validate_extensions']);
    }
  }
  else {
    // No validator was provided, so add one using the default list.
    // Build a default non-munged safe list for file_munge_filename().
    $extensions = 'jpg jpeg gif png txt doc xls pdf ppt pps odt ods odp';
    $validators['file_validate_extensions'] = array();
    $validators['file_validate_extensions'][0] = $extensions;
  }

  if (!empty($extensions)) {
    // Munge the filename to protect against possible malicious extension hiding
    // within an unknown file type (ie: filename.html.foo).
    $file->filename = file_munge_filename($file->filename, $extensions);
  }

  // Rename potentially executable files, to help prevent exploits (i.e. will
  // rename filename.php.foo and filename.php to filename.php.foo.txt and
  // filename.php.txt, respectively). Don't rename if 'allow_insecure_uploads'
  // evaluates to TRUE.
  if (!variable_get('allow_insecure_uploads', 0) && preg_match('/\.(php|pl|py|cgi|asp|js)(\.|$)/i', $file->filename) && (substr($file->filename, -4) != '.txt')) {
    $file->filename .= '.txt';
    $file->filemime = 'text/plain';
    // The .txt extension may not be in the allowed list of extensions. We have
    // to add it here or else the file upload will fail.
    if (!empty($extensions)) {
      $validators['file_validate_extensions'][0] .= ' txt';
      drupal_set_message(t('For security reasons, your upload has been renamed to %filename.', array('%filename' => $file->filename)));
    }
  }
}

function dropzones3_widget_process($element, &$form_state, $form) {
  $field = field_widget_field($element, $form_state);
  $instance = field_widget_instance($element, $form_state);
  $config = _s3fs_get_config();

  // We will need the cached form for ajax_get_form() in the signing endpoint.
  $form_state['cache'] = TRUE;

  // Only runs if form validation fails with files successfully uploaded.
  if (!empty($element['#value'])) {
    foreach(element_children($element) as $key) {
      unset($element[$key]);
    }
    $delta = 0;
    foreach ($element['#value'] as $item) {
      $element[$delta]['#file'] = $item;
      $element[$delta]['#weight'] = $delta;
      $element[$delta]['#theme'] = 'dropzones3_item';
      $delta++;
    }
    $element['#file_upload_delta'] = $delta;
  }

  dropzones3_add_template("dropzones3-item-client", theme('dropzones3_item_client'));

  $element['#message'] = $instance['widget']['settings']['message'];
  $element['#description'] = theme('file_upload_help', array('description' => $element['#description'], 'upload_validators' => $element['#upload_validators']));

  // Dropzone expects a dot before all accepted extensions to
  // differentiate between accepted MIME types.
  $dot = function($string) {
    return '.' . $string;
  };
  if (isset($element['#upload_validators']['file_validate_extensions'][0])) {
    $extension_list = implode(',', array_map($dot, array_filter(explode(' ', $instance['settings']['file_extensions']))));
  }

  $element['#attached']['js'][] = array(
    'type' => 'setting',
    'data' => array(
      'dzs3' => array(
        '#' . $element['#id'] => array(
          // Drupal Specific used in notify function.
          'drupal' => array(
            // Get element name without the last element since we'll add it in JS.
            'elementParents' => $element['#parents'],
            'formId' => $form['#id'],
            'formSubmitConfirmation' => TRUE,
          ),
          's3' => array(
            'accesskey' => variable_get('dropzones3_aws_access_key'),
            'region' => $config['region'],
            'bucket' => $config['bucket'],
            'ssencrypt' => $instance['widget']['settings']['ssencrypt'],
          ),
          'signing' => array(
            'endpoint' => '/dropzones3/sign/' . implode('/', $element['#array_parents']) . '/' . $form['form_build_id']['#value'],
          ),
          'autoQueue' => $instance['widget']['settings']['autostart'],
          'previewTemplate' => '#dropzones3-item-client',
          'thumbnails' => array(
            'createImageThumbnails' => FALSE,
          ),
          'validation' => array(
            'maxFiles' => $field['cardinality'] == FIELD_CARDINALITY_UNLIMITED ? NULL : $field['cardinality'],
            // Convert bytes to KB to MB.
            'maxFilesize' => !empty($instance['settings']['max_filesize']) ? parse_size($instance['settings']['max_filesize']) / 1024 / 1024: NULL,
            'acceptedFiles' => isset($extension_list) ? $extension_list : NULL,
          ),
          'addRemoveLinks' => FALSE,
          'resuming' => array('localStorageResume' => FALSE),
        ),
      ),
    ),
  );
  return $element;
}

/**
 * The #pre_render callback for element.
 */
function dropzones3_widget_pre_render($element) {
  if (($library = libraries_load('cryptojs')) && empty($library['loaded'])) {
    drupal_set_message("Library 'CryptoJS' not found.", 'error');
  }
  if (($library = libraries_load('dropzones3')) && empty($library['loaded'])) {
    drupal_set_message("Library 'Dropzone S3' not found.", 'error');
  }
  return $element;
}

/**
 * Determines the URI for a file field instance.
 *
 * @param $field
 *   A field array.
 * @param $instance
 *   A field instance array.
 * @param $data
 *   An array of token objects to pass to token_replace().
 *
 * @return
 *   A file directory URI with tokens replaced.
 *
 * @see file_field_widget_uri()
 */
function dropzones3_widget_uri($field, $instance, $data = array()) {
  $config = _s3fs_get_config();
  $destination = trim($instance['settings']['file_directory'], '/');

  // Replace tokens.
  $destination = token_replace($destination, $data);

  if ($config['root_folder']) {
    return $field['settings']['uri_scheme'] . '://' . $config['root_folder']. '/' . $destination;
  }
  return $field['settings']['uri_scheme'] . '://' . $destination;
}
