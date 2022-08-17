//
// Test Environment: xml2html multidocument utility
//
// Filters for log messages in HTML log
//
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
//
//
//

// NOTE: for formatting of this file
// clang-format --style=LLVM -i <filename>
// was used.

// Session can have multiple log tables (one for messages before
// tests, another for messages after tests, for instance), this
// array includes rows from all such tables, allowing to filter
// them all at once.
var log_tables_rows = [];

// Update visibility of a row in table of log messages.
function update_row_visibility(table_row) {
  var nested_visible = table_row.getAttribute('data-nest-visible');
  var eu_visible = table_row.getAttribute('data-eu-visible');

  //
  // We have two filters working in parallel: one according to row
  // nesting level, another one according to entity/user. We show
  // a row only if both filters approve it.
  // null is treated as "filter does not care".
  //

  if ((nested_visible === null || nested_visible == 'yes') &&
      (eu_visible === null || eu_visible == 'yes'))
    table_row.style.display = '';
  else
    table_row.style.display = 'none';
}

//////////////////////////////////////////////////////////////////////////
// Implementation of filter based on messages nesting level
//////////////////////////////////////////////////////////////////////////

function hide_nested_row(table_row) {
  table_row.setAttribute('data-nest-visible', 'no');
  update_row_visibility(table_row);
}

function show_nested_row(table_row) {
  table_row.setAttribute('data-nest-visible', 'yes');
  update_row_visibility(table_row);
}

function get_row_entity(row) { return row.getAttribute('data-entity'); }

function is_same_entity(row1, row2) {
  return (get_row_entity(row1) == get_row_entity(row2));
}

function nested_user_ignored(row) {
  // Entries with these users are not touched by nesting filter
  return ((row.getAttribute('data-user') == "Verdict") ||
          (row.getAttribute('data-user') == "Artifact"));
}

function get_row_nest_lvl(row) {
  return parseInt(row.getAttribute('data-nest-lvl'));
}

function is_nested(row1, row2) {
  return (get_row_nest_lvl(row1) < get_row_nest_lvl(row2));
}

function get_nested_rows(row_id) {
  var row = log_tables_rows[row_id];

  var nested_rows = new Array();
  for (var nested_row_id = row_id + 1; nested_row_id < log_tables_rows.length;
       nested_row_id++) {
    if (nested_user_ignored(log_tables_rows[nested_row_id])) {
      continue;
    }
    if (!is_same_entity(row, log_tables_rows[nested_row_id])) {
      continue;
    }
    if (!is_nested(row, log_tables_rows[nested_row_id])) {
      break;
    }
    nested_rows.push(log_tables_rows[nested_row_id]);
  }
  return nested_rows;
}

function collapse_nested_rows(row_id) {
  var nested_rows = get_nested_rows(row_id);
  for (var idx = 0; idx < nested_rows.length; idx++) {
    hide_nested_row(nested_rows[idx]);
  }
  show_expand_button(row_id);
}

function expand_nested_rows(row_id) {
  var nested_rows = get_nested_rows(row_id);
  for (var idx = 0; idx < nested_rows.length; idx++) {
    show_nested_row(nested_rows[idx]);
  }
  show_collapse_button(row_id);
}

// Get HTML element in which to place a toggle button for a given row.
function get_log_toggle(row_id) {
  var row = log_tables_rows[row_id];
  var id = row.id;

  id = id.replace(/^line/, "");
  return document.getElementById("log_toggle" + id);
}

function show_collapse_button(row_id) {
  var holder = get_log_toggle(row_id);
  holder.innerHTML =
      '<input type="button" class="btn btn-default btn-sm te_fold" value="-" onclick="collapse_nested_rows(' +
      row_id + ');">';
}

function show_expand_button(row_id) {
  var holder = get_log_toggle(row_id);
  holder.innerHTML =
      '<input type="button" class="btn btn-success btn-sm te_fold" value="+" onclick="expand_nested_rows(' +
      row_id + ');">';
}

