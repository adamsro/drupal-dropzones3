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
    '#element_validate' => array('dropzones3_upload_validate'),
    '#process' => array('dropzones3_element_process'),
    '#pre_render' => array('dropzones3_element_pre_render'),
    '#theme' => 'dropzones3',
    '#message' => t('Drop files here or click to upload.'),
    // The S3 directory relative to the bucket root where files are uploaded.
    '#upload_location' => !empty($config['root_folder']) ? $config['root_folder'] : '',
    '#upload_validators' => array(),
    '#preview_template' => '#dropzones3-item',
    '#extended' => FALSE,
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
 * Implements hook_field_widget_info().
 */
function dropzones3_field_widget_info() {
  return array(
    'dropzones3' => array(
      'label' => t('Dropzone S3'),
      'field types' => array('file', 'image'),
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

  // Load the items for form rebuilds from the field state as they might not be
  // in $form_state['values'] because of validation limitations. Also, they are
  // only passed in as $items when editing existing entities.
  $field_state = field_form_get_state($element['#field_parents'], $field['field_name'], $langcode, $form_state);
  if (isset($field_state['items'])) {
    $items = $field_state['items'];
  }
  // $element_info = element_info('dropzones3');
  $element = array(
    '#type' => 'dropzones3',
    '#upload_location' => $instance['settings']['file_directory'],
    '#upload_validators' => array(
      'file_validate_name_length' => array(),
      'file_validate_size' => array(parse_size($instance['settings']['max_filesize'])),
      'file_validate_extensions' => array($instance['settings']['file_extensions']),
    ),
    '#value_callback' => 'dropzones3_field_widget_value',
    '#ssencrypt' => $instance['settings']['ssencrypt'],
    // Allows this field to return an array instead of a single value.
    '#extended' => TRUE,
  );

  foreach ($items as $item) {
    $elements[$delta] = $element;
    $elements[$delta]['#default_value'] = $item;
    $elements[$delta]['#weight'] = $delta;
    $delta++;
  }
  // And then add one more empty row for new uploads except when this is a
  // programmed form as it is not necessary.
  if (($field['cardinality'] == FIELD_CARDINALITY_UNLIMITED || $delta < $field['cardinality']) && empty($form_state['programmed'])) {
    $elements[$delta] = $element;
    $elements[$delta]['#default_value'] = $defaults;
    $elements[$delta]['#weight'] = $delta;
    $elements[$delta]['#required'] = ($element['#required'] && $delta == 0);
  }
  $elements['#file_upload_delta'] = $delta;
  // $elements['#theme'] = 'file_widget_multiple';
  $elements['#description'] = theme('file_upload_help', array('description' => $element['#description'], 'upload_validators' => $element['#upload_validators']));

  return $elements;
}

/**
 * Validation callback for dropzones3 element type.
 */
function dropzones3_upload_validate(&$element, &$form_state) {
  // Consolidate the array value of this field to a single FID.
  if (!$element['#extended']) {
    form_set_value($element, $element['fid']['#value'], $form_state);
  }
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

  // TODO: I'm relatively sure this is useless, because of how we deal with the files.
  // But just in case, I'm going to leave it around for now.
  // Find the current value of this field from the form state, if it's there.
  $form_state_fid = $form_state['values'];
  foreach ($parents as $parent) {
    $form_state_fid = isset($form_state_fid[$parent]) ? $form_state_fid[$parent] : 0;
  }
  if ($element['#extended'] && isset($form_state_fid['fid'])) {
    $fid = $form_state_fid['fid'];
  }
  elseif (is_numeric($form_state_fid)) {
    $fid = $form_state_fid;
  }

  // If there's valid input, save the new upload.
  if (!empty($input) && !empty($input['key']) && $fid == 0 && !$remove_button_clicked) {
    $return = $input;

    // Construct a Drupal file object.
    $file = new stdClass();
    $file->uid = $user->uid;
    $file->filename = $input['filename'];
    $file->filesize = $input['filesize'];
    $file->filemime = file_get_mimetype($input['filename']);
    $file->uri = "s3://" . $input['key'];
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

    $wrapper = new S3fsStreamWrapper();
    $wrapper->writeUriToCache($file->uri);
  }

  if (empty($input) || $remove_button_clicked) {
    // If there is no input, or the remove button was just clicked, set the
    // default value.
    if ($element['#extended']) {
      $default_fid = isset($element['#default_value']['fid']) ? $element['#default_value']['fid'] : 0;
      $return = isset($element['#default_value']) ? $element['#default_value'] : array('fid' => 0);
    }
    else {
      $default_fid = isset($element['#default_value']) ? $element['#default_value'] : 0;
      $return = array('fid' => 0);
    }

    // Confirm that the file exists when used as a default value.
    if ($default_fid && $file = file_load($default_fid)) {
      $return['fid'] = $file->fid;
    }
    else {
      $return['fid'] = $fid;
    }
  }
  return $return;
}

/**
 * The #process callback for element.
 */
function dropzones3_element_process($element, &$form_state, $form) {
  $fid = isset($element['#value']['fid']) ? $element['#value']['fid'] : 0;
  $element['#file'] = $fid ? file_load($fid) : FALSE;
  $element['#tree'] = TRUE;
  // $parents_id = implode('_', $element['#parents']);

  dropzones3_add_template("dropzones3-item", theme('dropzones3-item'));

  // Set up the buttons first since we need to check if they were clicked.
  $element['upload_button'] = array(
    '#name' => implode('_', $element['#parents']) . '_upload_button',
    '#type' => 'submit',
    '#value' => t('Start Upload'),
    '#validate' => array(),
    '#submit' => array('file_field_widget_submit', 'dropzones3_element_submit'),
    '#limit_validation_errors' => array($element['#parents']),
    '#ajax' => TRUE,
    '#weight' => -5,
  );
  $element['remove_button'] = array(
    '#name' => "{$parents_id}_remove_button",
    '#type' => 'submit',
    '#value' => t('Remove'),
    '#validate' => array(),
    '#limit_validation_errors' => array($element['#parents']),
    '#attributes' => array('class' => array('cors-form-remove')),
    '#weight' => -5,
    '#submit' => array('s3fs_cors_remove_submit'),
  );

  $form_state['cache'] = TRUE;

  // If the #upload_location is set, we add a new key to the $form array that
  // stores this information in a way that is easier for us to look up in the
  // s3fs_cors_sign_request() function, where we don't know the element's
  // complete parentage. The data is stored as part of the $form array so we
  // can retrieve it server side but ensure that it's never exposed client side,
  // since doing so would allow users to upload files to an arbitrary directory
  // in the S3 bucket via local HTML modification.
  if (isset($element['#upload_location'])) {
    $form["#{$element['upload_button']['#name']}_location"] = $element['#upload_location'];
  }

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

  // Get element name without the last element since we'll add it in JS.
  $parents = $element['#parents'];
  $name = array_shift($parents);
  if (count($parents) > 1) {
    array_pop($parents);
    $name .= '[' . implode('][', $parents) . ']';
  }

  $element['#attached']['js'][] = array(
    'type' => 'setting',
    'data' => array(
      'dzs3' => array(
        '#' . $element['#id'] => array(
          // Drupal Specific used in notify function.
          'drupalElementParents' => $name,
          'drupalFormId' => $form['#id'],
          'drupalTriggeringName' => $element['upload_button']['#name'],
          'drupalTriggeringValue' => $element['upload_button']['#value'],

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

  // $element['upload_button']['#access'] = FALSE;
  $element['remove_button']['#access'] = FALSE;
  // hide($element['upload_button']);
  hide($element['remove_button']);

  return $element;
}

/**
 * Submit callback for the upload button on dropzones3 elements.
 */
function dropzones3_element_submit($form, &$form_state) {
  // Set the form to rebuild so that $form is correctly updated in response to
  // processing the file removal.
  $form_state['rebuild'] = TRUE;
}
