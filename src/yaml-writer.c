/* yaml-writer.c
 *
 * Copyright 2026 Zach Podbielniak
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "yaml-writer.h"
#include "yaml-private.h"
#include "yaml-mapping.h"
#include "yaml-sequence.h"

#include <string.h>

static void render_node(GString  *out,
                        YamlNode *node,
                        guint     depth,
                        guint     indent_spaces);

static void append_scalar_or_empty(GString  *out,
                                   YamlNode *node,
                                   guint     depth,
                                   guint     indent_spaces);

static void
append_indent(GString *out, guint depth, guint indent_spaces)
{
    guint i;
    guint width = depth * indent_spaces;

    for (i = 0; i < width; i++)
        g_string_append_c(out, ' ');
}

/*
 * A plain (unquoted) scalar is only safe when nothing about it could make a
 * reader -- or a parser -- take it for something else.  When in doubt this
 * says no and the value gets quoted, which is always correct and merely
 * less pretty.
 */
static gboolean
plain_scalar_is_safe(const gchar *value)
{
    static const gchar *const reserved[] = {
        "true", "false", "yes", "no", "on", "off",
        "null", "~", "y", "n", NULL
    };
    const gchar *p;
    gsize len;
    gsize i;

    if (value == NULL || *value == '\0')
        return FALSE;

    len = strlen(value);

    /* Leading or trailing space would be silently eaten on the way back in. */
    if (g_ascii_isspace(value[0]) || g_ascii_isspace(value[len - 1]))
        return FALSE;

    /*
     * An indicator in first position changes what the line means: '-' starts
     * a sequence entry, '#' a comment, '?' and ':' mapping structure, and
     * so on.
     */
    if (strchr("-?:,[]{}#&*!|>'\"%@`", value[0]) != NULL)
        return FALSE;

    /* These would come back as booleans or null rather than as strings. */
    for (i = 0; reserved[i] != NULL; i++)
    {
        if (g_ascii_strcasecmp(value, reserved[i]) == 0)
            return FALSE;
    }

    /* Anything that parses as a number would come back as one. */
    {
        gchar *end = NULL;

        g_ascii_strtod(value, &end);
        if (end != NULL && *end == '\0')
            return FALSE;
    }

    for (p = value; *p != '\0'; p++)
    {
        /* Control characters and newlines need a quoted or block form. */
        if ((guchar)*p < 0x20)
            return FALSE;

        /* ": " ends a key; " #" starts a comment.  Either splits the value. */
        if (p[0] == ':' && (p[1] == ' ' || p[1] == '\0'))
            return FALSE;
        if (p[0] == ' ' && p[1] == '#')
            return FALSE;
    }

    return TRUE;
}

/*
 * True for the values plain_scalar_is_safe() rejects purely because they
 * would come back as a bool, a number or null -- which is exactly right
 * when we do not know the author's intent, and exactly wrong when we do.
 * A scalar the source wrote plain meant that type.
 */
static gboolean
scalar_was_plain_typed(const gchar *value)
{
    static const gchar *const typed[] = {
        "true", "false", "yes", "no", "on", "off",
        "null", "~", NULL
    };
    gchar *end = NULL;
    gsize i;

    if (value == NULL || *value == '\0')
        return FALSE;

    for (i = 0; typed[i] != NULL; i++)
    {
        if (g_ascii_strcasecmp(value, typed[i]) == 0)
            return TRUE;
    }

    g_ascii_strtod(value, &end);
    return (end != NULL && *end == '\0');
}

/*
 * Single-quoted style is the one that needs no escaping beyond doubling the
 * quote itself: backslashes stay literal, which is what a Windows path or a
 * regex in a config file wants.
 */
static void
append_single_quoted(GString *out, const gchar *value)
{
    const gchar *p;

    g_string_append_c(out, '\'');
    for (p = value; *p != '\0'; p++)
    {
        if (*p == '\'')
            g_string_append(out, "''");
        else
            g_string_append_c(out, *p);
    }
    g_string_append_c(out, '\'');
}

/*
 * A multi-line string goes out as a literal block scalar.  Double-quoting
 * it with \n escapes would round-trip correctly and be unreadable, and
 * these are configuration files people read.
 *
 * The chomping indicator matters: without '-' on a value that does not end
 * in a newline, the parser adds one, and the value grows by a newline every
 * time the file is written back.
 */
