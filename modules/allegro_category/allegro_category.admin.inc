<?php

/**
 * @file
 * Allegro Category administration pages.
 */

/**
 * Settings page.
 */
function allegro_category_settings_form() {
  $form = array();
  $form['update'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update data'),
  );

  if (!variable_get('allegro_category_last_update')) {
    $message = t("Categories wasn't downloaded yet.");
  }
  else {
    $total_categories = db_query('SELECT COUNT(*) FROM {allegro_category}')->fetchField();
    $message = format_plural($total_categories, 'There is 1 downloaded category.', 'There is @count downloaded categories.');
  }

  $form['update']['status'] = array(
    '#markup' => '<p>' . $message . '</p>',
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
  // Check connection with WebAPI.
  if (!allegro_test_connection()) {
    drupal_set_message(t('Allegro module is not properly configured. Please go to <a href="@config-url">configuration page</a> and set up correct login creditials.', array('@config-url' => url('admin/allegro/settings'))), 'error');
    return array();
  }

  $transaction = db_transaction();
  try {
    $batch = array(
      'title' => t('Downloading categories'),
      'finished' => '_allegro_category_download_batch_finished',
      'file' => drupal_get_path('module', 'allegro_category') . '/allegro_category.admin.inc',
    );
    $allegro = allegro_client();
    if (!is_object($allegro)) {
      throw new Exception('Error occurred while connecting to Allegro WebAPI.');
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
    watchdog_exception('allegro_category', $e, $e->faultactor);
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
    variable_set('allegro_category_last_update', REQUEST_TIME);
  }
  else {
    drupal_set_message(t('The categories have not been properly downloaded.'), 'error');
    watchdog('allegro_category', 'The categories have not been properly downloaded.');
  }
}
