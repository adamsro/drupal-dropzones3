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

  $elements = $element;
  $element += array(
    '#process' => array('dropzones3_element_process'),
    '#theme' => 'dropzones3_item',
  );

  foreach ($items as $item) {
    $elements[$delta] = $element;
    $elements[$delta]['#default_value'] = $item;
    $elements[$delta]['#weight'] = $delta;
    $delta++;
  }
  $elements += array(
    '#file_upload_delta' => $delta,
    '#upload_location' => $instance['settings']['file_directory'],
    '#upload_validators' => array(
      'file_validate_name_length' => array(),
      'file_validate_size' => array(parse_size($instance['settings']['max_filesize'])),
      'file_validate_extensions' => array($instance['settings']['file_extensions']),
    ),
    // Allows this field to return an array instead of a single value.
    '#extended' => TRUE,
    // Tell form API that this element will have input values to be saved.
    '#input' => TRUE,
    '#value_callback' => 'dropzones3_field_widget_value',
    '#theme' => 'dropzones3_container',
    '#process' => array('dropzones3_widget_process'),
    '#pre_render' => array('dropzones3_widget_pre_render'),
    '#description' => theme('file_upload_help', array('description' => $element['#description'], 'upload_validators' => $element['#upload_validators'])),
    '#attached' => array(
      'js' => array(
        drupal_get_path('module', 'dropzones3') . '/dropzones3-init.js',
      ),
    ),
  );

  return $elements;
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

  foreach ($input as $value) {
    // Ensure that all the required properties are returned, even if empty.
    $value += array(
      'fid' => 0,
      'description' => '',
    );
    // Checkboxes lose their value when empty.
    // If the display field is present make sure its unchecked value is saved.
    if (empty($value['display'])) {
      $value['display'] = $field['settings']['display_field'] ? 0 : 1;
    }
    // If there's valid input, save the new upload.
    if (empty($value['fid']) && !empty($value['key'])) {
      $wrapper = new S3fsStreamWrapper();
      try {
        $wrapper->writeUriToCache($value['key']);
      } catch (Exception $e) {
        // File not found in s3. Mostly likely the form submitted before upload was completed.
        continue;
      }
      $record = db_select('s3fs_file', 's')
        ->fields('s')
        ->condition('uri', $value['key'], '=')
        ->execute()
        ->fetchAssoc();

      // Construct a Drupal file object.
      $file = new stdClass();
      $file->uid = $user->uid;
      $file->filename = $value['filename'];
      $file->filesize = $record['filesize'];
      $file->filemime = file_get_mimetype($file->filename);
      $file->uri = "s3://" . $value['key'];
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

      $value['fid'] = $file->fid;

    }
    $return[] = $value;
  }
  return $return;
}

/**
 * The #process callback for element.
 */
function dropzones3_element_process($element, &$form_state, $form) {
  $fid = isset($element['#value']['fid']) ? $element['#value']['fid'] : 0;
  $element['#file'] = $fid ? file_load($fid) : FALSE;

  $instance = field_widget_instance($element, $form_state);

  // Needed when printing files from a previous submission.
  $element['fid'] = array(
    '#type' => 'hidden',
    '#value' => $fid,
    '#attributes' => array('data-drupal-fid' => array($fid)),
  );
  $element['filename'] = array(
    '#type' => 'hidden',
    '#value' => isset($element['#file']->filename) ? $element['#file']->filename : '',
    '#attributes' => array('data-drupal-filename' => array()),
  );
  // Add the image preview.
  if ($element['#file'] && $instance['widget']['settings']['preview_image_style']) {
    $variables = array(
      'style_name' => $instance['widget']['settings']['preview_image_style'],
      'path' => $element['#file']->uri,
    );
    $element['preview'] = array(
      '#type' => 'markup',
      // '#attributes' => array('data-dzs3-thumbnail' => array()),
      '#markup' => theme('image_style', $variables),
    );
  }

  return $element;
}

function dropzones3_widget_process($element, &$form_state, $form) {

  // We will need the cached form for ajax_get_form() in the signing endpoint.
  $form_state['cache'] = TRUE;

  // $field = field_widget_field($element, $form_state);
  $instance = field_widget_instance($element, $form_state);
  dropzones3_add_template("dropzones3-item-client", theme('dropzones3_item_client'));

  $element['#message'] = $instance['widget']['settings']['message'];
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
            'triggeringValue' => t('Upload'),
            'startingDelta' => $element['#file_upload_delta'],
          ),
          's3' => array(
            'accesskey' => variable_get('dropzones3_aws_access_key'),
            'region' => variable_get('dropzones3_aws_region'),
            'bucket' => variable_get('dropzones3_aws_bucket'),
            'ssencrypt' => $instance['widget']['settings']['ssencrypt'],
          ),
          'signing' => array(
            'endpoint' => '/js/dropzones3/sign/' . implode('/', $element['#array_parents']) . '/' . $form['form_build_id']['#value'],
          ),
          'autoQueue' => $instance['widget']['settings']['autostart'],
          'previewTemplate' => '#dropzones3-item-client',
          'validation' => array(
            // 'maxFiles' => $element['#max_files'],
            // Convert bytes to KB to MB.
            'maxFilesize' => parse_size($instance['settings']['max_filesize']) / 1024 / 1024,
            'acceptedFiles' => isset($extension_list) ? $extension_list : NULL,
          ),
          'resuming' => array('automaticRetry' => false),
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
