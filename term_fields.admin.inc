<?php

// $Id$

/**
 * Administration page for Term Fields. Contains all the forms for editing fields.
 */
function term_fields_admin() {
	$form = array();
	$vocabs = taxonomy_get_vocabularies();
	$vocabularies = array();
	foreach ($vocabs as $vid => $vocab) {
		$vocabularies[$vid] = $vocab->name;
	}
  
  $result = pager_query('SELECT tf.fid AS fid, tf.title AS title, tf.description AS description, v.name AS name
  										FROM {term_fields} tf
  										LEFT JOIN {vocabulary} v ON tf.vid = v.vid
  										ORDER BY v.name, tf.title', variable_get('term_fields_per_page', 25));
  if ($fields = term_fields_fetch_fields($result)) {
  	$header = array('Field', 'Vocabulary', 'Description');
  	$output .= theme('table', $header, $fields, array('id' => 'term-fields-admin'));
  	$output .= theme('pager');
  	return $output;
  }
  $output = t('There are no fields defined yet.');
  return $output;
}

/**
 * Administration page for Term Fields. Provides form for creating new fields.
 */
function term_fields_admin_new() {
	$form = array();
	$vocabs = taxonomy_get_vocabularies();
	if (!empty($vocabs)) {
		$vocabs_select = array();
		foreach ($vocabs as $key => $value) {
			$vocabs_select[$key] = $value->name;
		}
	
		$form['vid'] = array(
			'#type' => 'select',
			'#title' => t('Vocabulary'),
			'#options' => $vocabs_select,
			'#description' => t('Choose a vocabulary to assign this field to. Only terms in this vocabulary will
												be able to use this field. Cannot be changed later.')
		);
		$form['fid'] = array(
			'#type' => 'textfield',
			'#title' => t('Field ID'),
			'#description' => t('An ID used by Drupal to identify this field. It must be unique. Only alphanumeric characters and underscores allowed. Cannot be changed later.'),
			'#required' => TRUE,
			'#maxlength' => 32
		);
		$form['title'] = array(
			'#type' => 'textfield',
			'#title' => t('Title'),
			'#description' => t('The readable name of this field. Can be changed later.'),
			'#required' => TRUE,
			'#element_validate' => array('title_validate'),
			'#maxlength' => 50
		);
		$form['description'] = array(
			'#type' => 'textfield',
			'#title' => t('Description'),
			'#description' => t('Enter a short description of this field to explain what the field is used for. Can be changed later.'),
			'#maxlength' => 100
		);
		$form['type'] = array(
			'#type' => 'radios',
			'#title' => t('Type'),
			'#options' => array(
				'textfield' => t('Single row textbox'),
				'textarea' => t('Multi-row textarea'),
				'numeric' => t('Numeric'),
				'select' => t('Select box'),
				'date' => t('Date'),
				'file' => t('File')
			),
			'#description' => t('Define what type of field to use. Single line textboxes should
													be used for fields such as names and dates, while multi line textareas
													should be used for longer fields, such as biographies. Cannot be changed later.'),
			'#required' => TRUE
		);
		$form['weight'] = array(
			'#type' => 'weight',
			'#title' => t('Weight'),
			'#default_value' => 0,
			'#description' => t('Heaveier weighted items get put at the bottom of lists. Can be changed later.')
		);
		$form['fields']['submit'] = array(
			'#type' => 'submit',
			'#value' => t('Submit'),
		);
	}
	else {
		$form['empty'] = array(
			'#type' => 'item',
			'#value' => t('You have to !add before you can add fields.', array('!add' => l('add a vocabulary', 'admin/content/taxonomy/add/vocabulary')))
		);
	}
	return $form;
}

/**
 * Validate handler for term_fields_admin_new().
 */
function term_fields_admin_new_validate($form, &$form_state) {
	$values = $form_state['values'];
	if (!term_fields_check_acceptable($values['fid'])) {
		form_set_error('fid', t('The Field ID must only only contain alphanumeric characters and underscores'));
	}
	$bad_fids = array('name', 'description', 'numeric');
	$fids = term_fields_get_fids();
	if (!empty($fids)) {
		$bad_fids = $bad_fids + $fids;
	}
	if (in_array(strtolower($values['fid']), $bad_fids)) {
		form_set_error('fid', t('The field ID !fid cannot be used. It is either already being used by another field, or is a reserved internal name.', array('!fid' => '<i>' . $values['fid'] . '</i>')));
	}
}

/**
 * Submit handler for term_fields_admin_new().
 */
function term_fields_admin_new_submit($form, &$form_state) {
	$values = $form_state['values'];
	
	$values['fid'] = strtolower($values['fid']);
	
	// determine which data type to use for the new column
	switch ($values['type']) {
		case 'textfield':
		case 'textarea':
		case 'select':
			$data_type = 'LONGTEXT';
			break;
		case 'numeric':
		case 'file':
			$data_type = 'INT';
			break;
		case 'date':
			$data_type = 'DATETIME';
	}
	
	// success
	drupal_set_message(t($values['title'] . ' (' . $values['fid'] . ') was successfully created.'));
	
	// make a new record in the term_fields table
	// select type fields need a default value for the options column
	if ($values['type'] != 'select') {
		$sql = "INSERT INTO {term_fields} (fid, vid, title, description, type, weight)
						VALUES ('%s', %d, '%s', '%s', '%s', %d)";
		db_query($sql, $values['fid'], $values['vid'], $values['title'], $values['description'], $values['type'], $values['weight']);
	}
	else {
		$sql = "INSERT INTO {term_fields} (fid, vid, title, description, type, weight, options)
						VALUES ('%s', %d, '%s', '%s', '%s', %d, '%s')";
		db_query($sql, $values['fid'], $values['vid'], $values['title'], $values['description'], $values['type'], $values['weight'], 'a:0:{}');
		// redirect to the edit page and encourage the user to set the options for the select field
		drupal_set_message(t('You should now specify some options for your select field.'));
		$form_state['redirect'] = 'admin/content/taxonomy/term_fields/edit/' . $values['fid'];
	}
	
	// add the new column
	$sql = "ALTER TABLE {term_fields_term} ADD %s %s";
	db_query($sql, $values['fid'], $data_type);
}

/**
 * Settings page
 */
function term_fields_admin_settings() {
	$form = array();
	$formats = array(
		'Y-m-d', 'm/d/Y', 'd/m/Y', 'Y/m/d',
    'd.m.Y', 'm/d/Y', 'd/m/Y', 'Y/m/d',
    'M j Y', 'j M Y', 'Y M j',
    'M j Y', 'j M Y', 'Y M j'
	);
	
	$options = array();
	foreach ($formats as $format) {
		$options[$format] = format_date(time(), 'custom', $format);
	}
	$form['date_format'] = array(
		'#type' => 'select',
		'#title' => t('Date Field Format'),
		'#description' => t('The format to be used when displaying date fields.'),
		'#default_value' => variable_get('term_fields_date_format', ''),
		'#options' => $options
	);
	$form['custom'] = array(
		'#type' => 'textfield',
		'#title' => t('Custom Date Format'),
		'#default_value' => variable_get('term_fields_date_format_custom', ''),
		'#description' => t('A user-defined date format. If set, it will be used instead of what is set in the Date Field Format field. Leave blank to disable. See the <a href="@url">PHP manual</a> for available options.', array('@url' => 'http://php.net/manual/function.date.php')),
		'#size' => 5
	);
	$form['per_page'] = array(
		'#type' => 'textfield',
		'#title' => t('Fields Per Page'),
		'#description' => t('How many fields to show per page on the fields administration page.'),
		'#default_value' => variable_get('term_fields_per_page', 15),
		'#size' => 3
	);
	$form['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Submit')
	);
	return $form;
}

/**
 * Submit handler for settings form.
 */
function term_fields_admin_settings_submit($form, &$form_state) {
	$values = $form_state['values'];
	variable_set('term_fields_date_format', $values['date_format']);
	variable_set('term_fields_date_format_custom', $values['custom']);
	if (is_numeric($values['per_page'])) {
		variable_set('term_fields_per_page', $values['per_page']);
	}
	else {
		form_set_error('per_page', t('The <i>Fields Per Page</i> value must be numeric.'));
	}
}

/**
 * Administration page for Term Fields. Provides form for editing.
 */
function term_fields_admin_edit($form_state, $fid) {
	$form = array();

	$sql = "SELECT * FROM {term_fields} WHERE fid = '%s'";
	$field = db_fetch_object(db_query($sql, $fid));
	$form['title'] = array(
		'#type' => 'textfield',
		'#title' => t('Title'),
		'#default_value' => $field->title,
		'#description' => t('You can change the title of this field here.'),
		'#required' => TRUE,
		'#size' => 25,
		'#maxlength' => 50
	);
	$form['description'] = array(
		'#type' => 'textfield',
		'#title' => t('Description'),
		'#default_value' => $field->description,
		'#description' => t('You can change the description of this field here.')
	);
	if ($field->type == 'select') {
		$form['options'] = array(
			'#type' => 'textarea',
			'#title' => t('Options (one per line)'),
			'#description' => t('This field is a select box, so you have to set the options for it here. One option per line.'),
			'#default_value' => $field->options ? implode("\n", unserialize($field->options)) : ''
		);
	}
	elseif ($field->type == 'file') {
	  $options = unserialize($field->options);
	  $form['file_max_size'] = array(
	    '#type' => 'textfield',
	    '#title' => t('Maximum file size'),
	    '#description' => t('Maximum size of the file in megabytes. Default is 1 MB.'),
	    '#default_value' => isset($options['file_max_size']) ? $options['file_max_size'] : 1,
	    '#size' => 1,
	    '#required' => TRUE
	  );
	  $form['file_allowed_exts'] = array(
	    '#type' => 'textfield',
	    '#title' => t('Allowed extensions'),
	    '#description' => t('Specify the allowed file extensions for this field. Separate extensions by spaces, do not include leading dot.'),
	    '#default_value' => isset($options['file_allowed_exts']) ? implode(' ', $options['file_allowed_exts']) : 'txt',
	    '#size' => 60,
	    '#required' => TRUE
	  );
	}
	$form['weight'] = array(
		'#type' => 'weight',
		'#title' => t('Weight'),
		'#default_value' => $field->weight,
		'#description' => t('Fields weighted heavier appear at the bottom of lists.')
	);
	$form['fid'] = array(
		'#type' => 'hidden',
		'#value' => $fid
	);
	$form['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Submit')
	);
	$form['delete'] = array(
		'#type' => 'submit',
		'#value' => t('Delete')
	);

	drupal_set_title(drupal_get_title() . ' ' . $fid);

	return $form;
}

/**
 * Submit handler for term_fields_admin_edit().
 */
function term_fields_admin_edit_submit($form, &$form_state) {
	$values = $form_state['values'];

	if ($form_state['clicked_button']['#value'] == t('Delete')) {
    drupal_goto('admin/content/taxonomy/term_fields/delete/' . $values['fid']);
	}
	else {
		$sql = "SELECT type FROM {term_fields} WHERE fid = '%s'";
		$type = db_result(db_query($sql, $values['fid']));
		switch ($type) {
		  case 'select':
		    $options = explode("\r\n", $values['options']);
			  $options = drupal_map_assoc($options);
			  $options = serialize($options);
			  $sql = "UPDATE {term_fields} SET title = '%s', description = '%s', weight = %d, options = '%s' WHERE fid = '%s'";
			  db_query($sql, $values['title'], $values['description'], $values['weight'], $options, $values['fid']);
		    break;
		  case 'file':
		    $options = array();
		    $options['file_max_size'] = $values['file_max_size'];
		    $options['file_allowed_exts'] = explode(' ', $values['file_allowed_exts']);
		    $options = serialize($options);
		    $sql = "UPDATE {term_fields} SET title = '%s', description = '%s', weight = %d, options = '%s' WHERE fid = '%s'";
			  db_query($sql, $values['title'], $values['description'], $values['weight'], $options, $values['fid']);
		    break;
		  default:
		    $sql = "UPDATE {term_fields} SET title = '%s', description = '%s', weight = %d WHERE fid = '%s'";
			  db_query($sql, $values['title'], $values['description'], $values['weight'], $values['fid']);
		    break;
		}
	
		drupal_set_message(t('Field <i>' . $values['fid'] . '</i> was successfully updated.'));
		$form_state['redirect'] = 'admin/content/taxonomy/term_fields';
	}
}

/**
 * Provide a confirmation screen for deletions.
 */
function term_fields_admin_delete($form_state, $fid) {
	$form = array();
	$form['fid'] = array(
		'#type' => 'hidden',
		'#value' => $fid
	);
	$question = t('Are you sure you want to delete !fid?', array('!fid' => $fid));
	$path = 'admin/content/taxonomy/term_fields';
	$description = t('All instances of this field will be deleted. This action cannot be undone.');
	return confirm_form($form, $question, $path, $description);
}

/**
 * Term field deletion confirmation screen.
 */
function term_fields_admin_delete_submit($form, &$form_state) {
	$fid = $form_state['values']['fid'];
	
	// remove the record from term_fields
	$sql = "DELETE FROM {term_fields} WHERE fid = '%s'";
	db_query($sql, $fid);
	
	// remove the column from term_fields_term
	$sql = "ALTER TABLE {term_fields_term} DROP %s";
	db_query($sql, $fid);
	
	drupal_set_message(t('Field ' . $fid . ' was deleted.'));
	$form_state['redirect'] = 'admin/content/taxonomy/term_fields';
}