<?php

/**
 * Export Allegro items to Views.
 */

/**
 * Implements hook_views_data_alter().
 */
function allegro_item_views_data_alter(&$data) {
  foreach (array('id', 'title') as $field) {
    $data['allegro_item'][$field]['field']['handler'] = 'allegro_item_handler_field';
  }

  $data['allegro_item']['external_id']['field']['handler'] = 'allegro_item_handler_field_external_id';

  $data['allegro_item']['operations'] = array(
    'field' => array(
      'title' => t('Operations links'),
      'help' => t('Display all the available operations links for the allegro item.'),
      'handler' => 'allegro_item_handler_field_operations',
    ),
  );
}
