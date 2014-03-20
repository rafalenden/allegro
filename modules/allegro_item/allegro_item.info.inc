<?php

/**
 * @file
 * Provides metadata for the allegro item entity.
 */

/**
 * Implements hook_entity_property_info().
 */
function allegro_item_entity_property_info() {
  $info = array();

  // Add meta-data about the basic llegro_item properties.
  $properties = &$info['allegro_item']['properties'];

  $properties['id'] = array(
    'label' => t('Item ID'),
    'description' => t('The internal numeric ID of the item.'),
    'type' => 'integer',
    'schema field' => 'id',
  );
  $properties['external_id'] = array(
    'label' => t('External ID'),
    'description' => t('The external numeric ID of the item.'),
    'type' => 'integer',
    'setter callback' => 'entity_property_verbatim_set',
    'setter permission' => 'administer allegro items',
    'schema field' => 'external_id',
  );
  $properties['country_code'] = array(
    'label' => t('Country code'),
    'description' => t('Code of country where item is listed.'),
    'type' => 'integer',
    'setter callback' => 'entity_property_verbatim_set',
    'setter permission' => 'administer allegro items',
    'schema field' => 'country_code',
  );
  $properties['status'] = array(
    'label' => t('Status'),
    'description' => t('Boolean indicating whether the item is active or disabled.'),
    'type' => 'boolean',
    'options list' => 'allegro_item_status_options_list',
    'setter callback' => 'entity_property_verbatim_set',
    'setter permission' => 'administer allegro items',
    'schema field' => 'status',
  );
  $properties['title'] = array(
    'label' => t('Title'),
    'description' => t('The title of the item.'),
    'type' => 'text',
    'setter callback' => 'entity_property_verbatim_set',
    'required' => TRUE,
    'schema field' => 'title',
  );
  $properties['uid'] = array(
    'label' => t('Creator ID'),
    'type' => 'integer',
    'description' => t('The unique ID of the item creator.'),
    'setter callback' => 'entity_property_verbatim_set',
    'setter permission' => 'administer allegro items',
    'clear' => array('creator'),
    'schema field' => 'uid',
  );
  $properties['created'] = array(
    'label' => t('Date created'),
    'description' => t('The date the item was created.'),
    'type' => 'date',
    'setter callback' => 'entity_property_verbatim_set',
    'setter permission' => 'administer allegro items',
    'schema field' => 'created',
  );
  $properties['changed'] = array(
    'label' => t('Date updated'),
    'description' => t('The date the item was most recently updated.'),
    'type' => 'date',
    'setter callback' => 'entity_property_verbatim_set',
    'query callback' => 'entity_metadata_table_query',
    'setter permission' => 'administer allegro items',
    'schema field' => 'changed',
  );

  return $info;
}