static void
append_literal_block(GString     *out,
                     const gchar *value,
                     guint        depth,
                     guint        indent_spaces)
{
    g_auto(GStrv) lines = NULL;
    gboolean trailing_newline;
    guint i;
    guint n;

    trailing_newline = (*value != '\0' && value[strlen(value) - 1] == '\n');

    g_string_append(out, trailing_newline ? "|" : "|-");
    g_string_append_c(out, '\n');

    lines = g_strsplit(value, "\n", -1);
    n = g_strv_length(lines);

    /* g_strsplit leaves an empty final element for a trailing newline. */
    if (trailing_newline && n > 0)
        n--;

    for (i = 0; i < n; i++)
    {
        append_indent(out, depth + 1, indent_spaces);
        g_string_append(out, lines[i]);
        g_string_append_c(out, '\n');
    }
}

static void
append_scalar(GString  *out,
              YamlNode *node,
              guint     depth,
              guint     indent_spaces)
{
    const gchar *value;

    if (yaml_node_get_node_type(node) == YAML_NODE_NULL)
    {
        g_string_append(out, "null");
        return;
    }

    value = yaml_node_get_string(node);
    if (value == NULL)
    {
        g_string_append(out, "null");
        return;
    }

    if (strchr(value, '\n') != NULL)
    {
        append_literal_block(out, value, depth, indent_spaces);
        return;
    }

    /*
     * How it was written in the source wins over what we would pick.
     *
     * yaml-glib models every scalar as a string, so by the time it reaches
     * here `true` and `'true'` look identical -- and the safe guess quotes
     * both, which silently turns a boolean into a string on a round-trip.
     * The parser records the original style precisely so that does not
     * happen.  A node built programmatically has no style, and only then
     * does the heuristic decide.
     */
    switch (yaml_node_get_scalar_style(node))
    {
        case YAML_SCALAR_STYLE_PLAIN:
            /*
             * It came in plain, so it can go out plain -- unless it has
             * since been changed to something that would no longer parse
             * that way.
             */
            if (plain_scalar_is_safe(value) || scalar_was_plain_typed(value))
            {
                g_string_append(out, value);
                return;
            }
            break;

        case YAML_SCALAR_STYLE_SINGLE_QUOTED:
        case YAML_SCALAR_STYLE_DOUBLE_QUOTED:
            append_single_quoted(out, value);
            return;

        case YAML_SCALAR_STYLE_LITERAL:
        case YAML_SCALAR_STYLE_FOLDED:
        case YAML_SCALAR_STYLE_ANY:
        default:
            break;
    }

    if (plain_scalar_is_safe(value))
        g_string_append(out, value);
    else
        append_single_quoted(out, value);
}

/*
 * Writes a value on the same line as its key or dash.  An empty mapping or
 * sequence goes out in flow style: "{}" and "[]" are the only way to say
 * "present but empty" in block style, and losing that distinction turns an
 * explicitly empty agents list into a null one.
 */
static void
append_scalar_or_empty(GString  *out,
                       YamlNode *node,
                       guint     depth,
                       guint     indent_spaces)
{
    switch (yaml_node_get_node_type(node))
    {
        case YAML_NODE_MAPPING:
            g_string_append(out, "{}");
            break;

        case YAML_NODE_SEQUENCE:
            g_string_append(out, "[]");
            break;

        default:
            append_scalar(out, node, depth, indent_spaces);
            break;
    }
}

static void
append_leading_comments(GString  *out,
                        YamlNode *node,
                        guint     depth,
                        guint     indent_spaces)
{
    GPtrArray *comments = yaml_node_get_leading_comments(node);
    guint i;

    /*
     * The separator goes above the comment block, not between the block and
     * the key: the comment belongs to what follows it.
     *
     * Never at the very start of the output, or the file would grow a
     * leading blank line every time it is written.
     */
    if (yaml_node_get_blank_before(node) && out->len > 0)
        g_string_append_c(out, '\n');

    if (comments == NULL)
        return;

    for (i = 0; i < comments->len; i++)
    {
        const gchar *line = g_ptr_array_index(comments, i);

        append_indent(out, depth, indent_spaces);

        /*
         * An empty comment line is emitted as a bare '#' rather than a
         * blank line.  A blank line would end the comment run on the way
         * back in, and the block would come apart a little more with every
         * round-trip.
         */
        if (line == NULL || *line == '\0')
            g_string_append(out, "#\n");
        else
            g_string_append_printf(out, "# %s\n", line);
    }
}

static void
append_trailing_comment(GString *out, YamlNode *node)
{
    const gchar *comment = yaml_node_get_trailing_comment(node);

    if (comment != NULL && *comment != '\0')
        g_string_append_printf(out, "  # %s", comment);
}

/*
 * True when the node needs its own indented block beneath the key, rather
 * than sitting on the same line after "key: ".
 */
