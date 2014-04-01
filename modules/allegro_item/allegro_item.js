(function($) {

  Drupal.behaviors.allegroItemForm = {
    attach: function(context, settings) {

      $form = $('#allegro-item-form', context);

      $form.once(function() {
        $(this).submit(function() {
          $(this).find('input[type=submit]').click(function(e) {
            e.preventDefault();
          });
        });
      });

    }
  }

})(jQuery);
