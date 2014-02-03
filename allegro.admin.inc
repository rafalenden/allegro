<?php

/**
 * @file
 * Allegro administration pages.
 */

/**
 * Settings page.
 */
function allegro_settings_form() {
  $form = array();
  $form['allegro_login'] = array(
    '#type' => 'textfield',
    '#title' => t('Login'),
    '#description' => t('Your Allegro account login.'),
    '#default_value' => variable_get('allegro_login'),
    '#required' => TRUE,
  );
  $form['allegro_password'] = array(
    '#type' => 'password',
    '#title' => t('Password'),
    '#description' => t('Your Allegro account password.'),
    '#default_value' => variable_get('allegro_password'),
  );
  if (variable_get('allegro_password')) {
    $form['allegro_password']['#description'] = t("Your Allegro account password. Leave blank if you don't wish to change it.");
  }
  $form['allegro_webapikey'] = array(
    '#type' => 'textfield',
    '#title' => t('WebAPI Key'),
    '#description' => t('Allegro WebAPI service key. !url.', array('!url' => l(t('Get API key'), 'http://allegro.pl/myaccount/webapi.php'))),
    '#default_value' => variable_get('allegro_webapikey'),
    '#required' => TRUE,
  );
  $form['allegro_country_code'] = array(
    '#type' => 'select',
    '#title' => t('Allegro site'),
    '#description' => t('Allegro site to connect with.'),
    '#options' => Allegro::getCountries(),
    '#default_value' => variable_get('allegro_country_code', 1),
    '#required' => TRUE,
  );

  $form['#submit'][] = 'allegro_settings_form_submit';

  return system_settings_form($form);
}

/**
 * Settings page submit callback.
 */
function allegro_settings_form_submit($form, &$form_state) {
  $values = &$form_state['values'];

  if (empty($values['allegro_password'])) {
    $values['allegro_password'] = variable_get('allegro_password');
  }
  elseif (Allegro::passwordEncrypted()) {
    // Encrypt password before saving.
    $values['allegro_password'] = base64_encode(hash('sha256', $values['allegro_password'], true));
  }

  // Test SOAP connection.
  $test_passed = allegro_test_connection(
    $values['allegro_login'], $values['allegro_password'],
    $values['allegro_webapikey'], $values['allegro_country_code']);

  if ($test_passed) {
    drupal_set_message(t('Connected successfully to WebAPI service.'));
  }
  else {
    drupal_set_message(t('Could not connect to WebAPI service.'), 'error');
  }
}
