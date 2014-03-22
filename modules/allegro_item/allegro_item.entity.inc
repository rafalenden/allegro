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
  public $is_new;

  /**
   * Create a default Allegro item.
   */
  public function __construct(array $values = array()) {
    $this->is_new = TRUE;
    $this->status = ALLEGRO_ITEM_STATUS_SELLING;
    return parent::__construct($values, 'allegro_item');
  }

  /**
   * Override this in order to implement a custom default URI and specify
   * 'entity_class_uri' as 'uri callback' hook_entity_info().
   *
   * @return array
   */
  protected function defaultUri() {
    return array('path' => 'allegro-item/' . strtolower($this->identifier()));
  }

  /**
   * URL string
   */
  public function url() {
    $uri = $this->defaultUri();
    return $uri['path'];
  }

  /**
   * Build the URL string
   */
  protected function buildURL($type) {
    $url = $this->url();
    return $url . '/' . $type;
  }

  /**
   * View URL
   */
  public function viewURL() {
    return $this->buildURL('view');
  }

  /**
   * Edit URL
   */
  public function editURL() {
    return $this->buildURL('edit');
  }

  /**
   * Delete URL
   */
  public function deleteURL() {
    return $this->buildURL('delete');
  }

}
