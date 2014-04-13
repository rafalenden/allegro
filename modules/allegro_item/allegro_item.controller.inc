<?php

/**
 * Controller for loading, creating, and saving Allegro items.
 */
class AllegroItemController extends EntityAPIController {

  /**
   * Load Allegro item.
   * @param array $ids
   *   IDs of Allegro items to load.
   * @param array $conditions
   *
   * @return array
   *   Array of AllegroItem objects.
   */
  public function load($ids = array(), $conditions = array()) {
    $allegro_items = parent::load($ids, $conditions);

    $result = db_select('allegro_field_data', 'fd')
      ->condition('fd.item_id', $ids)
      ->fields('fd', array('item_id', 'fid', 'value'))
      ->orderBy('fd.fid')
      ->execute()->fetchAll();

    foreach ($allegro_items as &$allegro_item) {
      $allegro_item->is_new = FALSE;
    }

    return $allegro_items;
  }

  /**
   * Attaches data to Allegro item entities upon loading.
   *
   * @param $allegro_items
   *   Associative array of query results, keyed on the entity ID.
   * @param $revision_id
   *   ID of the revision that was loaded, or FALSE if the most current revision
   *   was loaded.
   */
  protected function attachLoad(&$allegro_items, $revision_id = FALSE) {
    $result = db_select('allegro_field_data', 'fd')
      ->condition('fd.item_id', array_keys($allegro_items))
      ->fields('fd', array('item_id', 'fid', 'value'))
      ->orderBy('fd.fid')
      ->execute();

    foreach ($result as $row) {
      $allegro_items[$row->item_id]->field_data[$row->fid] = $row->value;
    }

    parent::attachLoad($allegro_items, $revision_id);
  }

  /**
   * Save Allegro item.
   *
   * @param object $allegro_item
   *
   * @return mixed
   *   Allegro item object on success, FALSE otherwise.
   */
  public function save($allegro_item) {
    $transaction = db_transaction();
    try {
      global $user;

      // Determine if we will be inserting a new Allegro item.
      $allegro_item->is_new = empty($allegro_item->id);

      // Set the timestamp fields.
      if (empty($allegro_item->created)) {
        $allegro_item->created = REQUEST_TIME;
      }
      $allegro_item->changed = REQUEST_TIME;

      if (empty($allegro_item->country_code)) {
        $allegro_item->country_code = variable_get('allegro_country_code', 1);
      }

      // Set the Allegro_item uid if it's being created at this time.
      if (empty($allegro_item->uid)) {
        global $user;
        $allegro_item->uid = $user->uid;
      }

      // Give modules the opportunity to prepare field data for saving.
      field_attach_presave('allegro_item', $allegro_item);

      $allegro = allegro_client();

      // If this is a new Allegro item...
      if ($allegro_item->is_new) {
        $local_id = uniqid();

        if (empty($allegro_item->sellformfields_prepared) && !empty($allegro_item->sellformfields)) {
          $allegro_item->sellformfields_prepared = allegro_field_prepare_multiple($allegro_item->sellformfields);
        }

        $new_item = $allegro->newAuction($allegro_item->sellformfields_prepared, $local_id);
        $verified_item = $allegro->verifyItem($local_id);

        if ($new_item->itemId != $verified_item->itemId) {
          throw new Exception(t('Unknown error occurred while validating item.'));
        }

        $allegro_item->external_id = (int) $verified_item->itemId;

        // Save the new Allegro item.
        drupal_write_record('allegro_item', $allegro_item);
        $op = 'insert';

        // Save values of fields.
        $query = db_insert('allegro_field_data')->fields(array('item_id', 'fid', 'value'));
        foreach ($allegro_item->sellformfields as $fid => $value) {
          $query->values(
            array(
              'item_id' => $allegro_item->id,
              'fid' => $fid,
              'value' => $value,
            )
          );
        }
        $query->execute();
      }
      else {
        $changed_item = $allegro->changeItemFields($allegro_item->external_id, $allegro_item->sellformfields_prepared);

        // Save the updated Allegro item.
        drupal_write_record('allegro_item', $allegro_item, 'id');
        $op = 'update';

        // Update values of fields.
        foreach ($allegro_item->fields_to_update as $fid => $value) {
          db_update('allegro_field_data')
            ->fields(array('value' => $value))
            ->condition('item_id', $allegro_item->id)
            ->condition('fid', $fid)
            ->execute();
        }
      }

      // Save fields.
      $function = 'field_attach_' . $op;
      $function('allegro_item', $allegro_item);

      module_invoke_all('entity_' . $op, $allegro_item, 'allegro_item');

      // Clear internal properties.
      unset($allegro_item->is_new);

      // Ignore slave server temporarily to give time for the saved order to be
      // propagated to the slave.
      db_ignore_slave();

      return $allegro_item;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('allegro_item', $e, $e->getMessage(), WATCHDOG_ERROR);
      throw $e;
    }
  }