static gboolean
node_is_block(YamlNode *node)
{
    YamlNodeType type = yaml_node_get_node_type(node);

    if (type == YAML_NODE_MAPPING)
        return yaml_mapping_get_size(yaml_node_get_mapping(node)) > 0;

    if (type == YAML_NODE_SEQUENCE)
        return yaml_sequence_get_length(yaml_node_get_sequence(node)) > 0;

    return FALSE;
}

static void
render_mapping(GString  *out,
               YamlNode *node,
               guint     depth,
               guint     indent_spaces)
{
    YamlMapping *mapping = yaml_node_get_mapping(node);
    GList *keys;
    GList *l;

    keys = yaml_mapping_get_members(mapping);

    for (l = keys; l != NULL; l = l->next)
    {
        const gchar *key = l->data;
        YamlNode *value = yaml_mapping_get_member(mapping, key);

        if (value == NULL)
            continue;

        append_leading_comments(out, value, depth, indent_spaces);
        append_indent(out, depth, indent_spaces);

        if (plain_scalar_is_safe(key))
            g_string_append(out, key);
        else
            append_single_quoted(out, key);

        g_string_append_c(out, ':');

        if (node_is_block(value))
        {
            append_trailing_comment(out, value);
            g_string_append_c(out, '\n');
            render_node(out, value, depth + 1, indent_spaces);
        }
        else
        {
            g_string_append_c(out, ' ');
            append_scalar_or_empty(out, value, depth, indent_spaces);
            append_trailing_comment(out, value);
            g_string_append_c(out, '\n');
        }
    }

    g_list_free(keys);
}

static void
render_sequence(GString  *out,
                YamlNode *node,
                guint     depth,
                guint     indent_spaces)
{
    YamlSequence *sequence = yaml_node_get_sequence(node);
    guint size = yaml_sequence_get_length(sequence);
    guint i;

    for (i = 0; i < size; i++)
    {
        YamlNode *element = yaml_sequence_get_element(sequence, i);

        if (element == NULL)
            continue;

        append_leading_comments(out, element, depth, indent_spaces);
        append_indent(out, depth, indent_spaces);
        g_string_append(out, "- ");

        if (node_is_block(element))
        {
            /*
             * A block element starts on the dash's own line, so its first
             * child is written here at the current column and the rest
             * follow one level deeper.  Rendering into a scratch buffer and
             * splicing keeps that logic in one place instead of threading a
             * "first line already indented" flag through every level.
             */
            g_autoptr(GString) nested = g_string_new(NULL);
            const gchar *text;

            render_node(nested, element, depth + 1, indent_spaces);
            text = nested->str;

            /* Skip the first line's indentation; the "- " already covered it. */
            while (*text == ' ')
                text++;

            g_string_append(out, text);
        }
        else
        {
            append_scalar_or_empty(out, element, depth, indent_spaces);
            append_trailing_comment(out, element);
            g_string_append_c(out, '\n');
        }
    }
}

static void
render_node(GString  *out,
            YamlNode *node,
            guint     depth,
            guint     indent_spaces)
{
    switch (yaml_node_get_node_type(node))
    {
        case YAML_NODE_MAPPING:
            render_mapping(out, node, depth, indent_spaces);
            break;

        case YAML_NODE_SEQUENCE:
            render_sequence(out, node, depth, indent_spaces);
            break;

        case YAML_NODE_SCALAR:
        case YAML_NODE_NULL:
        default:
            append_indent(out, depth, indent_spaces);
            append_scalar(out, node, depth, indent_spaces);
            g_string_append_c(out, '\n');
            break;
    }
}

gchar *
yaml_writer_render(YamlNode *root, guint indent_spaces)
{
    GString *out;

    g_return_val_if_fail(root != NULL, NULL);

    if (indent_spaces == 0)
        indent_spaces = 2;

    out = g_string_new(NULL);

    /* The root's own comments are the file header. */
    append_leading_comments(out, root, 0, indent_spaces);

    switch (yaml_node_get_node_type(root))
    {
        case YAML_NODE_MAPPING:
            if (yaml_mapping_get_size(yaml_node_get_mapping(root)) == 0)
                g_string_append(out, "{}\n");
            else
                render_mapping(out, root, 0, indent_spaces);
            break;

        case YAML_NODE_SEQUENCE:
            if (yaml_sequence_get_length(yaml_node_get_sequence(root)) == 0)
                g_string_append(out, "[]\n");
            else
                render_sequence(out, root, 0, indent_spaces);
            break;

        default:
            append_scalar(out, root, 0, indent_spaces);
            g_string_append_c(out, '\n');
            break;
    }

    return g_string_free(out, FALSE);
}
