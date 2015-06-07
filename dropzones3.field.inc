<?php
/**
 * @file
 * Functions for Jquery File Upload Element.
 */

/**
 * Implements hook_element_info().
 */
function dropzones3_element_info() {
  $file_path = drupal_get_path('module', 'dropzones3');
  $config = _s3fs_get_config();
  $types['dropzones3'] = array(
    '#input' => TRUE,
    '#value_callback' => 'dropzones3_upload_value',
    // '#element_validate' => array('file_managed_file_validate'),
    '#process' => array('dropzones3_element_process'),
    '#pre_render' => array('dropzones3_element_pre_render'),
    '#theme' => 'dropzones3',
    '#message' => t('Drop files here or click to upload.'),
    // The S3 directory relative to the bucket root where files are uploaded.
    '#upload_location' => !empty($config['root_folder']) ? $config['root_folder'] : '',
    '#upload_validators' => array(),
    '#preview_template' => '#dropzones3-item',
    '#extended' => TRUE,
    '#autostart' => TRUE,
    '#ssencrypt' => FALSE,
    '#description' => '',
    '#attached' => array(
      'js' => array(
        $file_path . '/dropzones3-init.js',
      ),
    ),
  );
  return $types;
}

/**
 * The #process callback for element.
 */
function dropzones3_element_process($element, &$form_state, $form) {
  dropzones3_add_template("dropzones3-item", theme('dropzones3-item'));

  // Dropzone expects a dot before all accepted extensions to
  // differentiate between accepted MIME types.
  $dot = function($string) {
    return '.' . $string;
  };
  if (isset($element['#upload_validators']['file_validate_extensions'][0])) {
    $extension_list = implode(',', array_map($dot, array_filter(explode(' ', $element['#upload_validators']['file_validate_extensions'][0]))));
  }

  $signing_endpoint = '/js/dropzones3/sign/' . implode('/', $element['#array_parents']) . '/' . $form['form_build_id']['#value'];
  $notifying_endpoint = '/js/dropzones3/notify/' . implode('/', $element['#array_parents']) . '/' . $form['form_build_id']['#value'];

  $element['#attached']['js'][] = array(
    'type' => 'setting',
    'data' => array(
      'dzs3' => array(
        '#' . $element['#id'] => array(
          // Drupal Specific used in notify function.
          'drupalElementName' => $element['#name'],
          'drupalFormId' => $form['#id'],

          'autoQueue' => $element['#autostart'],
          'previewTemplate' => $element['#preview_template'],
          's3' => array(
            'accesskey' => variable_get('dropzones3_aws_access_key'),
            'region' => variable_get('dropzones3_aws_region'),
            'bucket' => variable_get('dropzones3_aws_bucket'),
            'ssencrypt' => $element['#server_side_encrypt'],
          ),
          'signing' => array(
            'endpoint' => $signing_endpoint,
            'ssencrypt' => (bool) $element['#server_side_encrypt'],
          ),
          'notifying' => array(
            'endpoint' => $notifying_endpoint,
          ),
          'validation' => array(
            // 'maxFiles' => $element['#max_files'],
            // Convert bytes to KB to MB.
            'maxFilesize' => $element['#upload_validators']['file_validate_size'][0] / 1024 / 1024,
            'acceptedFiles' => isset($extension_list) ? $extension_list : NULL,
          ),
        ),
      ),
    ),
  );
  return $element;
}

/**
 * The #pre_render callback for element.
 */
function dropzones3_element_pre_render($element) {
  if (($library = libraries_load('cryptojs')) && empty($library['loaded'])) {
    drupal_set_message("Library 'CryptoJS' not found.", 'error');
  }
  if (($library = libraries_load('dropzones3')) && empty($library['loaded'])) {
    drupal_set_message("Library 'Dropzone S3' not found.", 'error');
  }
  return $element;
}

/**
 * Implements hook_field_widget_info().
 */
