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
    '#default_value' => ($access_key = !empty(variable_get('dropzones3_aws_access_key')) ? variable_get('dropzones3_aws_access_key') : ''),
    '#required'       => TRUE,
  );
  $form['dropzones3_aws_secret_key'] = array(
    '#type' => 'textfield',
    '#title' => t('AWS Secret Key'),
    '#default_value' => ($secret_key = !empty(variable_get('dropzones3_aws_secret_key')) ? variable_get('dropzones3_aws_secret_key') : ''),
    '#required'       => TRUE,
  );
  $form['dropzones3_aws_bucket'] = array(
    '#type'           => 'textfield',
    '#title'          => t('S3 Bucket Name'),
    '#default_value'  => !empty(variable_get('dropzones3_aws_bucket')) ? variable_get('dropzones3_aws_bucket') : '',
    '#required'       => TRUE,
  );
  $form['dropzones3_aws_region'] = array(
    '#type'          => 'select',
    '#options'       => $region_map,
    '#title'         => t('S3 Region'),
    '#description'   => t(
      'The region in which your bucket resides. Be careful to specify this accurately,
      as you are likely to see strange or broken behavior if the region is set wrong.'
    ),
    '#default_value' => !empty(variable_get('dropzones3_aws_region')) ? variable_get('dropzones3_aws_region') : '',
    '#required'       => TRUE,
  );
  return system_settings_form($form);
}
