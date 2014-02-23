<?php

/**
 * @file
 * Allegro Item administration pages.
 */

/**
 * Menu callback; Displays a listing of Allegri items.
 *
 * @return string
 *   Items as HTML unordered list.
 */
function allegro_item_list() {
  $query = new EntityFieldQuery();
  $query
   ->entityCondition('entity_type', 'allegro_item')
   ->propertyOrderBy('created', 'DESC')
   ->range(0, 5);
  $result = $query->execute();

  if (empty($result['allegro_item'])) {
    return t('No Allegro items added.');
  }

  $allegro_items = allegro_item_load_multiple(array_keys($result['allegro_item']));

  foreach ($allegro_items as $allegro_item) {
    $items[] = l($allegro_item->title, allegro_item_url($allegro_item));
  }

  return theme('item_list', array('items' => $items));
}

/**
 * Present an Allegro item submission form.
 */
function allegro_item_add() {
  $allegro_item = entity_get_controller('allegro_item')->create('allegro_item');
  drupal_set_title(t('Create Allegro item'), PASS_THROUGH);
  return drupal_get_form('allegro_item_form', $allegro_item);
}

/**
 * Menu callback; presents the Allegro item editing form, or redirects to delete confirmation.
 *
 * @param object $allegro_item
 *   The Allegro item object to edit.
 */
function allegro_item_page_edit($allegro_item) {
  drupal_set_title(t('<em>Edit Allegro item</em> @title', array('@title' => $allegro_item->title)), PASS_THROUGH);

  return drupal_get_form('allegro_item_form', $allegro_item);
}

/**
 * Form builder. Displays the Allegro item add/edit form.
 *
 * @param array $form
 * @param array $form_state
 * @param object $allegro_item
 *   The Allegro item object to edit, which may be brand new.
 */
