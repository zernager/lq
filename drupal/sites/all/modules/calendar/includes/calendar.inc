<?php

/**
 * @file
 * Calendar building functions for the Calendar module.
 */

/**
 * Build calendar
 *
 * @param unknown_type $view
 * @param unknown_type $items
 * @return themed table
 */
function calendar_build_calendar($view, $items) {
  // Remove nodes outside the selected date range.
  $values = array();
  foreach ($items as $delta => $item) {
    if (empty($item->calendar_start_date) || empty($item->calendar_end_date)) {
      continue;
    }
    $item_start = date_format($item->calendar_start_date, DATE_FORMAT_DATE);
    $item_end = date_format($item->calendar_end_date, DATE_FORMAT_DATE);
    if (($item_start >= $view->date_info->min_date_date && $item_start <= $view->date_info->max_date_date)
     || ($item_end >= $view->date_info->min_date_date && $item_end <= $view->date_info->max_date_date)) {
       $values[$item_start][date_format($item->calendar_start_date, 'H:i:s')][] = $item;
    }
  }
  $items = $values;
  ksort($items);

  $rows = array();
  $curday = clone($view->date_info->min_date);

  switch ($view->date_info->granularity) {
    case 'year':
      $rows = array();
      $view->date_info->mini = TRUE;
      for ($i = 1; $i <= 12; $i++) {
        $rows[$i] = calendar_build_month($curday, $view, $items);
      }
      $view->date_info->mini = FALSE;
      break;

    case 'month':
      $rows = calendar_build_month($curday, $view, $items);
      break;

    case 'day':
      $rows = calendar_build_day($curday, $view, $items);
      break;

    case 'week':
      $rows = calendar_build_week($curday, $view, $items);

      // Merge the day names in as the first row.
      $rows = array_merge(array(calendar_week_header($view)), $rows);
      break;
  }
  return $rows;

}

/**
 * Build one month.
 */
function calendar_build_month(&$curday, $view, $items) {
  $month = date_format($curday, 'n');
  date_modify($curday, '-' . strval(date_format($curday, 'j')-1) . ' days');
  $rows = array();
  do {
    $rows = array_merge($rows, calendar_build_week($curday, $view, $items, TRUE));
    $curday_date = date_format($curday, DATE_FORMAT_DATE);
    $curday_month = date_format($curday, 'n');
  } while ($curday_month == $month && $curday_date <= $view->date_info->max_date_date);

  // Merge the day names in as the first row.
  $rows = array_merge(array(calendar_week_header($view)), $rows);
  return $rows;
}

/**
 * Build one week row.
 */
function calendar_build_week(&$curday, $view, $items, $check_month = FALSE) {
  $curday_date = date_format($curday, DATE_FORMAT_DATE);
  $weekdays = calendar_untranslated_days($items, $view);
  $today = date_format(date_now(date_default_timezone()), DATE_FORMAT_DATE);
  $month = date_format($curday, 'n');
  $week = date_week($curday_date);
  $first_day = variable_get('date_first_day', 1);

  // move backwards to the first day of the week
  $day_wday = date_format($curday, 'w');
  date_modify($curday, '-' . strval((7 + $day_wday - $first_day) % 7) . ' days');
  $curday_date = date_format($curday, DATE_FORMAT_DATE);

  // If we're displaying the week number, add it as the
  // first cell in the week.
  if (!empty($view->date_info->style_with_weekno) && !in_array($view->date_info->granularity, array('day', 'week'))) {
    $url = date_real_url($view, NULL, $view->date_info->year .'-W'. $week);
    if (!empty($view->date_info->display_types['week'])) {
      $weekno = l($week, $url, array('query' => !empty($view->date_info->append) ? $view->date_info->append : ''));
    }
    else { 
      // Do not link week numbers, if Week views are disabled.
      $weekno = $week;
    }
    $rows[$week][] = array(
      'data' => $weekno,
      'id' => $view->name . '-weekno-' . $curday_date,  
      'class' => 'week');
  }
  for ($i = 0; $i < 7; $i++) {
    $curday_date = date_format($curday, DATE_FORMAT_DATE); 
    $class = strtolower($weekdays[$i] .
    ($view->date_info->mini ? ' mini' : ''));
    if ($check_month && ($curday_date < $view->date_info->min_date_date || $curday_date > $view->date_info->max_date_date || date_format($curday, 'n') != $month)) {
      $class .= ' empty';
      $variables = array(
        'curday' => $curday_date, 
        'view' => $view,
      );
      $content = array(
        'date' => '', 
        'datebox' => '', 
        'empty' => theme('calendar_empty_day', $variables), 
        'link' => '',
        'all_day' => array(), 
        'items' => array(),
        );
    }
    else {
      $content = calendar_build_day($curday, $view, $items);
      $class .= ($curday_date == $today ? ' today' : '') . ($curday_date < $today ? ' past' : '') . ($curday_date > $today ? ' future' : '');
    }

    $rows[$week][] = array(
      'data' => $content,
      'class' => $class, 'id' => $view->name . '-' . $curday_date);
    date_modify($curday, '+1 day');
  }
  return $rows;
}

