<?php

/**
 * @file
 * Allegro Category administration pages.
 */

/**
 * Settings page.
 */
function allegro_category_settings_form() {
  if (!allegro_is_configured_correctly()) {
    drupal_set_message(t('Allegro is not properly configured. Plese go to <a href="@config-url">configuration page</a> and set up correct login creditials.', array('@config-url' => url('admin/allegro/settings'))), 'error');
    return array();
  }

  $form = array();
  $form['update'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update data'),
  );
  $form['update']['download_categories'] = array(
    '#type' => 'submit',
    '#value' => t('Update categories'),
    '#submit' => array('allegro_category_download_batch'),
  );
  return $form;
}

/**
 * Menu callback: run a batch downloading Allegro categories.
 */
function allegro_category_download_batch() {
  $transaction = db_transaction();
  try {
    $batch = array(
      'title' => t('Downloading categories'),
      'finished' => '_allegro_category_download_batch_finished',
      'file' => drupal_get_path('module', 'allegro_category') . '/allegro_category.admin.inc',
    );
    $allegro = allegro_client();
    if (!is_object($allegro)) {
      throw new Exception('Error while connecting to Allegro.');
    }

    $limit = 1000;
    $response = $allegro->getCatsDataCount();
    $iterations = $response->catsCount / $limit;

    allegro_category_delete_by_country(variable_get('allegro_country_code'));

    for ($i = 0; $i <= $iterations; $i++) {
      $batch['operations'][] = array('allegro_category_download', array($i, $limit));
    }

    batch_set($batch);
    batch_process();
  }
  catch (SoapFault $e) {
    $transaction->rollback();
    drupal_set_message($e->faultstring, 'error');
    allegro_watchdog_exception('allegro_category', $e, $e->faultactor);
  }
}

/**
 * Performs post-processing for allegro_category_download_batch().
 *
 * @param bool $success
 *   A boolean indicating whether the re-build process has completed.
 * @param array $results
 *   An array of results information.
 * @param array $operations
 *   An array of function calls (not used in this function).
 */
function _allegro_category_download_batch_finished($success, $results = array(), $operations = array()) {
  if ($success) {
    drupal_set_message(t('The categories have been downloaded.'));
    watchdog('allegro_category', 'Categories was updated.');
    variable_set('allegro_category_last_update', time());
  }
  else {
    drupal_set_message(t('The categories have not been properly downloaded.'), 'error');
    watchdog('allegro_category', 'The categories have not been properly downloaded.');
  }
}
