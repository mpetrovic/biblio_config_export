<?php

/*
 * @file
 * Hooks and implementations for Features
 */

/**
 * Implementation of hook_features_export_options.
 */
function biblio_config_features_export_options() {
  $options = array(
    'biblio_field_type' => t('Biblio Field Types'),
    'biblio_field_type_data' => t('Biblio Field Type Data'),
    'biblio_contributor_type' => t('Biblio Contributer Type'),
    'biblio_contributor_type_data' => t('Biblio Contributer Type Data'),
  );

  return $options;
}

/**
 * Implementation of hook_features_export
 */
function biblio_config_features_export($data, &$export, $module_name) {
  $export['dependencies']['biblio'] = 'biblio';

  foreach ($data as $component) {
    $export['features']['biblio_config'][$component] = $component;
  }

  return array();
}

/**
 * Implementation of hook_features_export_render.
 */
function biblio_config_features_export_render($module_name, $data, $export = NULL) {
  $code = array();
  $code[] = '  $biblio_configs = array();';
  $code[] = '';
  foreach ($data as $table) {
    $item = _biblio_config_get_data($table);
    if (isset($item[$table])) {
      $code[] = '  $biblio_configs[] = '.features_var_export($item,'  ').';';
    }
  }
  $code[] = '  return $biblio_configs;';
  $code = implode("\n", $code);
  return array('biblio_config_features_default_settings' => $code);
}

/**
 * Gets the form config into an array
 * @param unknown_type $name
 */
function _biblio_config_get_data($table) {
  $data = array();
  $q = db_query("SELECT * FROM {".$table."}");
  while ($r = db_fetch_array($q)) {
    $arr = array();
    foreach ($r as $k => $v) {
      $arr[$k] = $v;
    }
    $data[$table][] = $arr;
  }

  return $data;
}

/**
 * Implementation of hook_features_rebuild().
 */