function allegro_item_form($form, &$form_state, $allegro_item) {
  if (!variable_get('allegro_field_last_update')) {
    drupal_set_message(t('Field information is not downloaded from Allegro. <a href="@fields-url">Download field information</a>.', array('@fields-url' => url('admin/allegro/settings/fields'))), 'error');
  }
  if (!variable_get('allegro_category_last_update')) {
    drupal_set_message(t('Categories are not downloaded from Allegro. <a href="@fields-url">Download categories</a>.', array('@fields-url' => url('admin/allegro/settings/categories'))), 'error');
  }
  if (!variable_get('allegro_field_last_update') || !variable_get('allegro_category_last_update')) {
    return array();
  }

  $module_path = drupal_get_path('module', 'allegro_item');
  $form['#attached']['css'][] =  $module_path . '/allegro_item.css';

  // Set the id and identify this as an Allegro item edit form.
  $form['#id'] = 'allegro-item-form';

  // Save the Allegro item for later, in case we need it.
  $form['#allegro_item'] = $allegro_item;
  $form_state['allegro_item'] = $allegro_item;

  $sellformfields = allegro_field_load_multiple(allegro_get_fields());

  $form['item'] = array(
    '#title' => t('Item details'),
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  $form['item'][1] = array(
    '#type' => 'textfield',
    '#description' => NULL,
    '#default_value' => $allegro_item->title,
  ) + allegro_field_convert($sellformfields[1]);

  $form['sellformfields'] = array(
    '#type' => 'value',
    '#value' => $sellformfields,
  );

  // Get current category ID.
  if (isset($form_state['triggering_element']['#value']) && $form_state['triggering_element']['#type'] == 'select') {
    $form_state['current_category'] = $form_state['triggering_element']['#value'];
  }
  elseif (isset($form_state['values'])) {
    $category_levels = preg_grep('/^level[\d]+$/', array_keys($form_state['values']));
    $last_level = end($category_levels);
    if (!empty($form_state['values'][$last_level])) {
      $form_state['current_category'] = $form_state['values'][$last_level];
    }
  }
  elseif (!isset($form_state['current_category'])) {
    $form_state['current_category'] = 0;
  }

  $form['item']['category'] = array(
    '#prefix' => '<div id="allegro-item-category-wrapper">',
    '#type' => 'fieldset',
    '#title' => t('Category'),
    '#suffix' => '</div>',
  );

  // Get path of selected categories.
  $lineage = allegro_category_hierarchy_select_lineage($form_state['current_category']);
  if ($form_state['current_category'] != 0) {
    $lineage[] = 0;
  }
  sort($lineage);

  $has_children = TRUE;

  foreach ($lineage as $level => $cid) {
    $children = allegro_category_hierarchy_select_children($cid);
    if (!$children) {
      $has_children = FALSE;
      break;
    }
    $form['item']['category']['level' . $level] = array(
      '#type' => 'select',
      '#size' => 10,
      '#options' => $children,
      '#prefix' => '<div class="category-level">',
      '#suffix' => '</div>',
      '#ajax' => array(
        'callback' => 'allegro_item_category_ajax_callback',
        'wrapper' => 'allegro-item-category-wrapper',
        'progress' => array('type' => 'none'),
      ),
    );
  }

  // Load fields for chosen category.
  if (!$has_children) {
    // Don't include global fields (cid = 0).
    unset($lineage[0]);
    $category_sellformfields = allegro_field_load_for_multiple_categories($lineage);

    if (count($category_sellformfields) > 0) {
      $form['item']['category']['extra_fields'] = array(
        '#type' => 'fieldset',
        '#title' => t('Item parameters'),
      );
    }

    foreach ($category_sellformfields as $sellformfield) {
      $form['item']['category']['extra_fields'][$sellformfield->fid] = array(
        '#description' => NULL,
        '#attributes' => array('class' => array('category-extra-field')),
        '#weight' => $sellformfield->pos,
      ) + allegro_field_convert($sellformfield);
    }
  }

  // Gallery.

  if (!isset($form_state['gallery_count'])) {
    $form_state['gallery_count'] = 1;
  }

  $form['gallery'] = array(
    '#type' => 'fieldset',
    '#title' => t('Gallery'),
    '#prefix' => '<div id="gallery-fieldset-wrapper">',
    '#suffix' => '</div>',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  for ($i = 16; $i <= ($form_state['gallery_count'] + 15); $i++) {
    $form['gallery']['images'][$i] = allegro_field_convert($sellformfields[$i]);
  }

  if ($form_state['gallery_count'] < 8) {
    $form['gallery']['add_image'] = array(
      '#type' => 'submit',
      '#value' => t('Add image'),
      '#limit_validation_errors' => array(),
      '#submit' => array('allegro_item_gallery_add_image'),
      '#ajax' => array(
        'callback' => 'allegro_item_gallery_callback',
        'wrapper' => 'gallery-fieldset-wrapper',
      ),
    );
  }

  if ($form_state['gallery_count'] > 1) {
    $form['gallery']['remove_image'] = array(
      '#type' => 'submit',
      '#value' => t('Remove last image'),
      '#limit_validation_errors' => array(),
      '#submit' => array('allegro_item_gallery_remove_image'),
      '#ajax' => array(
        'callback' => 'allegro_item_gallery_callback',
        'wrapper' => 'gallery-fieldset-wrapper',
      ),
    );
  }

  $form['offer'] = array(
    '#title' => t('Offer details'),
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  // Quantity.
  $form['offer'][5] = array(
    '#title' => t('Quantity'),
    '#description' => '',
  ) + allegro_field_convert($sellformfields[5]);

  $form['offer'][28] = array(
    '#title' => '',
  ) + allegro_field_convert($sellformfields[28]);

  // Buy now price.
  $form['offer'][8] = array(
    '#field_suffix' => t('zł'),
    '#description' => '',
    '#required' => TRUE,
  ) + allegro_field_convert($sellformfields[8]);

  // Duration.
  $form['offer'][4] = array(
    '#field_suffix' => t('days'),
  ) + allegro_field_convert($sellformfields[4]);
  unset($form['offer'][4]['#options'][5]);

  // Additional options (Promotion options).
  $form['offer'][15] = array(
    '#description' => NULL,
  ) + allegro_field_convert($sellformfields[15]);
  unset($form['offer'][15]['#options'][64]);

  $form['payment_shipment'] = array(
    '#title' => t('Payment and shipment'),
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  $form['payment_shipment']['prepayment'] = array(
    '#title' => t('Payment in advance'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  // Jeśli w jednej dostawie wypełnisz wszystkie 3 pola musisz tak samo postąpić z pozostałymi dostawami.

  $prepayment = array(36, 38, 37, 39, 41, 43, 44, 46, 47, 50, 51);

  $form['payment_shipment']['prepayment']['costs'] = array(
    '#type' => 'tableform',
    '#header' => array(
      t('Shipping method'),
      t('First item'),
      t('Next item'),
      t('Quantity in pack'),
    ),
    '#options' => allegro_field_convert_shipping_fields_table($prepayment, $sellformfields),
  );

  $form['payment_shipment']['cod'] = array(
    '#title' => t('Cash on delivery'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $cod = array(40, 42, 45, 48, 49, 52);

  $form['payment_shipment']['cod']['costs'] = array(
    '#type' => 'tableform',
    '#header' => array(
      t('Shipping method'),
      t('First item'),
      t('Next item'),
      t('Quantity in pack'),
    ),
    '#options' => allegro_field_convert_shipping_fields_table($cod, $sellformfields),
  );

  // Free shipping methods.
  $form['payment_shipment'][35] = allegro_field_convert($sellformfields[35]);
  $form['payment_shipment'][35]['#options'][1] .= ' (' . t('cash on delivery') . ')';

  // Shipping delay.
  if (isset($sellformfields[340])) {
    $form['payment_shipment'][340] = allegro_field_convert($sellformfields[340]);
    $form['payment_shipment'][340]['#options'][1] = 'immediately';
    foreach ($form['payment_shipment'][340]['#options'] as $key => $value) {
      $days = $value / 24;
      if ($key > 1) {
        $label = format_plural($days, t('1 day'), t('@days days'), array('@days' => $days));
        $form['payment_shipment'][340]['#options'][$key] = $label;
      }
    }
  }

  // Who pay for shipping.
  $form['payment_shipment'][12] = allegro_field_convert($sellformfields[12]);

  // Shipping options.
  $form['payment_shipment'][13] = array(
    '#description' => NULL,
  ) + allegro_field_convert($sellformfields[13]);
  unset($form['payment_shipment'][13]['#options'][16]);

  // Payment types.
  $form['payment_shipment'][14] = array(
    '#title' => t('Payment details'),
    '#description' => NULL,
  ) + allegro_field_convert($sellformfields[14]);
  // Details in item description.
  unset($form['payment_shipment'][14]['#options'][16]);

  // First bank account.
  $form['payment_shipment'][33] = array(
    '#states' => array(
      'visible' => array(':input[id="edit-14-1"]' => array('checked' => TRUE)),
    ),
  ) + allegro_field_convert($sellformfields[33]);

  // First bank account.
  $form['payment_shipment'][34] = array(
    '#states' => array(
      'visible' => array(':input[id="edit-14-1"]' => array('checked' => TRUE)),
    ),
  ) + allegro_field_convert($sellformfields[34]);

  // Second bank account.
  $form['payment_shipment'][34] = array(
    '#states' => array(
      'visible' => array(':input[id="edit-14-1"]' => array('checked' => TRUE)),
    ),
  ) + allegro_field_convert($sellformfields[34]);

  // Additional information on shipment and payment.
  $form['payment_shipment'][27] = array(
    '#states' => array(
      'visible' => array(
        ':input[id="edit-13-16"]' => array('checked' => TRUE),
        ':input[id="edit-14-16"]' => array('checked' => TRUE),
      ),
    ),
  ) + allegro_field_convert($sellformfields[27]);

  // Location.
  $form['payment_shipment']['location'] = array(
    '#title' => t('Location'),
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  // Region.
  $form['payment_shipment']['location'][10] = array(
    '#required' => TRUE,
  ) + allegro_field_convert($sellformfields[10]);

  // City.
  $form['payment_shipment']['location'][11] = allegro_field_convert($sellformfields[11]);

  // Postcode.
  $form['payment_shipment']['location'][32] = allegro_field_convert($sellformfields[32]);

  // Description (body).
  $form[24] = array(
    '#type' => 'text_format',
    '#format' => 'full_html',
  ) + allegro_field_convert($sellformfields[24]);

  // $form['#after_build'][] = 'allegro_item_form_after_build';

  // Add the buttons.
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create auction'),
    '#submit' => array('allegro_item_form_submit'),
  );

  if (!empty($allegro_item->id)) {
    $form['actions']['delete'] = array(
      '#access' => user_access('delete allegro items'),
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#submit' => array('allegro_item_form_delete_submit'),
    );
  }

  $form['#validate'][] = 'allegro_item_form_validate';

  field_attach_form('allegro_item', $allegro_item, $form, $form_state);
  return $form;
}

function allegro_item_category_ajax_callback($form, &$form_state) {
  $form_state['current_category'] = $form_state['triggering_element']['#value'];
  return $form['item']['category'];
}

/**
 * AJAX callback returning fields for selected category.
 */
function allegro_item_category_fields_callback($form, &$form_state) {
  return $form['item']['extra_fields'];
}

function allegro_item_gallery_callback($form, &$form_state) {
  return $form['gallery'];
}

/**
 * Submit handler for the "Add image" button.
 * Increments the max counter and causes a rebuild.
 */
function allegro_item_gallery_add_image($form, &$form_state) {
  $form_state['gallery_count']++;
  $form_state['rebuild'] = TRUE;
  return $form['gallery'];
}

/**
 * Submit handler for the "Remove" button.
 * Removes last custom replacement.
 */
function allegro_item_gallery_remove_image($form, &$form_state) {
  $form_state['gallery_count']--;
  $form_state['rebuild'] = TRUE;
  return $form['gallery'];

}

/**
 * Validation callback for allegro_item_form().
 *
 * @return bool
 *   TRUE if form has no errors, FALSE otherwise.
 */
function allegro_item_form_validate($form, &$form_state) {
  // Form is not submitted (AJAX).
  if (empty($form_state['submitted'])
      || in_array('allegro_item_gallery_add_image', $form_state['submit_handlers'])
      || in_array('allegro_item_gallery_remove_image', $form_state['submit_handlers'])) {
    return TRUE;
  }

  $allegro_item = $form_state['allegro_item'];
  $values = &$form_state['values'];
  field_attach_form_validate('allegro_item', $allegro_item, $form, $form_state);

  $extra_fields = isset($form['item']['extra_fields']) ? $form['item']['extra_fields'] : array();
  $extra_values = array();

  foreach ($extra_fields as $extra_field) {
    if (!isset($extra_field['#sellformfield']) || !is_object($extra_field['#sellformfield'])) {
      continue;
    }
    $field = $extra_field['#sellformfield'];
    if ($field->opt == ALLEGRO_FIELD_REQUIRED && isset($values[$field->fid]) && in_array($field->type, array(4, 5, 6))) {
      $values[$field->fid] = $extra_values[$field->fid] = (array) $values[$field->fid];
      if (!in_array(TRUE, $values[$field->fid])) {
        form_set_error($field->fid, t('@field field is required.', array('@field' => $field->title)));
      }
    }
  }

  $category = $form_state['current_category'];

  $form_errors = form_get_errors();
  if (!empty($form_errors)) {
    return FALSE;
  }

  $fields = $extra_values;
  foreach ($values as $key => $value) {
    if (!empty($form['sellformfields']['#value'][$key])) {
      $sellformfield = $form['sellformfields']['#value'][$key];

      if ($sellformfield->type == 6) {
        $fields[$sellformfield->fid] = allegro_field_calculate_options($value);
      }
      elseif (is_array($value) && isset($value['value'])) {
        $fields[$sellformfield->fid] = $value['value'];
      }
      elseif (is_string($value)) {
        $fields[$sellformfield->fid] = $value;
      }
    }
  }

  // Category.
  $fields[2] = $category;
  // Start time.
  $fields[3] = REQUEST_TIME;
  // Country.
  $fields[9] = variable_get('allegro_country_code');

  // Check if Allegro item fields are filled correctly.
  try {
    $form_state['allegro_client'] = $allegro = allegro_client();
    $form_state['created_fields'] = allegro_field_create_multiple($fields);
    $form_state['checked_item'] = $allegro->checkNewAuction($form_state['created_fields']);
  }
  catch (SoapFault $e) {
    dpm($allegro, '$allegro');
    dpm($fields, '$fields');
    dpm($form_state, '$form_state');
    form_set_error('', $e->faultstring);
    watchdog_exception('allegro_item', $e);
    return FALSE;
  }
}

/**
 * Form submit callback for allegro_item_form().
 *
 * @global object $user
 *
 * @param array $form
 * @param array $form_state
 *
 * @return bool
 *   TRUE if no errors occurred, FALSE otherwise.
 */
function allegro_item_form_submit($form, &$form_state) {
  global $user;

  // Sending verified item to Allegro.
  try {
    $allegro = &$form_state['allegro_client'];
    $values = &$form_state['values'];
    $local_id = uniqid();

    $item = $allegro->newAuction($form_state['created_fields'], $local_id);
    $verified_item = $allegro->verifyItem($local_id);
    if ($item->itemId == $verified_item->itemId) {
      $item_link = l($values[1], allegro_item_url_external($verified_item->itemId, $allegro->countryCode));
      drupal_set_message(t('Auction !auction-link (%item_id) created successfully.', array('!auction-link' => $item_link, '%item_id' => $verified_item->itemId)));
      drupal_set_message(t('Detailed description of costs for listing an offer: <pre>@costs</pre>', array('@costs' => $form_state['checked_item']->itemPriceDesc)));
    }
    else {
      form_set_error('', t('Unknown error occurred while validating item.'));
      watchdog('allegro', 'Unknown error occurred while validating item');
      return FALSE;
    }
  }
  catch (SoapFault $e) {
    form_set_error('', $e->faultstring);
    watchdog_exception('allegro', $e);
    return FALSE;
  }

  // Saving Allegro item entity to DB.
  $allegro_item = &$form_state['allegro_item'];
  $allegro_item->external_id = $item->itemId;

  if (empty($allegro_item->country_code)) {
    $allegro_item->country_code = variable_get('allegro_country_code', 1);
  }

  // Set the Allegro_item uid if it's being created at this time.
  if (empty($allegro_item->uid)) {
    $allegro_item->uid = $user->uid;
  }

  $allegro_item->title = $form_state['values'][1];

  field_attach_submit('allegro_item', $allegro_item, $form, $form_state);
  allegro_item_save($allegro_item);
  drupal_set_message(t('Allegro item saved.'));
  $form_state['redirect'] = allegro_item_url($allegro_item);
}

/**
 * Form submit callback for deleting confirmation.
 *
 * @param array $form
 * @param array $form_state
 */
function allegro_item_form_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $allegro_item = $form['#allegro_item'];
  $form_state['redirect'] = array(
    'allegro-item/' . $allegro_item->id . '/delete',
    array('query' => $destination));
}

/**
 * Menu callback to display an Allegro item entity.
 *
 * @global object $language
 *
 * @param object $allegro_item
 * @param string $view_mode
 *
 * @return string
 */
function allegro_item_view($allegro_item, $view_mode = 'default') {
  $entity_type = 'allegro_item';

  $allegro_item->content = array(
    '#view_mode' => $view_mode,
  );

  field_attach_prepare_view($entity_type, array($allegro_item->id => $allegro_item), $view_mode);
  entity_prepare_view($entity_type, array($allegro_item->id => $allegro_item));
  $allegro_item->content += field_attach_view($entity_type, $allegro_item, $view_mode);

  $allegro_item->content['item_id'] = array(
    '#type' => 'item',
    '#title' => t('Item number'),
    '#markup' => $allegro_item->external_id,
  );
  $allegro_item->content['created'] = array(
    '#type' => 'item',
    '#title' => t('Created date'),
    '#markup' => format_date($allegro_item->created),
  );
  $external_url = allegro_item_url_external($allegro_item->external_id, $allegro_item->country_code);
  $allegro_item->content['external_url'] = array(
    '#type' => 'item',
    '#title' => t('External URL to Allegro item'),
    '#markup' => l($external_url, $external_url),
  );

  module_invoke_all('entity_view', $allegro_item, $entity_type, $view_mode, NULL);
  drupal_alter(array('allegro_item_page_view', 'entity_view'), $allegro_item->content, $entity_type);

  return $allegro_item->content;
}
