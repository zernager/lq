<?php
// $Id: og_group_from_node.inc,v 1.1.2.1 2010/12/15 09:14:11 amitaibu Exp $

/**
 * @file
 * Plugin to provide an relationship handler for node from user.
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t('Group from node'),
  'keyword' => 'og_group',
  'description' => t('Get the group context from a node.'),
  'required context' => new ctools_context_required(t('Node'), 'node'),
  'context' => 'og_group_from_node_context',
);

/**
 * Return a new context based on an existing context.
 */
function og_group_from_node_context($context, $conf) {
  // If unset it wants a generic, unfilled context, which is just NULL.
  if (empty($context->data) || !isset($context->data->nid)) {
    return ctools_context_create_empty('og_group', NULL);
  }

  if (isset($context->data->nid)) {
    $group = og_get_group('node', $context->data->nid);

    // Send it to ctools.
    return ctools_context_create('og_group', $group);
  }
}
