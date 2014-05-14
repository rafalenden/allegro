<?php

/**
 * @file
 * Allegro item entity class.
 */

/**
 * The Allegro item entity class
 */
class AllegroItem extends Entity {
  public $title;
  public $id;
  public $external_id;
  public $country_code;
  public $status;
  public $uid;
  public $created;
  public $changed;
  public $is_new = TRUE;
  public $field_data = array();

  /**
   * Create a default Allegro item.
   */
  public function __construct(array $values = array()) {
    $this->status = ALLEGRO_ITEM_STATUS_SELLING;
    return parent::__construct($values, 'allegro_item');
  }

  /**
   * Finish selling of Allegro item.
   */
  public function finish() {
    return entity_get_controller($this->entityType)->finish(array($this->identifier()));
  }

  /**
   * Override this in order to implement a custom default URI and specify
   * 'entity_class_uri' as 'uri callback' hook_entity_info().
   *
   * @return array
   */
  protected function defaultUri() {
    return array('path' => 'allegro-item/' . $this->identifier());
  }

  /**
   * URL string.
   */
  public function url() {
    $uri = $this->defaultUri();
    return $uri['path'];
  }

  /**
   * External URL string.
   */
  public function externalUrl() {
    if (!empty($this->external_id)) {
      return allegro_item_url_external($this->external_id, $this->country_code);
    }
  }

  /**
   * Build the URL string.
   */
  protected function buildUrl($type) {
    $url = $this->url();
    return $url . '/' . $type;
  }

  /**
   * View URL.
   */
  public function viewUrl() {
    return $this->buildUrl('view');
  }

  /**
   * Edit URL.
   */
  public function editUrl() {
    return $this->buildUrl('edit');
  }

  /**
   * Delete URL.
   */
  public function deleteUrl() {
    return $this->buildUrl('delete');
  }

}
