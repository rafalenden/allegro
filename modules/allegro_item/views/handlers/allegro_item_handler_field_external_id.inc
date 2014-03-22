<?php

/**
 * @file
 * Contains the basic product field handler.
 */

/**
 * Field handler to provide simple renderer that allows linking to a product.
 */
class allegro_item_handler_field_external_id extends views_handler_field_numeric {
  function init(&$view, &$options) {
    parent::init($view, $options);

    if (!empty($this->options['link_to_external_item'])) {
      $this->additional_fields['external_id'] = 'external_id';
      $this->additional_fields['country_code'] = 'country_code';
    }
  }

  function option_definition() {
    $options = parent::option_definition();

    $options['link_to_external_item'] = array('default' => FALSE);

    return $options;
  }

  /**
   * Provide the link to product option.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['link_to_external_item'] = array(
      '#title' => t('Link this field to the external Allegro item'),
      '#description' => t('This will override any other link you have set.'),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['link_to_external_item']),
    );
  }

  /**
   * Render whatever the data is as a link to the product.
   *
   * Data should be made XSS safe prior to calling this function.
   */
  function render_link($data, $values) {
    if (!empty($this->options['link_to_external_item']) && $data !== NULL && $data !== '') {
      $external_id = $this->get_value($values, 'external_id');
      $country_code = $this->get_value($values, 'country_code');
      $this->options['alter']['make_link'] = TRUE;
      $this->options['alter']['path'] = allegro_item_url_external($external_id, $country_code);
    }

    return $data;
  }

  function render($values) {
    $value = $this->get_value($values);
    return $this->render_link($this->sanitize_value($value), $values);
  }
}