// Update style of buttons of the nested filter, showing in "on" state those
// which are currently enabled.
function update_nested_filter_buttons(nest_lvl) {
  var buttons = document.getElementsByClassName("te_nf_button");

  for (var i = 0; i < buttons.length; i++) {
    if (i <= nest_lvl || nest_lvl < 0) {
      buttons[i].className =
          buttons[i].className.replace(/\bbtn-default\b/, "btn-success");
    } else {
      buttons[i].className =
          buttons[i].className.replace(/\bbtn-success\b/, "btn-default");
    }
  }
}

function process_nested_rows(nest_lvl) {
  for (var row_id = 0; row_id < log_tables_rows.length; row_id++) {
    show_nested_row(log_tables_rows[row_id]);
  }

  for (var row_id = 0; row_id < log_tables_rows.length - 1; row_id++) {
    var nested_rows = get_nested_rows(row_id);
    if (nested_rows.length > 0) {
      show_collapse_button(row_id);
      if ((nest_lvl >= 0) &&
          (nest_lvl == get_row_nest_lvl(log_tables_rows[row_id]))) {
        collapse_nested_rows(row_id);
      }
    }
  }

  update_nested_filter_buttons(nest_lvl);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Implementation of filter based on entity and user of log message
//////////////////////////////////////////////////////////////////////////

// Map of maps - first level by entity, second - by user. Value tells
// whether messages with given entity and user are visible.
var eu_visibility = new Map();
// Array of button style update handlers. A button has a different style
// depending on visibility of messages related to it.
var eu_buttons_updaters = [];

// Entity used for the test logs (set by init_entity_user_filter()).
var test_entity = null;
// Shortcut for test entity name used in text on buttons.
var test_entity_short = '[#T]';

// If enabled, do not hide ERROR messages even if they should be filtered
// out by entity/user filter.
var eu_no_hide_errors = true;

// Array of entity/user pairs which are related to test scenario
// (filled by init_entity_user_filter()).
var eu_scenario_def;

function hide_eu_row(table_row) {
  table_row.setAttribute('data-eu-visible', 'no');
  update_row_visibility(table_row);
}

function show_eu_row(table_row) {
  table_row.setAttribute('data-eu-visible', 'yes');
  update_row_visibility(table_row);
}

// Update state of all buttons of entity/user filter according to
// current visibility configuration.
function update_eu_buttons() {
  var i;

  for (i = 0; i < eu_buttons_updaters.length; i++) {
    eu_buttons_updaters[i]();
  }
}

// Update visibility of all log messages whose entity and user
// are in a given map.
function set_eu_visibility(eu_map) {
  if (eu_map.size == 0)
    return;

  for (var row_id = 0; row_id < log_tables_rows.length; row_id++) {
    var row = log_tables_rows[row_id];
    var row_entity = row.getAttribute('data-entity');
    var row_user = row.getAttribute('data-user');
    var row_level = row.getAttribute('data-level');

    if (eu_map.has(row_entity)) {
      var users_map = eu_map.get(row_entity);

      if (users_map.has(row_user)) {
        if (users_map.get(row_user) ||
            (eu_no_hide_errors && row_level == "ERROR"))
          show_eu_row(row);
        else
          hide_eu_row(row);
      }
    }
  }

  update_eu_buttons();
}

function eu_button_update_style(el, visible, enabled_style) {
  // Disabled button
  if (visible === null)
    return;

  if (visible) {
    el.className = el.className.replace(/\bbtn-default\b/g, enabled_style);
  } else {
    var re = RegExp("\\b" + enabled_style + "\\b", "g");
    el.className = el.className.replace(re, 'btn-default');
  }
}

function eu_gen_button_class(state) {
  // state is visibility state of log messages configured
  // by this button. null means there is no such messages
  // and button should be disabled.
  if (state === null)
    return 'disabled';
  else if (state)
    return 'btn-primary';
  else
    return 'btn-default';
}

//////////////////////////////////////////////////////////////////////////
// Functions related to "ALL" button of entity/user filter
//////////////////////////////////////////////////////////////////////////

// Get visibility state for "ALL" button of entity/user filter.
// That button is displayed in "on" state only if all entity/user
// combinations are visible.
// Returns null if the button is disabled because there is no
// messages in the log.
function eu_all_state() {
  var count = 0;

  for (var [entity, users] of eu_visibility) {
    for (var [user, val] of users) {
      count++;
      if (!val) {
        return false;
      }
    }
  }

  if (count == 0)
    return null;
  return true;
}

// Get HTML code for "ALL" button of entity/user filter.
function eu_all_button() {
  var btn_class;

  btn_class = eu_gen_button_class(eu_all_state());

  return '<input type="button" class="btn ' + btn_class + ' btn-sm ' +
         'te_filter_button" id="eu_all_button" value="#ALL" ' +
         'onclick="eu_all_button_onclick();">';
}

// Process OnClick event for "ALL" button of entity/user filter.
function eu_all_button_onclick() {
  var m = new Map();
  var new_visibility = eu_all_state();

  if (new_visibility === null)
    return;
  else
    new_visibility = !new_visibility;

  for (var [entity, users] of eu_visibility) {
    for (var [user, val] of users) {
      if (val != new_visibility) {
        users.set(user, new_visibility);
        if (!m.has(entity))
          m.set(entity, new Map());
        m.get(entity).set(user, new_visibility);
      }
    }
  }

  set_eu_visibility(m);
}

// Update style of "ALL" button after changes in visibility configuration.
function eu_all_button_update() {
  var el = document.getElementById('eu_all_button');
  var visible = eu_all_state();

  eu_button_update_style(el, visible, "btn-primary");
}

//////////////////////////////////////////////////////////////////////////
// Functions related to "SCENARIO" button of entity/user filter
//////////////////////////////////////////////////////////////////////////

// Get visibility state for "SCENARIO" button of entity/user filter.
// That button is displayed in "on" state only if all entity/user
// combinations related to test scenario are visible.
// Returns null if the button is disabled because there is no
// messages related to scenario in the log.
function eu_scenario_state() {
  var count = 0;

  for (var spec of eu_scenario_def) {
    if (eu_visibility.has(spec.entity)) {
      var users = eu_visibility.get(spec.entity);
      if (users.has(spec.user) && !users.get(spec.user))
        return false;

      count++;
    }
  }

  if (count == 0)
    return null;
  return true;
}

// Get HTML code for "SCENARIO" button of entity/user filter.
function eu_scenario_button() {
  var btn_class;

  btn_class = eu_gen_button_class(eu_scenario_state());

  return '<input type="button" class="btn ' + btn_class + ' btn-sm ' +
         'te_filter_button" id="eu_scenario_button" value="#SCENARIO" ' +
         'onclick="eu_scenario_button_onclick();">';
}

// Process OnClick event for "SCENARIO" button of entity/user filter.
function eu_scenario_button_onclick() {
  var m = new Map();
  var new_visibility = eu_scenario_state();

  if (new_visibility === null)
    return;
  else
    new_visibility = !new_visibility;

  for (var spec of eu_scenario_def) {
    if (eu_visibility.has(spec.entity)) {
      var users = eu_visibility.get(spec.entity);
      if (users.has(spec.user) && users.get(spec.user) != new_visibility) {
        users.set(spec.user, new_visibility);
        if (!m.has(spec.entity))
          m.set(spec.entity, new Map());
        m.get(spec.entity).set(spec.user, new_visibility);
      }
    }
  }

  set_eu_visibility(m);
}

// Update style of "SCENARIO" button after changes in visibility
// configuration.
function eu_scenario_button_update() {
  var el = document.getElementById('eu_scenario_button');
  var visible = eu_scenario_state();

  eu_button_update_style(el, visible, "btn-primary");
}

//////////////////////////////////////////////////////////////////////////
// Functions related to "TEST" button of entity/user filter
//////////////////////////////////////////////////////////////////////////

// Get visibility state for "TEST" button of entity/user filter.
// That button is displayed in "on" state only if all entity/user
// combinations with test entity are visible.
// Returns null if the button is disabled because there is no
// messages with the test entity in the log.
function eu_test_state() {
  var count = 0;

  if (eu_visibility.has(test_entity)) {
    var users = eu_visibility.get(test_entity);
    for (var [user, val] of users) {
      if (!val)
        return false;

      count++;
    }
  }

  if (count == 0)
    return null;
  return true;
}

// Get HTML code for "TEST" button of entity/user filter.
function eu_test_button() {
  var btn_class;

  btn_class = eu_gen_button_class(eu_test_state());

  return '<input type="button" class="btn ' + btn_class + ' btn-sm ' +
         'te_filter_button" id="eu_test_button" value="#TEST" ' +
         'onclick="eu_test_button_onclick();">';
}

// Process OnClick event for "TEST" button of entity/user filter.
function eu_test_button_onclick() {
  var m_users = new Map();
  var m = new Map([ [ test_entity, m_users ] ]);
  var new_visibility = eu_test_state();

  if (new_visibility === null)
    return;
  else
    new_visibility = !new_visibility;

  if (eu_visibility.has(test_entity)) {
    var users = eu_visibility.get(test_entity);
    for (var [user, val] of users) {
      if (val != new_visibility) {
        users.set(user, new_visibility);
        m_users.set(user, new_visibility);
      }
    }
  }

  set_eu_visibility(m);
}

// Update style of "TEST" button after changes in visibility configuration.
function eu_test_button_update() {
  var el = document.getElementById('eu_test_button');
  var visible = eu_test_state();

  eu_button_update_style(el, visible, "btn-primary");
}

//////////////////////////////////////////////////////////////////////////
// Functions related to "ERROR" button of entity/user filter
//////////////////////////////////////////////////////////////////////////

// Get HTML code for "ERROR" button of entity/user filter.
function eu_error_button() {
  var btn_class;

  btn_class = (eu_no_hide_errors ? 'btn-danger' : 'btn-default');

  return '<input type="button" class="btn ' + btn_class + ' btn-sm ' +
         'te_filter_button" id="eu_error_button" value="ERROR" ' +
         'onclick="eu_error_button_onclick(this);">';
}

// Process OnClick event for "ERROR" button of entity/user filter.
function eu_error_button_onclick(el) {
  eu_no_hide_errors = !eu_no_hide_errors;

  set_eu_visibility(eu_visibility);
  eu_button_update_style(el, eu_no_hide_errors, "btn-danger");
}

//////////////////////////////////////////////////////////////////////////
// Functions related to regular buttons of entity/user filter
//////////////////////////////////////////////////////////////////////////

// Get handler which can be called to update style of a regular button
// after changing visibility configuration.
function eu_button_updater(entity, user, btn_id) {

  // Here Javascript closure is used. Current values of local
  // variables are remembered in an environment of the function
  // created and returned here, so that a different function
  // specific to the current entity/user is returned every time.
  function updater() {
    var visible;
    var el = document.getElementById(btn_id);

    if (eu_visibility.has(entity)) {
      var users = eu_visibility.get(entity);
      if (users.has(user)) {
        visible = users.get(user);
        eu_button_update_style(el, visible, "btn-primary");
      }
    }
  }

  return updater;
}

// Get HTML code of a button configuring visibility for all log messages
// with given entity and user.
function eu_button(entity, user, btn_id) {
  var visible = eu_visibility.get(entity).get(user);
  var btn_class;
  var entity_name = entity;

  if (entity == test_entity)
    entity_name = test_entity_short;

  btn_class = (visible ? 'btn-primary' : 'btn-default');

  return '<input type="button" class="btn ' + btn_class + ' btn-sm ' +
         'te_filter_button" value="' + entity_name + ':' + user +
         '" onclick="eu_button_onclick(\'' + entity + '\', \'' + user +
         '\');" id="' + btn_id + '">';
}

// Process OnClick event for a regular button of entity/user filter.
function eu_button_onclick(entity, user) {
  var m = new Map();
  var users = eu_visibility.get(entity);
  var users_aux = new Map();
  var new_visibility = !users.get(user);

  users.set(user, new_visibility);
  users_aux.set(user, new_visibility);
  m.set(entity, users_aux);
  set_eu_visibility(m);

  update_eu_buttons();
}

// Initialize variables used by entity/user filter, create buttons for it.
function init_entity_user_filter() {
  var btns_row;
  var btns_descr;
  var btns_list;
  var btn_num = 0;
  var btn_id;
  var fixed_entities = new Map();

  var table_btns = document.getElementById('filter_buttons');
  var tables = document.getElementsByClassName('te_log_table');

  for (var table_id = 0; table_id < tables.length; table_id++) {
    var table = tables[table_id];

    // 0th row is table header, so start with index 1.
    for (var row_id = 1; row_id < table.rows.length; row_id++) {
      log_tables_rows.push(table.rows[row_id]);
    }
  }

  // Get all pairs of entity/user, set them all visible by
  // default.
  for (var row_id = 0; row_id < log_tables_rows.length; row_id++) {
    var row = log_tables_rows[row_id];
    var entity = row.getAttribute('data-entity');
    var user = row.getAttribute('data-user');
    var users_map;

    if (!eu_visibility.has(entity))
      eu_visibility.set(entity, new Map());
    users_map = eu_visibility.get(entity);

    users_map.set(user, true);
    row.setAttribute('data-eu-visible', 'yes');

    // Assume that "TAPI Jumps" logs are always present and
    // go only under test entity, so that test entity can
    // be determined from such logs.
    if (test_entity === null && user == 'TAPI Jumps')
      test_entity = entity;
  }

  // Logs with such entity/user pairs will be assigned to
  // "SCENARIO" button.
  eu_scenario_def = [];
  eu_scenario_def.push({entity : test_entity, user : 'Step'});
  eu_scenario_def.push({entity : test_entity, user : 'Artifact'});
  eu_scenario_def.push({entity : test_entity, user : 'Self'});
  eu_scenario_def.push({entity : 'Tester', user : 'Run'});

  btns_row = table_btns.insertRow(-1);
  btns_descr = btns_row.insertCell(0);
  btns_list = btns_row.insertCell(1);
  btns_descr.innerHTML += 'Entity/User:';

  // Create map of entity displayed names to entity real names
  // (only for test entity displayed name differs from real name).
  for (var entity of eu_visibility.keys()) {
    var entity_name = entity;

    if (entity == test_entity)
      entity_name = test_entity_short;

    fixed_entities.set(entity_name, entity);
  }

  // Add a button for each entity/user pair, ordering them
  // in alphabetical order of displayed entity names and
  // user names.
  for (var entity of Array.from(fixed_entities.keys()).sort()) {
    var real_entity = fixed_entities.get(entity);
    var users = eu_visibility.get(real_entity);
    for (var user of Array.from(users.keys()).sort()) {
      btn_id = "entity_user_btn_" + btn_num;
      btn_num++;

      btns_list.innerHTML += ' ' + eu_button(real_entity, user, btn_id);
      eu_buttons_updaters.push(eu_button_updater(real_entity, user, btn_id));
    }
  }

  // Add SCENARIO, TEST, ALL and ERROR buttons.
  // Make sure that they all are on the same line.
  btns_list.innerHTML += '<br><span style="white-space: nowrap;">' +
                         eu_scenario_button() + '&nbsp;' + eu_test_button() +
                         '&nbsp;' + eu_all_button() + '&nbsp;&nbsp;&nbsp;' +
                         eu_error_button() + '</span>';

  eu_buttons_updaters.push(eu_scenario_button_update);
  eu_buttons_updaters.push(eu_test_button_update);
  eu_buttons_updaters.push(eu_all_button_update);
}

function init_log_filter() {
  init_entity_user_filter();
  process_nested_rows(-1);
}