/**
 * Build the contents of a single day for the $rows results.
 */
function calendar_build_day($curday, $view, $items) {
  $curday_date = date_format($curday, DATE_FORMAT_DATE);
  $selected = FALSE;
  $max_events = !empty($view->date_info->style_max_items) ? $view->date_info->style_max_items : 0;
  $types = array();
  $inner = array();
  $all_day = array();
  $empty = '';
  $link = '';
  $count = 0;

  foreach ($items as $date => $day) {
    if ($date == $curday_date) {
      $count = 0;
      $selected = TRUE;
      ksort($day);
      foreach ($day as $time => $hour) {
        foreach ($hour as $key => $item) {
          $count++;

          // @TODO $node->type may or may not exist any more, or be relevant.
          // Dig into the theme code to either remove it or get it working differently.
          if (isset($item->type)) {
            $types[$item->type] = $item;
          }
          if (!$view->date_info->mini && (empty($max_events) || ($max_events > 0 && $count <= $max_events))) {
            // Theme the item here unless this is a 'Day' or 'Week' view.
            // Day and week views need to do more processing before rendering
            // the item, so just past them the unrendered item.
            $theme = isset($item->calendar_node_theme) ? $item->calendar_node_theme : 'calendar_' . $view->date_info->granularity . '_node';
            $variables = array(
              'node' => $item, 
              'view' => $view,
            );
            if ($item->calendar_all_day) {
              $all_day[] = in_array($view->date_info->calendar_type, array('day', 'week')) ? $item : theme($theme, $variables);
            }
            else {
              $key = date_format($item->calendar_start_date, 'H:i:s');
              $inner[$key][] = in_array($view->date_info->calendar_type, array('day', 'week')) ? $item : theme($theme, $variables);
            }
          }
        }
      }
    }
  }
  ksort($inner);
  if (empty($inner) && empty($all_day)) {
    $variables = array(
      'curday' => $curday_date, 
      'view' => $view,
    );
    $empty = theme('calendar_empty_day', $variables);
  }
  // we have too many events on this day. use the theme('calendar_multiple_')
  if ($max_events > 0 && $count > $max_events && $view->date_info->calendar_type != 'day' && !$view->date_info->mini) {
    $variables = array(
      'curday' => $curday_date, 
      'count' => $count, 
      'view' => $view, 
      'types' => $types,
    );
    if ($view->date_info->style_max_items_behavior == 'hide') {
      $inner = array();
    }
    $link = theme('calendar_' . $view->date_info->calendar_type . '_multiple_node', $variables);
  }

  $variables = array(
    'date' => $curday_date, 
    'view' => $view, 
    'items' => $items, 
    'selected' => $selected,
  ); 
  $content = array(
    'date' => $curday_date, 
    'datebox' => theme('calendar_datebox', $variables), 
    'empty' => $empty,
    'link' => $link,
    'all_day' => $all_day,
    'items' => $inner,
    );
  return $content;
}
