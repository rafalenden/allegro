<?php

/**
 * Controller for loading, creating, and saving Allegro items.
 */
class AllegroItemController extends DrupalDefaultEntityController {

  /**
   * Save Allegro item.
   *
   * @global object $user
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

      // Give modules the opportunity to prepare field data for saving.
      field_attach_presave('allegro_item', $allegro_item);

      // If this is a new Allegro item...
      if ($allegro_item->is_new) {
        // Save the new Allegro item.
        drupal_write_record('allegro_item', $allegro_item);
        $op = 'insert';
      }
      else {
        // Save the updated Allegro item.
        drupal_write_record('allegro_item', $allegro_item, 'id');
        $op = 'update';
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
      allegro_watchdog_exception('allegro_item', $e, NULL, WATCHDOG_ERROR);
      return FALSE;
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
  public function delete($ids) {
    if (!empty($ids)) {
      $transaction = db_transaction();
      $allegro_items = $this->load($ids, array());
      try {
        db_delete('allegro_item')
         ->condition('id', $ids, 'IN')
         ->execute();

        foreach ($allegro_items as $allegro_item) {
          field_attach_delete('allegro_item', $allegro_item);
        }

        // Ignore slave server temporarily to give time for the
        // saved Allegro item to be propagated to the slave.
        db_ignore_slave();
      }
      catch (Exception $e) {
        $transaction->rollback();
        allegro_watchdog_exception('allegro_item', $e, NULL, WATCHDOG_ERROR);
        return FALSE;
      }

      module_invoke_all('entity_delete', $allegro_item, 'allegro_item');

      // Clear the page and block and Allegro item caches.
      cache_clear_all();
      $this->resetCache();
    }

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
