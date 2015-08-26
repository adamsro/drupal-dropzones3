<?php
/**
 * @file
 * Global configuration for DropzoneS3; add AWS Credentials for limited user.
 */

function dropzones3_admin_form($form, &$form_state) {
  $form = array();

  $region_map = array(
    '' => 'Default',
    'us-east-1' => 'US Standard (us-east-1)',
    'us-west-1' => 'US West - Northern California  (us-west-1)',
    'us-west-2' => 'US West - Oregon (us-west-2)',
    'eu-west-1' => 'EU - Ireland  (eu-west-1)',
    'eu-central-1' => 'EU - Frankfurt (eu-central-1)',
    'ap-southeast-1' => 'Asia Pacific - Singapore (ap-southeast-1)',
    'ap-southeast-2' => 'Asia Pacific - Sydney (ap-southeast-2)',
    'ap-northeast-1' => 'Asia Pacific - Tokyo (ap-northeast-1)',
    'sa-east-1' => 'South America - Sao Paulo (sa-east-1)',
  );

  $form['dropzones3_aws_access_key'] = array(
    '#type' => 'textfield',
    '#title' => t('AWS Access Key'),
    '#default_value' => !empty(variable_get('dropzones3_aws_access_key')) ? variable_get('dropzones3_aws_access_key') : '',
    '#description' => t('Access key for limited user which will interact with the bucket via CORS.'),
    '#required'       => TRUE,
  );
  $form['dropzones3_aws_secret_key'] = array(
    '#type' => 'textfield',
    '#title' => t('AWS Secret Key'),
    '#default_value' => !empty(variable_get('dropzones3_aws_secret_key')) ? variable_get('dropzones3_aws_secret_key') : '',
    '#description' => t('Secret key used for signing requests. Key will not be accessable.'),
    '#required'       => TRUE,
  );
  $form['dropzones3_cors_origin'] = array(
    '#type'          => 'textfield',
    '#title'         => t('CORS Origin'),
    '#description'   => t(
      'Please enter the URL from which your users access this website, e.g. <i>https://www.example.com</i>.
      You may optionally specifiy up to one wildcard, e.g. <i>*.example.com</i>.<br>
      Upon submitting this form, if this field is filled, your S3 bucket will be configured to allow CORS
      requests from the specified origin. If the field is empty, your bucket\'s CORS config will be deleted.'),
    '#default_value' => !empty(variable_get('dropzones3_cors_origin')) ? variable_get('dropzones3_cors_origin') : '',
  );
  $form = system_settings_form($form);
  $form['#submit'][] = 'dropzones3_cors_config_submit';
  return $form;
}

 /**
 * Enables CORS on the configured bucket when the form is submitted.
 */
function dropzones3_cors_config_submit($form, &$form_state) {
  $config = _s3fs_get_config();
  $s3 = _s3fs_get_amazons3_client($config);
  if (!empty($form_state['values']['dropzones3_cors_origin'])) {

    $allowed_origins = preg_split("/[\s,]+/", $form_state['values']['dropzones3_cors_origin']);
    foreach ($allowed_origins as &$origin) {
      $origin = trim($origin);
    }
    $allowed_origins = array_filter($allowed_origins);

    $s3->putBucketCors(array(
      'Bucket' => $config['bucket'],
      'CORSRules' => array(
        array(
          'AllowedHeaders' => array('*'),
          'ExposeHeaders' => array('x-amz-version-id'),
          'AllowedMethods' => array('GET','POST','PUT'),
          'MaxAgeSeconds' => 3000,
          'AllowedOrigins' => $allowed_origins,
        ),
      ),
    ));
  }
  else {
    // If $form_state['values']['s3fs_cors_origin'] is empty, that means we
    // need to delete their bucket's CORS config.
    $s3->deleteBucketCors(array(
      'Bucket' => $config['bucket'],
    ));
  }
}
