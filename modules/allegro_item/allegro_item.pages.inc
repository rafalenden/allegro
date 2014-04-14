<?php

/**
 * @file
 * Allegro Item pages.
 */

/**
 * Allegro item view page.
 */
function allegro_item_view_page(AllegroItem $allegro_item) {
  drupal_set_title($allegro_item->label());
  return allegro_item_view($allegro_item);
}

/**
 * Present an Allegro item submission form.
 */
function allegro_item_add_page() {
  $allegro_item = allegro_item_create();
  drupal_set_title(t('Create Allegro item'));
  return drupal_get_form('allegro_item_form', $allegro_item);
}

/**
 * Menu callback; presents the Allegro item editing form, or redirects to delete confirmation.
 *
 * @param object $allegro_item
 *   The Allegro item object to edit.
 */
function allegro_item_edit_page(AllegroItem $allegro_item) {
  drupal_set_title(t('Edit Allegro item: %title', array('%title' => $allegro_item->label())), PASS_THROUGH);
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
  // Definitions of fields or categories are not downloaded form WebAPI.
  if (!variable_get('allegro_field_last_update') || !variable_get('allegro_category_last_update')) {
    if (!variable_get('allegro_field_last_update')) {
      drupal_set_message(t('Field information is not downloaded from Allegro. <a href="@fields-url">Download field information</a>.', array('@fields-url' => url('admin/allegro/settings/fields'))), 'error');
    }

    if (!variable_get('allegro_category_last_update')) {
      drupal_set_message(t('Categories are not downloaded from Allegro. <a href="@fields-url">Download categories</a>.', array('@fields-url' => url('admin/allegro/settings/categories'))), 'error');
    }

    return array();
  }

  $form['#id'] = 'allegro-item-form';

  $path = drupal_get_path('module', 'allegro_item');
  $form['#attached']['css'][] = $path . '/allegro_item.css';
  $form['#attached']['js'][] = $path . '/allegro_item.js';

  // Load and convert information about external fields.
  $sellformfields = allegro_field_load_multiple(allegro_get_fields());
  $sellformfields_converted = allegro_field_convert_multiple($sellformfields);

  // Save the Allegro item for later, in case we need it.
  $form_state['allegro_item'] =& $allegro_item;

  // Save information about external fields for later, in case we need it.
  $form_state['sellformfields'] =& $sellformfields;

  // Editing Allegro item.
  if (!$allegro_item->is_new) {
    foreach ($allegro_item->field_data as $fid => $value) {
      if (isset($sellformfields_converted[$fid])) {
        if ($sellformfields_converted[$fid]['#type'] == 'checkboxes') {
          $value = allegro_field_decalculate_options($sellformfields_converted[$fid]['#options'], $value);
        }
        $sellformfields_converted[$fid]['#default_value'] = $value;
      }
    }

    $form_state['current_category'] = $sellformfields_converted[2]['#default_value'];
  }

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
  ) + $sellformfields_converted[1];

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

    if (!isset($lineage[$level + 1])) {
      $current_cid = NULL;
    }
    else {
      $current_cid = $lineage[$level + 1];
    }

    $form['item']['category']['level' . $level] = array(
      '#type' => 'select',
      '#size' => 10,
      '#options' => $children,
      '#prefix' => '<div class="category-level">',
      '#suffix' => '</div>',
      '#default_value' => $current_cid,
      '#limit_validation_errors' => array(),
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
      $default_value = isset($allegro_item->field_data[$sellformfield->fid])
        ? $allegro_item->field_data[$sellformfield->fid]
        : NULL;

      $form['item']['category']['extra_fields'][$sellformfield->fid] = array(
        '#description' => NULL,
        '#attributes' => array('class' => array('category-extra-field')),
        '#weight' => $sellformfield->pos,
      ) + allegro_field_convert($sellformfield, $default_value);
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
    $form['gallery']['images'][$i] = $sellformfields_converted[$i];
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


  // Auction type.
  $form['offer']['auction_type'] = array(
    '#title' => t('Auction type'),
    '#type' => 'radios',
    '#default_value' => 1,
    '#options' => array(
      1 => t('Only Buy Now! (no bidding)'),
      2 => t('Auction (bidding)'),
    ),
    '#required' => TRUE,
  );

  if (!empty($sellformfields_converted[6]['#default_value'])) {
    $form['offer']['auction_type']['#default_value'] = ALLEGRO_ITEM_TYPE_AUCTION;
  }

  // Quantity.
  $form['offer'][5] = array(
    '#title' => t('Quantity'),
    '#description' => '',
  ) + $sellformfields_converted[5];

  // Quantity type.
  $form['offer'][28] = array(
    '#title' => '',
  ) + $sellformfields_converted[28];

  // Starting price.
  $form['offer'][6] = array(
    '#field_suffix' => t('zł'),
    '#description' => '',
    '#states' => array(
      'visible' => array(':input[name=auction_type]' => array('value' => ALLEGRO_ITEM_TYPE_AUCTION)),
    ),
  ) + $sellformfields_converted[6];

  // Minimum price.
  $form['offer'][7] = array(
    '#field_suffix' => t('zł'),
    '#description' => '',
    '#states' => array(
      'visible' => array(':input[name=auction_type]' => array('value' => ALLEGRO_ITEM_TYPE_AUCTION)),
    ),
  ) + $sellformfields_converted[7];

  // Buy now price.
  $form['offer'][8] = array(
    '#field_suffix' => t('zł'),
    '#description' => '',
    '#required' => TRUE,
  ) + $sellformfields_converted[8];

  // Duration.
  $form['offer'][4] = array(
    '#field_suffix' => t('days'),
  ) + $sellformfields_converted[4];
  unset($form['offer'][4]['#options'][5]);

  // Additional options (Promotion options).
  $form['offer'][15] = array(
    '#description' => NULL,
  ) + $sellformfields_converted[15];
  unset($form['offer'][15]['#options'][64]);

  $form['payment_shipment'] = array(
    '#title' => t('Payment and shipment'),
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
  );

  $form_state['prepayment_shipment'] = array(36, 38, 37, 39, 41, 43, 44, 46, 47, 50, 51);
  $prepayment =& $form_state['prepayment_shipment'];

  $form['payment_shipment']['prepayment'] = array(
    '#title' => t('Payment in advance'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['payment_shipment']['prepayment']['costs'] = array(
    '#type' => 'tableform',
    '#header' => array(
      t('Shipping method'),
      t('First item'),
      t('Next item'),
      t('Quantity in pack'),
    ),
    '#options' => allegro_field_convert_shipping_fields_table($prepayment, $sellformfields, $allegro_item->field_data),
  );

  $form_state['cod_shipment'] = array(40, 42, 45, 48, 49, 52);
  $cod =& $form_state['cod_shipment'];

  $form['payment_shipment']['cod'] = array(
    '#title' => t('Cash on delivery'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['payment_shipment']['cod']['costs'] = array(
    '#type' => 'tableform',
    '#header' => array(
      t('Shipping method'),
      t('First item'),
      t('Next item'),
      t('Quantity in pack'),
    ),
    '#options' => allegro_field_convert_shipping_fields_table($cod, $sellformfields, $allegro_item->field_data),
  );

  // Free shipping methods.
  $form['payment_shipment'][35] = $sellformfields_converted[35];
  $form['payment_shipment'][35]['#options'][1] .= ' (' . t('cash on delivery') . ')';

  // Shipping delay.
  if (isset($sellformfields[340])) {
    $form['payment_shipment'][340] = $sellformfields_converted[340];
    $form['payment_shipment'][340]['#options'][1] = t('immediately');
    foreach ($form['payment_shipment'][340]['#options'] as $key => $value) {
      $days = $value / 24;
      if ($key > 1) {
        $label = format_plural($days, t('1 day'), t('@days days'), array('@days' => $days));
        $form['payment_shipment'][340]['#options'][$key] = $label;
      }
    }
  }

  // Who pay for shipping.
  $form['payment_shipment'][12] = $sellformfields_converted[12];

  // Shipping options.
  $form['payment_shipment'][13] = array(
    '#description' => NULL,
  ) + $sellformfields_converted[13];
  unset($form['payment_shipment'][13]['#options'][16]);

  // Payment types.
  $form['payment_shipment'][14] = array(
    '#title' => t('Payment details'),
    '#description' => NULL,
  ) + $sellformfields_converted[14];
  // Details in item description.
  unset($form['payment_shipment'][14]['#options'][16]);

  // First bank account.
  $form['payment_shipment'][33] = array(
    '#states' => array(
      'visible' => array(':input[id="edit-14-1"]' => array('checked' => TRUE)),
    ),
  ) + $sellformfields_converted[33];

  // First bank account.
  $form['payment_shipment'][34] = array(
    '#states' => array(
      'visible' => array(':input[id="edit-14-1"]' => array('checked' => TRUE)),
    ),
  ) + $sellformfields_converted[34];

  // Second bank account.
  $form['payment_shipment'][34] = array(
    '#states' => array(
      'visible' => array(':input[id="edit-14-1"]' => array('checked' => TRUE)),
    ),
  ) + $sellformfields_converted[34];

  // Additional information on shipment and payment.
  $form['payment_shipment'][27] = array(
    '#states' => array(
      'visible' => array(
        ':input[id="edit-13-16"]' => array('checked' => TRUE),
        ':input[id="edit-14-16"]' => array('checked' => TRUE),
      ),
    ),
  ) + $sellformfields_converted[27];

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
  ) + $sellformfields_converted[10];

  // City.
  $form['payment_shipment']['location'][11] = $sellformfields_converted[11];

  // Postcode.
  $form['payment_shipment']['location'][32] = $sellformfields_converted[32];

  // Description (body).
  $form[24] = array(
    '#type' => 'text_format',
    '#format' => 'full_html',
  ) + $sellformfields_converted[24];

  // $form['#after_build'][] = 'allegro_item_form_after_build';

  // Add the buttons.
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => $allegro_item->is_new ? t('Create auction') : t('Update auction'),
    '#submit' => array('allegro_item_form_submit'),
  );

  $form['#validate'][] = 'allegro_item_form_validate';

  field_attach_form('allegro_item', $allegro_item, $form, $form_state);
  return $form;
}

/**
 * Ajax callback for category and related fields.
 */
function allegro_item_category_ajax_callback($form, &$form_state) {
  $form_state['current_category'] = $form_state['triggering_element']['#value'];
  return $form['item']['category'];
}

/**
 * Ajax callback for item images.
 */
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

  $allegro_item =& $form_state['allegro_item'];
  $values =& $form_state['values'];
  field_attach_form_validate('allegro_item', $allegro_item, $form, $form_state);

  $shipment_costs = array();
  $disabled_fid = NULL;

  foreach ($form['payment_shipment']['prepayment']['costs'] + $form['payment_shipment']['cod']['costs'] as $fid => $element) {
    if (isset($values['label_' . $fid]) && empty($values['label_' . $fid])) {
      $disabled_fid = $fid;
    }
    if (isset($disabled_fid) && in_array($disabled_fid, array($fid, $fid - 100, $fid - 200))) {
      $values[$fid] = NULL;
    }
  }

  if ($values['auction_type'] == ALLEGRO_ITEM_TYPE_BUY_NOW) {
    $values[6] = NULL;
    $values[7] = NULL;
  }

  $extra_fields = isset($form['item']['category']['extra_fields'])
    ? $form['item']['category']['extra_fields']
    : array();

  foreach ($extra_fields as $key => $extra_field) {
    if (!isset($extra_field['#sellformfield']) || !is_object($extra_field['#sellformfield'])) {
      continue;
    }
    $field = $extra_field['#sellformfield'];
    $form_state['sellformfields'][$key] = $field;

    if (isset($values[$field->fid])) {
      $fields[$field->fid] = (array) $values[$field->fid];

      // Field type is one of checkboxes, radios or select.
      $has_options =  in_array($field->type, array(
        ALLEGRO_FIELD_COMBOBOX, ALLEGRO_FIELD_RADIOBUTTON, ALLEGRO_FIELD_CHECKBOX
      ));
      // Check if required field with options is not empty.
      if ($field->opt == ALLEGRO_FIELD_REQUIRED && $has_options && !in_array(TRUE, $fields[$field->fid])) {
        form_set_error($field->fid, t('@field field is required.', array('@field' => $field->title)));
      }
    }
  }

  // Don't continue when there are any errors.
  $form_errors = form_get_errors();
  if (!empty($form_errors)) {
    return FALSE;
  }

  $form_state['sellformfields_values'] =& $fields;

  foreach ($values as $key => $value) {
    if (isset($form_state['sellformfields'][$key])) {
      $sellformfield = $form_state['sellformfields'][$key];

      // Single value field.
      if (is_scalar($value)) {
        $fields[$sellformfield->fid] = $value;
      }
      // Filtered text.
      elseif (is_array($value) && isset($value['value'])) {
        $fields[$sellformfield->fid] = $value['value'];
      }
      // Field with options (select, checkboxes).
      elseif (is_array($value)) {
        $fields[$sellformfield->fid] = allegro_field_calculate_options($value);
      }
    }
  }

  // Category.
  $fields[2] = $form_state['current_category'];

  // Add to request country from global settings.
  $fields[9] = variable_get('allegro_country_code');

  // Check if Allegro item fields are filled correctly.
  try {
    $allegro = allegro_client();

    if ($allegro_item->is_new) {
      // Start time.
      $fields[3] = REQUEST_TIME;
      $form_state['sellformfields_prepared'] = allegro_field_prepare_multiple($fields);
      $form_state['checked_item'] = $allegro->checkNewAuction($form_state['sellformfields_prepared']);
    }
    else {
      $fields = array_diff_assoc($fields, $allegro_item->field_data);
      // Unset start price and minimal price when auction type is "Buy now!".
      if ($values['auction_type'] == 1) {
        $fields[6] = $fields[7] = 0;
      }

      $allegro_item->fields_to_update = $fields;
      $form_state['sellformfields_prepared'] = allegro_field_prepare_multiple($fields);
      $form_state['checked_item'] = $allegro->changeItemFields($allegro_item->external_id, $form_state['sellformfields_prepared'], array(), TRUE);
      $form_state['checked_item'] = $form_state['checked_item']->changedItem;
    }

    // throw new Exception('Test');
  }
  catch (Exception $e) {
    if (function_exists('dpm')) {
      dpm($allegro, '$allegro');
      dpm($allegro_item, '$allegro_item');
      dpm($fields, '$fields');
      dpm($form_state, '$form_state');
    }

    form_set_error('', $e->getMessage());
    watchdog_exception('allegro_item', $e);
    return FALSE;
  }
}

/**
 * Form submit callback for allegro_item_form().
 *
 * @return bool
 *   TRUE if no errors occurred, FALSE otherwise.
 */
function allegro_item_form_submit($form, &$form_state) {
  global $user;

  // Sending verified item to Allegro.
  try {
    $allegro = allegro_client();

    $allegro_item =& $form_state['allegro_item'];
    $allegro_item->title = $form_state['values'][1];
    $allegro_item->sellformfields = $form_state['sellformfields_values'];
    $allegro_item->sellformfields_prepared = $form_state['sellformfields_prepared'];

    field_attach_submit('allegro_item', $allegro_item, $form, $form_state);

    $is_new = $allegro_item->is_new;
    $t_args = array('%title' => $allegro_item->title);
    $allegro_item = allegro_item_save($allegro_item);

    if (!($allegro_item instanceof AllegroItem)) {
      throw new Exception(t('Error occurred while saving Allegro item %title.', $t_args));
    }

    if ($is_new) {
      $item_external_link = l($allegro_item->title, $allegro_item->externalURL());

      drupal_set_message(t('Auction !auction-link (%item_id) created successfully.', array('!auction-link' => $item_external_link, '%item_id' => $allegro_item->external_id)));

      $costs = array(
        'title' => t('Detailed description of costs for listing an offer'),
        'items' => explode(PHP_EOL, trim($form_state['checked_item']->itemPriceDesc)),
      );
      drupal_set_message(theme('item_list', $costs));

      watchdog('allegro_item', 'Added Allegro item %title.', $t_args, WATCHDOG_NOTICE, l(t('view'), $allegro_item->url()));
    }
    else {
      drupal_set_message(t('The auction %title has been updated.', $t_args));

      $costs = array(
        'title' => t('Additional costs for updating an offer'),
        'items' => array(),
      );

      foreach ($form_state['checked_item']->itemSurcharge->item as $cost) {
        $costs['items'][] = $cost->surchargeDescription . ': ' . $cost->surchargeAmount->amountValue . $cost->surchargeAmount->amountCurrencySign;
      }
      drupal_set_message(theme('item_list', $costs));
      watchdog('allegro_item', 'Updated Allegro item %title.', $t_args, WATCHDOG_NOTICE, l(t('view'), $allegro_item->url()));
    }

    $form_state['redirect'] = $allegro_item->url();
  }
  catch (Exception $e) {
    form_set_error('', $e->getMessage());
    return FALSE;
  }
}

/**
 * Form constructor for the Allegro item finishing confirmation form.
 */
function allegro_item_finish_confirm($form, &$form_state, $allegro_item) {
  $form['allegro_item_id'] = array('#type' => 'value', '#value' => $allegro_item->id);
  return confirm_form($form,
    t('Are you sure you want to finish %title?', array('%title' => $allegro_item->title)),
    $allegro_item->url(),
    t('This action cannot be undone.'),
    t('Finish auction'),
    t('Cancel')
  );
}

/**
 * Executes Allegro item finishing.
 *
 * @see allegro_item_finish_confirm()
 */
function allegro_item_finish_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $allegro_item = allegro_item_load($form_state['values']['allegro_item_id']);
    $deleting_succedded = allegro_item_finish($form_state['values']['allegro_item_id']);

    $t_args = array('%title' => $allegro_item->title);
    if ($deleting_succedded) {
      cache_clear_all();
      watchdog('allegro_item', 'Finished Allegro item %title.', $t_args);
      drupal_set_message(t('Allegro item %title has been finished.', $t_args));
      $form_state['redirect'] = 'admin/allegro';
    }
    else {
      watchdog('allegro_item', 'Error occurred while finishing Allegro item %title.', $t_args);
      drupal_set_message(t('Allegro item %title could not be finished.', $t_args), 'error');
    }
  }
}

