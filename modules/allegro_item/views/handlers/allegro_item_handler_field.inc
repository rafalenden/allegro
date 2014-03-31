<?php

/**
 * @file
 * Contains the basic product field handler.
 */

/**
 * Field handler to provide simple renderer that allows linking to a product.
 */
class allegro_item_handler_field extends views_handler_field {
  function init(&$view, &$options) {
    parent::init($view, $options);

    if (!empty($this->options['link_to_item'])) {
      $this->additional_fields['id'] = 'id';
    }
  }

  function option_definition() {
    $options = parent::option_definition();

    $options['link_to_item'] = array('default' => FALSE);

    return $options;
  }

  /**
   * Provide the link to product option.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['link_to_item'] = array(
      '#title' => t('Link this field to the Allegro item'),
      '#description' => t('This will override any other link you have set.'),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['link_to_item']),
    );
  }

  /**
   * Render whatever the data is as a link to the product.
   *
   * Data should be made XSS safe prior to calling this function.
   */
  function render_link($data, $values) {
    if (!empty($this->options['link_to_item']) && $data !== NULL && $data !== '') {
      $item_id = $this->get_value($values, 'id');
      $this->options['alter']['make_link'] = TRUE;
      $this->options['alter']['path'] = 'allegro-item/' . $item_id;
    }

    return $data;
  }

  function render($values) {
    $value = $this->get_value($values);
    return $this->render_link($this->sanitize_value($value), $values);
  }
}
