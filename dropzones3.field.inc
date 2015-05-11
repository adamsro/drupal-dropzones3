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
  $types['dropzones3'] = array(
    '#input' => TRUE,
    '#value_callback' => 'file_managed_file_value',
    '#element_validate' => array('file_managed_file_validate'),
    '#process' => array('dropzones3_element_process'),
    '#pre_render' => array('dropzones3_element_pre_render'),
    '#theme' => 'dropzones3',
    '#button_id' => NULL,
    '#message' => t('Drop files here or click to upload.'),
    '#progress_message' => NULL,
    '#upload_validators' => array(),
    '#extended' => TRUE,
    '#max_filesize' => '',
    '#autostart' => TRUE,
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

  if (isset($element['#upload_validators']['file_validate_extensions'][0])) {
    $extension_list = implode(',', array_filter(explode(' ', $element['#upload_validators']['file_validate_extensions'][0])));
  }
  $element['#button_id'] = $element['#id'] . '-button';

  $element['#attached']['js'][] = array(
    'type' => 'setting',
    'data' => array(
      'dzs3' => array(
        '#' . $element['#id'] => array(
          'extentions' => isset($extension_list) ? $extension_list : NULL,
          'autostart' => $element['#autostart'],
          'file_input' => $element['upload']['#name'],
          'max_size' => $element['#max_filesize'],
          'ajax_base' => '/dropzones3/ajax',
          'extra_params' => array(
            'form_build_id' => $form['form_build_id']['#value'],
          ),
          'signingEndpoint' => '/dropzones3/ajax/sign',
          's3' => array(
            'accesskey' => variable_get('dropzones3_aws_access_key'),
            'region' => variable_get('dropzones3_aws_region'),
            'bucket' => variable_get('dropzones3_aws_bucket'),
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
    $form['instance']['settings']['max_filesize']['#description'] = t("You may enter a value greater than the default maximum upload size above. It can exceed PHP's maximum post and file upload sizes as well.");
  }
}

/**
 * Implements hook_field_widget_settings_form().
 */
function dropzones3_field_widget_settings_form($field, $instance) {
  $widget = $instance['widget'];
  $settings = $widget['settings'];
  $form['autostart'] = array(
    '#type' => 'checkbox',
    '#title' => t('Start upload on files added'),
    '#default_value' => $settings['autostart'],
    '#description' => t('When checked, upload will start as soon as files are added without requiring to click <em>Upload</em>, unless some of the added files did not pass validation.'),
  );
  $form['awskey'] = array(
    '#type' => 'textfield',
    '#title' => t('Key for user which will upload to S3 bucket.'),
    '#default_value' => $settings['awskey'],
  );
  $form['awsbucketname'] = array(
    '#type' => 'textfield',
    '#title' => t('Name of the s3 bucket to be uplaoded to'),
    '#default_value' => $settings['awsbucketname'],
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

  // Essentially we use the managed_file type, extended with some enhancements.
  $element_info = element_info('dropzones3');
  $element += array(
    '#type' => 'dropzones3',
    '#upload_location' => file_field_widget_uri($field, $instance),
    '#upload_validators' => file_field_widget_upload_validators($field, $instance),
    // '#value_callback' => 'file_field_widget_value',
    // '#process' => array_merge($element_info['#process'], array('file_field_widget_process')),
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
    $elements = array($element);
  }
  return $elements;
}