/**
 * Form constructor for the Allegro item deletion confirmation form.
 */
function allegro_item_delete_confirm($form, &$form_state, $allegro_item) {
  $allegro_item_website = allegro_load_website_by_country_code($allegro_item->country_code);

  // Always provide entity id in the same form key as in the entity edit form.
  $form['allegro_item_id'] = array('#type' => 'value', '#value' => $allegro_item->id);
  $form['finish_auction'] = array(
    '#type' => 'checkbox',
    '#title' => t('End selling of this auction at %website', array('%website' => $allegro_item_website)),
    '#default_value' => TRUE,
  );

  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $allegro_item->title)),
    $allegro_item->url(),
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Executes Allegro item deletion.
 *
 * @see allegro_item_delete_confirm()
 */
function allegro_item_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $allegro_item = allegro_item_load($form_state['values']['allegro_item_id']);
    $deleting_succedded = allegro_item_delete($form_state['values']['allegro_item_id'], $form_state['values']['finish_auction']);

    $t_args = array('%title' => $allegro_item->title);
    if ($deleting_succedded) {
      cache_clear_all();
      watchdog('allegro_item', 'Deleted Allegro item %title.', $t_args);
      drupal_set_message(t('Allegro item %title has been deleted.', $t_args));
      $form_state['redirect'] = 'admin/allegro';
    }
    else {
      watchdog('allegro_item', 'Error occurred while deleting Allegro item %title.', $t_args);
      drupal_set_message(t('Allegro item %title could not be deleted.', $t_args), 'error');
    }
  }
}

