<?php

/**
 * Export Allegro items to Views.
 */

/**
 * Implements hook_views_data_alter().
 */
function allegro_item_views_data_alter(&$data) {
  $data['allegro_item']['operations'] = array(
    'field' => array(
      'title' => t('Operations links'),
      'help' => t('Display all the available operations links for the allegro item.'),
      'handler' => 'allegro_item_handler_field_item_operations',
    ),
  );
}
