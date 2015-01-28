(function ($) {
  Drupal.behaviors.theme_field_collection_table_help_tooltips = {
    attach: function (context, settings) {
      $('table[data-field-collection-table-help-tooltips]', context).tooltip();
    }
  };
})(jQuery);
