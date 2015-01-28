<?php

/**
 * Print a single row of multiple fields.
 */
function theme_field_collection_table_multiple_value_field($variables) {
  $element = $variables['element'];
  $header = array();
  $cells = array();

  // Order field widgets by their widget weight.
  $instances = field_info_instances($element['#entity_type'], $element['#bundle']);
  uasort($instances, '_field_collection_table_sort_items_widget_helper');
  foreach ($instances as $field_name => $instance) {
    if (empty($element[$field_name])) {
      continue;
    }

    $header[] = _field_collection_table_get_title($element[$field_name]);
    $cells[] = array('data' => $element[$field_name]);

    // Remove the original field to prevent duplicate printing.
    unset($element[$field_name]);
  }

  $element['field_collection_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => array($cells),
    '#weight' => 0,
  );

  $element['#sorted'] = FALSE;
  return drupal_render_children($element);
}

/**
 * Comparison function for sorting field instances by their widget's weight.
 */
function _field_collection_table_sort_items_widget_helper($a, $b) {
  $a_weight = (is_array($a) && isset($a['widget']['weight']) ? $a['widget']['weight'] : 0);
  $b_weight = (is_array($b) && isset($b['widget']['weight']) ? $b['widget']['weight'] : 0);
  return $a_weight - $b_weight;
}

/**
 * Replacement for theme_field_multiple_value_form().
 *
 * Each field is printed in a separate cell.
 */
function theme_field_collection_table_multiple_value_fields($variables) {
  $element = $variables['element'];
  $output = '';

  if (isset($element['#cardinality']) && ($element['#cardinality'] > 1 || $element['#cardinality'] == FIELD_CARDINALITY_UNLIMITED)) {
    $table_id = drupal_html_id($element['#field_name'] . '_values');
    $order_class = $element['#field_name'] . '-delta-order';
    $required = !empty($element['#required']) ? '<span class="form-required" title="' . t('This field is required. ') . '">*</span>' : '';

    $rows = array();

    // Sort items according to '_weight' (needed when the form comes back after
    // preview or failed validation)
    $items = array();
    foreach (element_children($element) as $key) {
      if ($key === 'add_more') {
        $add_more_button = &$element[$key];
      }
      else {
        $items[] = &$element[$key];
      }
    }
    usort($items, '_field_sort_items_value_helper');

    $header = array(
      array(
        'data' => '<label>' . t('!title: !required', array('!title' => $element['#title'], '!required' => $required)) . "</label>",
        'class' => array('field-label'),
      ),
    );
    $help_tooltips_js_loaded = FALSE;

    // Add the items as table rows.
    foreach ($items as $key => $item) {
      uasort($item, 'element_sort');
      $item['_weight']['#attributes']['class'] = array($order_class);
      $cells = array(
        array('data' => '', 'class' => array('field-multiple-drag')),
      );
      $help_format = isset($item['#instance']['widget']['settings']['field_collection_table_help']) ?
        $item['#instance']['widget']['settings']['field_collection_table_help'] : 'field';
      $help_is_tooltip = FALSE !== strpos($help_format, '_tooltip');

      // Load tooltip JS if tooltip help format and not already loaded.
      if ($help_is_tooltip && !$help_tooltips_js_loaded) {
        drupal_add_library('system', 'ui.tooltip');
        drupal_add_js(drupal_get_path('module', 'field_collection_table') . '/theme/tooltips.js', 'file');
        $help_tooltips_loaded = TRUE;
      }

      foreach (element_children($item) as $field_name) {
        if (isset($item[$field_name]['#language'])) {
          $item_language = $item[$field_name]['#language'];
          $item_help = isset($item[$field_name][$item_language]['#description']) ?
            $item[$field_name][$item_language]['#description'] : '';
          $item_is_required = $item[$field_name][$item_language]['#required'];
        }
        else {
          $item_language = '';
          $item_help = '';
          $item_is_required = FALSE;
        }

        // Remove help text from item if tooltip help format or a header help format.
        if ($help_is_tooltip || (0 === strpos($help_format, 'header'))) {
          // Is there a better way to do this?...

          $item[$field_name][$item_language]['#description'] = '';

          foreach (element_children($item[$field_name][$item_language]) as $index) {
            $item[$field_name][$item_language][$index]['#description'] = '';

            if (isset($item[$field_name][$item_language][$index]['value'])) {
              $item[$field_name][$item_language][$index]['value']['#description'] = '';
            }
            // TODO: Fix for other field types (such as link).
          }
        }

        // Only add the header/tooltips once.
        if ($key == 0) {
          $header_data = t('!title', array('!title' => _field_collection_table_get_title($item[$field_name])));

          if ($item_is_required) {
            $header_data .= ' ' . theme('form_required_marker');
          }

          $header_data = '<label style="display:inline-block;">' . $header_data . '</label>';

          if (!empty($item_help)) {
            if ('header_tooltip' == $help_format) {
              $header_data .= sprintf('<span class="ui-icon ui-icon-help"%s></span>',
                drupal_attributes(array(
                  'style' => 'display:inline-block;',
                  'title' => $item_help,
                ))
              );
            }
            elseif ('header' == $help_format) {
              $header_data .= '<div class="description">' . $item_help . '</div>';
            }
          }

          $header[] = array(
            'data' => $header_data,
            'class' => array('field-label'),
          );
        }

        // Add title attribute to field if field_tooltip help format.
        if (('field_tooltip' == $help_format) && !empty($item_help)) {
          $item[$field_name][$item_language]['#attributes']['title'] = array($item_help);

          foreach (element_children($item[$field_name][$item_language]) as $index) {
            // Is there a better way to do this?...

            $item[$field_name][$item_language][$index]['#attributes']['title'] = $item_help;

            if (isset($item[$field_name][$item_language][$index]['value'])) {
              $item[$field_name][$item_language][$index]['value']['#attributes']['title'] = $item_help;
            }
            // TODO: Fix for other field types (such as link).
          }
        }

        $cells[] = array('data' => $item[$field_name]);
      }

      $rows[] = array(
        'data' => $cells,
        'class' => array('draggable'),
      );
    }

    $output = array(
      '#prefix' => '<div class="form-item">',
      '#suffix' => '</div>',
    );
    $output['field_collection_table'] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
      '#attributes' => array(
        'id' => $table_id,
        'class' => array(
          'field-multiple-table',
        ),
      ),
    );
    if ($help_is_tooltip) {
      // Add attribute to table so JS can find table by selector.
      $output['field_collection_table']['#attributes']['data-field-collection-table-help-tooltips'] = '';
    }
    if (!empty($element['#description'])) {
      $output[] = array(
        '#prefix' => '<div class="description">',
        '#suffix' => '</div>',
        '#markup' => $element['#description'],
      );
    }
    if (isset($add_more_button)) {
      $output[] = $add_more_button + array(
        '#prefix' => '<div class="clearfix">',
        '#suffix' => '</div>',
      );
    }

    $output = drupal_render($output);

    drupal_add_tabledrag($table_id, 'order', 'sibling', $order_class);
  }
  else {
    foreach (element_children($element) as $key) {
      $output .= drupal_render($element[$key]);
    }
  }

  return $output;
}

