(function($) {

  Drupal.behaviors.allegroItem = {
    attach: function() {
      $('#hierarchical-select-0-wrapper').bind('before-update-hierarchical-select', function() {
        var option = $(this).find('select option:selected');
        if (option.hasClass('has-children')) {
          $('#category-extra-fields').text('');
        }
        if (option.length > 0) {
          $('#extra_fields_helper').trigger('hs_changed');
        }
      });
    }
  };

})(jQuery);