/**
 * Form constructor for the Allegro item selling again confirmation form.
 */
function allegro_item_sell_again_confirm($form, &$form_state, $allegro_item) {
  $form['allegro_item_id'] = array('#type' => 'value', '#value' => $allegro_item->id);
  return confirm_form($form,
    t('Are you sure you want to sell again %title?', array('%title' => $allegro_item->title)),
    $allegro_item->url(),
    t('This action cannot be undone.'),
    t('Sell again'),
    t('Cancel')
  );
}

/**
 * Executes selling again Allegro item.
 *
 * @see allegro_item_sell_again_confirm()
 */
function allegro_item_sell_again_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $allegro_item = allegro_item_load($form_state['values']['allegro_item_id']);
    $selling_again_succedded = allegro_item_sell_again($form_state['values']['allegro_item_id']);

    $t_args = array('%title' => $allegro_item->title);
    if ($selling_again_succedded) {
      watchdog('allegro_item', 'Selling again Allegro item %title.', $t_args);
      drupal_set_message(t('Allegro item %title has been listed again.', $t_args));
      $form_state['redirect'] = 'admin/allegro';
    }
    else {
      watchdog('allegro_item', 'Error occurred while listing again Allegro item %title.', $t_args);
      drupal_set_message(t('Allegro item %title could not be listed again.', $t_args), 'error');
    }
  }
}