/**
 * Implements template_preprocess_HOOK__PATTERN().
 */
function template_preprocess_table__field_collection_table(&$variables) {
  if (empty($variables['settings'])) {
    return;
  }
  if (isset($variables['settings']['empty'])) {
    _field_collection_table_hide_empty($variables);
  }
}

/**
 * Remove columns that are entirely empty.
 */
function _field_collection_table_hide_empty(&$variables) {
  $rows = $variables['rows'];

  $count = array();
  foreach ($rows as $row_delta => $row) {
    foreach ($row['data'] as $column_delta => $column) {
      if (!isset($count[$column_delta])) {
        $count[$column_delta] = 0;
      }
      if (isset($column['data']['#empty'])) {
        $count[$column_delta]++;
      }
    }
  }
  foreach ($count as $column_delta => $column) {
    if ($column === count($rows)) {
      foreach ($rows as $row_delta => $row) {
        unset($variables['rows'][$row_delta]['data'][$column_delta]);
        unset($variables['header'][$column_delta]);
      }
    }
  }
}

/**
 * Derivative of theme_table() solely for the HOOK_preprocess_table__PATTERN().
 */
function theme_table__field_collection_table($variables) {
  return theme_table($variables);
}

/**
 * Helps find the title of the field, as it could be in several places.
 */
function _field_collection_table_get_title($field) {
  $title = '';

  if (isset($field['#language']) && isset($field[$field['#language']])) {
    $language = $field['#language'];
    if (isset($field[$language]['#title'])) {
      $title = $field[$language]['#title'];
    }
    elseif (isset($field[$language][0]['#title'])) {
      $title = $field[$language][0]['#title'];
    }
  }
  elseif (isset($field['#title'])) {
    $title = empty($field['#is_weight']) ? $field['#title'] : t('Order');
  }
  elseif (isset($field['#value'])) {
    $title = $field['#value'];
  }

  return $title;
}
