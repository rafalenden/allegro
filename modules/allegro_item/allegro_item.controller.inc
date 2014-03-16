<?php

/**
 * Controller for loading, creating, and saving Allegro items.
 */
class AllegroItemController extends DrupalDefaultEntityController {

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
    }
  }

  /**
   * Deletes multiple Allegro items by ID.
   *
   * @param array $ids
   *   An array of Allegro item IDs to delete.
   *
   * @return bool
   *   TRUE on success, FALSE otherwise.
   */
  public function delete($ids, DatabaseTransaction $transaction = NULL) {
    $allegro_items = $ids ? $this->load($ids) : FALSE;
    if (!$allegro_items) {
      // Do nothing, in case invalid or no ids have been passed.
      return;
    }

    if (!isset($transaction)) {
      $transaction = db_transaction();
    }

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

          $failed_item_link = l($failed_allegro_item->title, allegro_item_url($failed_allegro_item));
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

      if (!empty($result->finishItemsSucceed->item)) {
        // Delete Allegro item entity.
        db_delete('allegro_item')
          ->condition('id', $ids, 'IN')
          ->execute();

        // Delete Allegro field data.
        db_delete('allegro_field_data')
          ->condition('item_id', $ids, 'IN')
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
   * Create a default Allegro item.
   *
   * @param string $type
   *   The machine-readable type of the Allegro item.
   *
   * @return object
   *   An Allegro item object with all default fields initialized.
   */
  public function create($type = 'allegro_item') {
    return (object) array(
      'id' => '',
      'type' => $type,
      'title' => '',
    );
  }

}
