<?php
// $Id$

/**
 * @file
 * FeedsTermProcessor class.
 */

/**
 * Feeds processor plugin. Create taxonomy terms from feed items.
 */
class TermFieldsTermProcessor extends FeedsTermProcessor {
  /**
   * Get id of an existing feed item term if available.
   */
  protected function existingItemId(FeedsImportBatch $batch, FeedsSource $source) {
    $tid = parent::existingItemId($batch, $source);
    
    if ($tid == 0) {
      foreach ($this->uniqueTargets($batch) as $target => $value) {
        $query = "SELECT tid FROM {term_fields_term} AS tft, term_fields AS tf WHERE tf.fid = '%s' AND tf.vid = %d and tft.%s = '%s'";
        if ($tid = db_result(db_query($query, $target, $this->vocabulary()->vid, $target, $value))) {
          return $tid;
        }
      }
    }
    
    return $tid;
  }
}
