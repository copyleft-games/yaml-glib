/* yaml-private.h
 *
 * Copyright 2025 Zach Podbielniak
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Private declarations shared between yaml-glib source files.
 * This header should not be installed or used by external code.
 */

#ifndef __YAML_PRIVATE_H__
#define __YAML_PRIVATE_H__

#include <glib.h>
#include "yaml-types.h"
#include "yaml-node.h"
#include "yaml-mapping.h"
#include "yaml-sequence.h"

G_BEGIN_DECLS

/*
 * YamlNode internal structure.
 * This is the actual layout of a YamlNode.
 */
struct _YamlNode
{
    volatile gint ref_count;
    YamlNodeType  type;
    gboolean      immutable;

    /* Parent node (weak reference, not ref-counted to avoid cycles) */
    YamlNode     *parent;

    /* YAML-specific metadata */
    gchar        *tag;
    gchar        *anchor;

    /*
     * Comments attached to this node.
     *
     * libyaml discards comments entirely -- they are not tokens and never
     * reach the document API -- so these are recovered by the parser from
     * the source text, anchored by line number, and are only populated when
     * yaml_parser_set_capture_comments() asked for them.
     *
     * leading_comments holds the run of whole-line comments immediately
     * above the node, in source order, without the leading '#'.  A blank
     * line inside that run is preserved as an empty string so round-tripping
     * keeps the author's paragraph breaks.
     *
     * trailing_comment is a comment that shared the node's own line.
     */
    GPtrArray    *leading_comments;  /* gchar*, or NULL when there are none */
    gchar        *trailing_comment;

    /*
     * Whether a blank line sat above this node (or above its comment block)
     * in the source.  Without it, every blank line between sections is lost
     * on the first write-back and a config file people navigate visually
     * collapses into one dense block.
     */
    gboolean      blank_before;

    /* Type-specific data */
    union {
        YamlMapping  *mapping;
        YamlSequence *sequence;
        struct {
            gchar           *value;
            YamlScalarStyle  style;
            /* Cached typed value for scalars */
            gboolean         has_int;
            gint64           int_value;
            gboolean         has_double;
            gdouble          double_value;
            gboolean         has_boolean;
            gboolean         boolean_value;
        } scalar;
    } data;
};

/*
 * YamlMapping internal structure.
 */
struct _YamlMapping
{
    volatile gint  ref_count;
    gboolean       immutable;
    GHashTable    *members;  /* gchar* -> YamlNode* */
    /* Preserve insertion order for consistent output */
    GPtrArray     *keys_order;
};

/*
 * YamlSequence internal structure.
 */
struct _YamlSequence
{
    volatile gint  ref_count;
    gboolean       immutable;
    GPtrArray     *elements;  /* YamlNode* */
};

/*
 * Internal helper to free node contents without freeing the node itself.
 * Used during reinitialization.
 */
void
yaml_node_clear_internal(YamlNode *node);

/*
 * Internal helper to parse scalar string to typed values.
 * Populates the cached int/double/boolean values.
 */
void
yaml_node_parse_scalar_internal(YamlNode *node);

/*
 * Internal helper to set parent on child nodes.
 */
void
yaml_node_set_parent_internal(
    YamlNode *child,
    YamlNode *parent
);

G_END_DECLS

#endif /* __YAML_PRIVATE_H__ */
