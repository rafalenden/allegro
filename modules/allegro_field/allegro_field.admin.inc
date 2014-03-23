<?php

/**
 * @file
 * Allegro Field administration pages.
 */

/**
 * Settings page.
 */
function allegro_field_settings_form() {
  $form = array();
  $form['update'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update data'),
  );

  if (!variable_get('allegro_field_last_update')) {
    $message = t("Fields wasn't downloaded yet.");
  }
  else {
    $total_fields = db_query('SELECT COUNT(*) FROM {allegro_field} WHERE country_code = :country_code', array(':country_code' => variable_get('allegro_country_code')))->fetchField();
    $message = t('There is @count downloaded fields.', array('@count' => $total_fields));
  }

  $form['update']['status'] = array(
    '#markup' => '<p>' . $message . '</p>',
  );

  $form['update']['download_fields'] = array(
    '#type' => 'submit',
    '#value' => t('Update fields'),
    '#submit' => array('allegro_field_download_batch'),
  );
  return $form;
}

/**
 * Menu callback: run a batch downloading Allegro fields.
 */
function allegro_field_download_batch() {
  // Check connection with WebAPI.
  if (!allegro_test_connection()) {
    drupal_set_message(t('Allegro module is not properly configured. Please go to <a href="@config-url">configuration page</a> and set up correct login credentials.', array('@config-url' => url('admin/allegro/settings'))), 'error');
    return array();
  }

  $transaction = db_transaction();
  try {
    $batch = array(
      'title' => t('Downloading fields'),
      'finished' => '_allegro_field_download_batch_finished',
      'file' => drupal_get_path('module', 'allegro_field') . '/allegro_field.admin.inc',
    );

    $allegro = allegro_client();

    $limit = 1000;
    $count = $allegro->getSellFormFieldsCount();
    $iterations = $count / $limit;

    allegro_field_delete_by_country(variable_get('allegro_country_code'));

    for ($i = 0; $i <= $iterations; $i++) {
      $batch['operations'][] = array('allegro_field_download', array($i, $limit));
    }

    batch_set($batch);
    batch_process();
  }
  catch (SoapFault $e) {
    $transaction->rollback();
    drupal_set_message($e->faultstring, 'error');
    watchdog_exception('allegro_field', $e, $e->faultactor);
  }
}

/**
 * Performs post-processing for allegro_field_download_batch().
 *
 * @param bool $success
 *   A boolean indicating whether the re-build process has completed.
 * @param array $results
 *   An array of results information.
 * @param array $operations
 *   An array of function calls (not used in this function).
 */
function _allegro_field_download_batch_finished($success, $results = array(), $operations = array()) {
  if ($success) {
    drupal_set_message(t('The fields have been downloaded.'));
    watchdog('allegro_field', 'Fields was updated.');
    variable_set('allegro_field_last_update', time());
  }
  else {
    drupal_set_message(t('The fields have not been properly downloaded.'), 'error');
    watchdog('allegro_field', 'The fields have not been properly downloaded.');
  }
}