function dropzones3_field_widget_info() {
  return array(
    'dropzones3' => array(
      'label' => t('Dropzone S3'),
      'field types' => array('file', 'image'),
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_CUSTOM,
        'default value' => FIELD_BEHAVIOR_DEFAULT,
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
    '#default_value' => $settings['message'] ? $settings['message'] : t('Drop files here or click to upload.'),
  );
  return $form;
}

/**
 * Implements hook_field_widget_form().
 */
function dropzones3_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {

  $defaults = array(
    'fid' => 0,
    'display' => !empty($field['settings']['display_default']),
    'description' => '',
  );

  $max_filesize = parse_size($instance['settings']['max_filesize']);

  // Load the items for form rebuilds from the field state as they might not be
  // in $form_state['values'] because of validation limitations. Also, they are
  // only passed in as $items when editing existing entities.
  $field_state = field_form_get_state($element['#field_parents'], $field['field_name'], $langcode, $form_state);
  if (isset($field_state['items'])) {
    $items = $field_state['items'];
  }
  $element_info = element_info('dropzones3');
  $element += array(
    '#type' => 'dropzones3',
    '#upload_location' => $instance['settings']['file_directory'],
    '#upload_validators' => array(
      'file_validate_name_length' => array(),
      'file_validate_size' => array($max_filesize),
      'file_validate_extensions' => array($instance['settings']['file_extensions']),
    ),
    '#value_callback' => 'dropzones3_field_widget_value',
    '#ssencrypt' => $instance['settings']['ssencrypt'],
    // Allows this field to return an array instead of a single value.
    '#extended' => TRUE,
  );
  if ($field['cardinality'] == 1) {
    // Set the default value.
    $element['#default_value'] = !empty($items) ? $items[0] : $defaults;
    // If there's only one field, return it as delta 0.
    if (empty($element['#default_value']['fid'])) {
      $element['#description'] = theme('file_upload_help', array('description' => $element['#description'], 'upload_validators' => $element['#upload_validators']));
    }
  }
  else {
    foreach ($items as $item) {
      $element['#children'][$delta] = $element;
      $element['#children'][$delta]['#default_value'] = $item;
      $element['#children'][$delta]['#weight'] = $delta;
      $delta++;
    }
    $element['#file_upload_delta'] = $delta;
    $element['#theme'] = 'file_widget_multiple';
  }
  return $element;
}

/**
 * Callback #value_callback.
 */
function dropzones3_field_widget_value($element, $input = FALSE, $form_state = array()) {
  if ($input) {
    // Checkboxes lose their value when empty.
    // If the display field is present make sure its unchecked value is saved.
    $field = field_widget_field($element, $form_state);
    if (empty($input['display'])) {
      $input['display'] = $field['settings']['display_field'] ? 0 : 1;
    }
  }

  // Handle uploads and the like.
  $return = dropzones3_upload_value($element, $input, $form_state);

  // Ensure that all the required properties are returned, even if empty.
  $return += array(
    'fid' => 0,
    'display' => 1,
    'description' => '',
  );

  return $return;
}

/**
 * Callback #value_callback.
 */
function dropzones3_upload_value($element, $input = FALSE, $form_state = array()) {
  global $user;
  $return = array();
  $parents_id = implode('_', $element['#parents']);

  if (!empty($input['fid'])) {
    // The input will have a non-zero fid only when saving the full node form.
    // We don't want to do anything when that happens, because everything has
    // already been done in the AJAX workflow.
    return $input;
  }

  $remove_button_clicked = (isset($form_state['input']['_triggering_element_name'])
    && $form_state['input']['_triggering_element_name'] == "{$parents_id}_remove_button");

  // If there's valid input, save the new upload.
  if ($input !== FALSE && !empty($input['filename']) && !$remove_button_clicked) {
    $return = $input;
    $base_dir = '';
    if (isset($element['#upload_location']) && file_uri_target($element['#upload_location']) != '') {
      $base_dir = file_uri_target($element['#upload_location']) . '/';
    }
    $uri = "s3://$base_dir{$input['filename']}";

    // We dont want to overwrite metadata for an existing file. A user could change
    // $_POST['key'] to point to a file they didnt own and gain access to it.
    $file_exists = db_select('s3fs_file', 's')
      ->fields('s')
      ->condition('uri', $uri, '=')
      ->execute();
    if ($file_exists->rowCount()) {
      drupal_add_http_header('Status', '400 Bad Request');
      drupal_exit();
    }

    // Store the file's metadata into s3fs's metadata cache.
    try {
      $wrapper = new S3fsStreamWrapper();
      $wrapper->writeUriToCache($uri);
    }
    catch (Exception $e) {
      watchdog_exception('dropzones3', $e);
      drupal_add_http_header('Status', '500 Error');
      drupal_exit();
    }

    // Actual filesize reported by s3. Delete file if it exceeds max size.
    // Try to midigate 'fill up the bucket' attack.
    $result = db_select('s3fs_file', 's')
      ->fields('s', array('filesize'))
      ->condition('uri', $uri, '=')
      ->execute()
      ->fetchAssoc();

    // Construct a Drupal file object.
    $file = new stdClass();
    $file->uid = $user->uid;
    $file->filename = $_POST['filename'];
    $file->filesize = $result['filesize'];
    $file->filemime = file_get_mimetype($file->uri);
    $file->uri = $uri;
    $file->status = 0;
    $file->timestamp = REQUEST_TIME;

    // Save the uploaded file to the file_managed table.
    module_invoke_all('file_presave', $file);
    module_invoke_all('entity_presave', $file, 'file');

    drupal_write_record('file_managed', $file);
    // Inform modules about the newly added file.
    module_invoke_all('file_insert', $file);
    module_invoke_all('entity_insert', $file, 'file');

    // Clear the static loading cache.
    entity_get_controller('file')->resetCache(array($file->fid));
    $return['fid'] = $file->fid;
  }

  return $return;
}