function biblio_config_features_rebuild($module) {
  $config = module_invoke($module, 'biblio_config_features_default_settings');

  $field_type_data = array();

  foreach ($config['biblio_field_type_data'] as $v) {
    $field_type_data[$v['ftdid']] = $v;
  }

  // take linkage tables and construct them with linkage to data tables

  $field_type = array();

  foreach ($config['biblio_field_type'] as $v) {
    $v['ftdid'] = &$field_type_data[$v['ftdid']];
    $v['cust_tdid'] = &$field_type_data[$v['cust_tdid']];
    $field_type = $v;
  }

  // add data tables to db, checking for collisions and updating self appropriately

  foreach ($field_type_data as &$v) {
    $table = 'biblio_field_type_data';

    $title = db_result(db_query("SELECT title FROM { $table } WHERE ftdid = %d", $v['ftdid']));
    if ($title === FALSE) {
      // this ftdid doesnt exist. its safe to add it.
      db_query("INSERT INTO { $table } VALUES (%d, '%s', '%s')", $v['ftdid'], $v['title'], $v['hint']);
    }
    elseif ($title == $v['title']) {
      // we're good to go. Update the hint and be done
      db_query("UPDATE { $table } SET hint = '%s' WHERE ftdid = %d", $v['hint'], $v['ftdid']);
    }
    else {
      // oh god collision!
      // look for an existing name

      $ftdid = db_result(db_query("SELECT ftdid FROM { table } WHERE title = '%s'", $v['ftdid']));
      if ($ftdid !== FALSE) {
        // we found the name with a different id
        $v['ftdid'] = $ftdid;
      }
      else {
        // the name doesn't exist and the ftdid is taken. add a new row entirely
        // why doesn't this table have auto increment if it's going to use unique integer fields?!
        $new_id = db_result(db_query("SELECT MAX(ftdid)+1 FROM { $table }"));
        $v['ftdid'] = $new_id;
        db_query("INSERT INTO { $table } VALUES (%d, '%s', '%s')", $v['ftdid'], $v['title'], $v['hint']);
      }
    }
  }

  // add linkage tables to db, checking for collisions and getting info from data tables as necessary

  // by this point, we know whether an ftdid points to the correct resource or not.
  // any matches we find for ftdid in the linkage table points to what we want it to, for sure.
  foreach ($field_type as $v) {
    $table = 'biblio_field_type';

    $act = array();
    foreach ($v as $k => $d) {
      if (is_array($d)) {
        $act[$k] = $d['ftdid'];
      }
      else {
        $act[$k] = $d;
      }
    }

    // field_type is something we can bulldoze. It's settings for form displays and layouts, not actual content
    $count = db_result(db_query("SELECT COUNT(*) FROM { $table } WHERE tid = %d AND fid = %d", $act['tid'], $act['fid']));
    if ($count > 0) {
      // row exists already. just update it
      db_query("UPDATE { $table } SET ftdid = %d, cust_tdid = %d, common = %d, autocomplete = %d, required = %d, weight = %d, visible = %d WHERE tid = %d AND fid = %d AND ftdid = %d", $act['ftdid'], $act['cust_tdid'], $act['common'], $act['autocomplete'], $act['required'], $act['weight'], $act['visible'], $act['tid'], $act['fid']);
    }
    else {
      db_query("INSERT INTO { $table } VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d)", $act['tid'], $act['fid'], $act['ftdid'], $act['cust_tdid'], $act['common'], $act['autocomplete'], $act['required'], $act['weight'], $act['visible']);
    }
  }


  // contributor type

  $contributor_type_data = array();

  foreach ($config['biblio_contributor_type_data'] as $v) {
    $contributor_type_data[$v['auth_type']] = $v;
  }

  $contributor_type = array();

  foreach ($config['biblio_contributor_type'] as $v) {
    $v['auth_type'] = &$contributor_type_data[$v['auth_type']];
    $contributor_type[] = $v;
  }

  foreach ($contributor_type_data as $v) {

    $table = 'biblio_contributor_type_data';

    $title = db_result(db_query("SELECT title FROM { $table } WHERE auth_type = %d", $v['auth_type']));
    if ($title === FALSE) {
      // this auth_type doesnt exist. its safe to add it.
      db_query("INSERT INTO { $table } VALUES (%d, '%s', '%s')", $v['auth_type'], $v['title'], $v['hint']);
    }
    elseif ($title == $v['title']) {
      // we're good to go. Update the hint and be done
      db_query("UPDATE { $table } SET hint = '%s' WHERE auth_type = %d", $v['hint'], $v['auth_type']);
    }
    else {
      // oh god collision!
      // look for an existing name

      $auth_type = db_result(db_query("SELECT auth_type FROM { table } WHERE title = '%s'", $v['auth_type']));
      if ($auth_type !== FALSE) {
        // we found the name with a different id
        $v['auth_type'] = $auth_type;
      }
      else {
        // the name doesn't exist and the auth_type is taken. add a new row entirely
        // this table has an auto increment. let's use it
        if (@db_query("INSERT INTO { $table } VALUES (0, '%s', '%s')", $v['auth_type'], $v['title'], $v['hint'])) {
          $v['auth_type'] = db_last_insert_id($table, 'auth_type');
        }
      }
    }
  }

  foreach ($contributor_type as $v) {
    $table = 'biblio_contributor_type';

    $act = array();
    foreach ($v as $k => $d) {
      if (is_array($d)) {
        $act[$k] = $d['auth_type'];
      }
      else {
        $act[$k] = $d;
      }
    }

    // contributor type isnt something we can just bulldoze. It's linked to by contributor, which is content
    $count = db_result(db_query("SELECT COUNT(*) FROM { $table } WHERE auth_category = %d AND biblio_type = %d AND auth_type = %d", $act['auth_category'], $ct['biblio_type'], $act['auth_type']));
    if ($count === 0) {
      // row doesn't exist. there's no additional data so just insert it as is
      db_query("INSERT INTO { $table } VALUES (%d, %d, %d)", $act['auth_category'], $ct['biblio_type'], $act['auth_type']);
    }
  }
}

/**
 * Puts the form config data into the database
 */
function _biblio_config_set_data($item) {
  $table = key($item);
  $schema = drupal_get_schema($table);
  $primary = $schema['primary key'];
  foreach ($item[$table] as $row) {
    $query = $where = $w_args = $u_args = $q_args = array();
    foreach ($row as $k => $v) {
      if (in_array($k, $primary)) {
        $where[] = "%s = ".(gettype($v)=="string"?"'%s'":"%d");
        $w_args[] = $k;
        $w_args[] = $v;
      }
      else {
        $query[] = "%s = ".(gettype($v)=="string"?"'%s'":"%d");
        $u_args[] = $k;
        $u_args[] = $v;
      }
    }

    $q_args = array_merge($u_args, $w_args);
    array_unshift($q_args,  "INSERT INTO {".$table."} SET ".implode(', ',array_merge($query, $where)));

    // try inserting
    if (!@call_user_func_array('db_query', $q_args) && count($query)) {
      // it failed, so update, but only if all the fields arent primary keys.
      // if all the fields are primary keys, they exist and there's no other parameters so we're good
      $q_args[0] = "UPDATE {".$table."} SET ".implode(', ',$query)." WHERE ".implode(' AND ',$where);
      call_user_func_array('db_query', $q_args);
    }
  }
}

/**
 * Implementation of hook_features_revert.
 */
function biblio_config_features_revert($module) {
  biblio_config_features_rebuild($module);
}