  /**
   * Finishes multiple Allegro items by ID.
   *
   * @param array $ids
   *   An array of Allegro item IDs to delete.
   *
   * @return bool
   *   TRUE on success, FALSE otherwise.
   */
  public function finish(array $ids) {
    $allegro_items = $ids ? $this->load($ids) : FALSE;
    if (!$allegro_items) {
      // Do nothing, in case invalid or no ids have been passed.
      return FALSE;
    }

    $transaction = db_transaction();
    try {
      $allegro = allegro_client();

      $items_to_finish = array();
      $external_ids = array();
      foreach ($allegro_items as $allegro_item) {
        $external_ids[$allegro_item->external_id] = $allegro_item->id;
        $items_to_finish[] = array('finishItemId' => $allegro_item->external_id);
      }

      // Finish listing an offers.
      $result = $allegro->finishItems($items_to_finish);

      // Error occurred while finishing some auctions.
      if (!empty($result->finishItemsFailed->item)) {
        $failed_items = array();
        foreach ($result->finishItemsFailed->item as $failed_item) {
          $failed_allegro_item_id = $external_ids[$failed_item->finishItemId];
          $failed_allegro_item = $allegro_items[$failed_allegro_item_id];

          $failed_item_link = l($failed_allegro_item->title, $failed_allegro_item->url());
          $failed_items[] = $failed_item_link . ': ' . $failed_item->finishErrorMessage;

          $ids_key_to_unset = array_search($failed_allegro_item_id, $ids);
          unset($ids[$ids_key_to_unset]);
        }

        $failed_items_list = array(
          'title' => t('Items that was not finished'),
          'items' => $failed_items,
        );
        drupal_set_message(theme('item_list', $failed_items_list), 'error');
      }

      if (!empty($result->finishItemsSucceed->item)) {
        // Set status of ALlegro item as inactive.
        $num_finished = db_update('allegro_item')
          ->fields(array('status' => 0))
          ->condition('id', $ids)
          ->execute();

        // Ignore slave server temporarily.
        db_ignore_slave();
      }
      else {
        throw new Exception(t('No items was finished.'));
      }
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('allegro_item', $e, NULL, WATCHDOG_ERROR);
      return FALSE;
    }

    return TRUE;
  }

  /**
   * Deletes multiple Allegro items by ID.
   *
   * @param array $ids
   *   An array of Allegro item IDs to delete.
   * @param array $ids
   *   Finish selling of items while deleting them.
   *
   * @return bool
   *   TRUE on success, FALSE otherwise.
   */
  public function delete($ids, DatabaseTransaction $transaction = NULL, $finish_auctions = TRUE) {
    $allegro_items = $ids ? $this->load($ids) : FALSE;
    if (!$allegro_items) {
      // Do nothing, in case invalid or no ids have been passed.
      return FALSE;
    }

    if (!isset($transaction)) {
      $transaction = db_transaction();
    }

    try {
      if ($finish_auctions) {
        $allegro = allegro_client();

        $items_to_finish = array();
        $external_ids = array();
        foreach ($allegro_items as $allegro_item) {
          $external_ids[$allegro_item->external_id] = $allegro_item->id;
          $items_to_finish[] = array('finishItemId' => $allegro_item->external_id);
        }
        // Finish listing an offers.
        $result = $allegro->finishItems($items_to_finish);

        // Error occurred while finishing some auctions.
        if (!empty($result->finishItemsFailed->item)) {
          $failed_items = array();
          foreach ($result->finishItemsFailed->item as $failed_item) {
            $failed_allegro_item_id = $external_ids[$failed_item->finishItemId];
            $failed_allegro_item = $allegro_items[$failed_allegro_item_id];

            $failed_item_link = l($failed_allegro_item->title, $failed_allegro_item->url());
            $failed_items[] = $failed_item_link . ': ' . $failed_item->finishErrorMessage;

            $ids_key_to_unset = array_search($failed_allegro_item_id, $ids);
            unset($ids[$ids_key_to_unset]);
          }

          $failed_items_list = array(
            'title' => t('Items that was not deleted'),
            'items' => $failed_items,
          );
          drupal_set_message(theme('item_list', $failed_items_list), 'error');
        }
      }

      if (!$finish_auctions || !empty($result->finishItemsSucceed->item)) {
        // Delete Allegro item entity.
        db_delete('allegro_item')
          ->condition('id', $ids)
          ->execute();

        // Delete Allegro field data.
        db_delete('allegro_field_data')
          ->condition('item_id', $ids)
          ->execute();

        // Delete Field API data.
        foreach ($allegro_items as $allegro_item) {
          field_attach_delete('allegro_item', $allegro_item);
        }

        // Ignore slave server temporarily.
        db_ignore_slave();
      }
      else {
        throw new Exception(t('No items was deleted.'));
      }
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('allegro_item', $e, NULL, WATCHDOG_ERROR);
      return FALSE;
    }

    module_invoke_all('entity_delete', $allegro_item, 'allegro_item');

    // Clear the page and block and Allegro item caches.
    cache_clear_all();
    $this->resetCache();

    return TRUE;
  }

  /**
   * Pre-populating the built content.
   */
  public function buildContent($allegro_item, $view_mode = 'full', $langcode = NULL, $content = array()) {
    $content['item_id'] = array(
      '#type' => 'item',
      '#title' => t('Item number'),
      '#markup' => $allegro_item->external_id,
    );
    $content['created'] = array(
      '#type' => 'item',
      '#title' => t('Created date'),
      '#markup' => format_date($allegro_item->created),
    );
    $content['external_url'] = array(
      '#type' => 'item',
      '#title' => t('External URL to Allegro item'),
      '#markup' => l($allegro_item->externalURL(), $allegro_item->externalURL()),
    );

    return parent::buildContent($allegro_item, $view_mode, $langcode, $content);
  }

}
