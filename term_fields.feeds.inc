<?php
// $Id$

/**
 * Implementation hook_ctools_plugin_api()
 */
function term_fields_ctools_plugin_api($owner, $api) {
  if ($owner == 'feeds' && $api == 'plugins') {
    return array(
      'version' => 1,
      'path' => drupal_get_path('module', 'term_fields') . '/feeds',
    );
  }
}

/**
 * A hook_feeds_plugins() declares available Fetcher, Parser or Processor
 * plugins to Feeds. For an example look at feeds_feeds_plugin(). For exposing
 * this hook hook_ctools_plugin_api() MUST be implemented, too.
 *
 * @see feeds_feeds_plugin()
 */
function term_fields_feeds_plugins() {
  $path = drupal_get_path('module', 'term_fields') . '/feeds';
  $info = array();
  
  $info['TermFieldsTermProcessor'] = array(
    'name' => 'Taxonomy term processor (with term fields)',
    'description' => 'Create taxonomy terms from parsed content. This is a extended Taxonomy term processor.',
    // 'help' => 'More verbose description here. Will be displayed on processor selection menu.',
    'handler' => array(
      'parent' => 'FeedsTermProcessor',
      'class' => 'TermFieldsTermProcessor',
      'file' => 'TermFieldsTermProcessor.inc',
      'path' => $path,
    ),
  );
  
  return $info;
}

/**
 * Alter mapping targets for taxonomy terms. Use this hook to add additional
 * target options to the mapping form of Taxonomy term processor.
 *
 * For an example implementation, look at geotaxnomy module.
 * http://drupal.org/project/geotaxonomy
 *
 * @param &$targets
 *   Array containing the targets to be offered to the user. Add to this array
 *   to expose additional options. Remove from this array to suppress options.
 *   Remove with caution.
 * @param $vid
 *   The vocabulary id
 */
function term_fields_feeds_term_processor_targets_alter(&$targets, $vid) {
  $fields = term_fields_get_rows($vid);
  foreach($fields as $field) {
    $targets[$field->fid] = array(
      'name' => 'Term field: ' . $field->title,
      'description' => $field->description,
      'optional_unique' => TRUE,
      'callback' => 'term_fields_feeds_term_set_target'
    );
  }
}

/**
 * Callback for mapping. Here is where the actual mapping happens.
 *
 * When the callback is invoked, $target contains the name of the field the
 * user has decided to map to and $value contains the value of the feed item
 * element the user has picked as a source.
 */
function term_fields_feeds_term_set_target(&$term, $target, $value) {
  $term[$target] = $value;
}

/**
 * Helper function. Load a taxonomy term and merge with term fields.
 */
function term_fields_feeds_taxonomy_load($term) {
  $term_object = (object) $term;
  $fields = term_fields_get_fields($term_object);
  return array_merge($term, $fields);
}