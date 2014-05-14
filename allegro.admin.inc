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
    '#title' => t('Auction site'),
    '#description' => t('Auction site to connect with.'),
    '#options' => AllegroAPI::getSupportedWebsites(),
    '#default_value' => variable_get('allegro_country_code', 1),
    '#required' => TRUE,
  );

  $form['allegro_test_mode'] = array(
    '#type' => 'checkbox',
    '#title' => t('Turn on test mode'),
    '#description' => t('When test mode is active all operations are executed at <a href="@sandbox-url">WebAPI Sandbox</a> instead of production website.', array('@sandbox-url' => 'http://webapisandbox.pl/')),
    '#default_value' => variable_get('allegro_test_mode', FALSE),
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
  elseif (AllegroAPI::isPasswordEncrypted()) {
    // Encrypt password before saving.
    $values['allegro_password'] = AllegroAPI::hashPassword($values['allegro_password']);
  }

  // Test SOAP connection.
  $test_passed = allegro_test_connection(
    $values['allegro_login'], $values['allegro_password'],
    $values['allegro_webapikey'], $values['allegro_country_code'],
    $values['allegro_test_mode']);

  if ($test_passed) {
    drupal_set_message(t('Connected successfully to WebAPI service.'));
  }
  else {
    drupal_set_message(t('Could not connect to WebAPI service.'), 'error');
  }
}
