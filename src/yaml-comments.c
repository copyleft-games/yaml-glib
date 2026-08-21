/* yaml-comments.c
 *
 * Copyright 2026 Zach Podbielniak
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "yaml-comments.h"

#include <yaml.h>
#include <string.h>

typedef enum {
    LINE_CONTENT = 0,   /* has content, and possibly a trailing comment */
    LINE_COMMENT,       /* nothing but a comment */
    LINE_BLANK          /* nothing but whitespace */
} YamlLineKind;

typedef struct {
    YamlLineKind  kind;
    gchar        *comment;    /* owned; NULL when there is none */
    gint          last_token_end_col;  /* -1 when no token ends on this line */
    gboolean      inside_token;        /* a multi-line token covers this line */
} YamlLineInfo;

struct _YamlCommentIndex {
    GArray *lines;   /* YamlLineInfo, indexed by 0-based line number */

    /*
     * Lines already handed out as another node's leading comments.  Without
     * this, a comment block sitting above a nested mapping would be claimed
     * both by the mapping and by its first member, and be emitted twice on
     * the way back out.
     */
    GHashTable *consumed;
};

static void
line_info_clear(YamlLineInfo *info)
{
    g_clear_pointer(&info->comment, g_free);
}

/*
 * Strips the '#' and one following space, and trims trailing whitespace.
 * Keeping the text without its marker means the emitter owns how comments
 * are rendered, and re-emitting cannot accumulate '#' characters.
 */
static gchar *
extract_comment_text(const gchar *line, gsize hash_offset)
{
    const gchar *text = line + hash_offset + 1;
    gsize len;

    if (*text == ' ')
        text++;

    len = strlen(text);
    while (len > 0 && (text[len - 1] == '\r' ||
                       text[len - 1] == '\n' ||
                       text[len - 1] == '\t' ||
                       text[len - 1] == ' '))
    {
        len--;
    }

    return g_strndup(text, len);
}

/*
 * Runs libyaml's scanner over the source and records, per line, where the
 * last token ends and whether a multi-line token passes through.  This is
 * what makes the '#' search safe: a '#' at or after the last token's end
 * column on a content line is genuinely a comment, and a '#' on a line a
 * token passes through is genuinely not.
 */
static gboolean
scan_token_extents(const gchar *data, gsize length, GArray *lines)
{
    yaml_parser_t parser;
    gboolean ok = TRUE;

    if (!yaml_parser_initialize(&parser))
        return FALSE;

    yaml_parser_set_input_string(&parser,
                                 (const unsigned char *)data,
                                 length);

    while (TRUE)
    {
        yaml_token_t token;
        guint line;

        if (!yaml_parser_scan(&parser, &token))
        {
            ok = FALSE;
            break;
        }

        if (token.type == YAML_STREAM_END_TOKEN)
        {
            yaml_token_delete(&token);
            break;
        }

        /* Interior lines of a multi-line token are pure content. */
        for (line = (guint)token.start_mark.line + 1;
             line < (guint)token.end_mark.line && line < lines->len;
             line++)
        {
            g_array_index(lines, YamlLineInfo, line).inside_token = TRUE;
        }

        line = (guint)token.end_mark.line;
        if (line < lines->len)
        {
            YamlLineInfo *info = &g_array_index(lines, YamlLineInfo, line);
            gint end_col = (gint)token.end_mark.column;

            if (end_col > info->last_token_end_col)
                info->last_token_end_col = end_col;
        }

        yaml_token_delete(&token);
    }

    yaml_parser_delete(&parser);
    return ok;
}

YamlCommentIndex *
yaml_comment_index_new(const gchar *data, gsize length)
{
    YamlCommentIndex *index;
    g_auto(GStrv) split = NULL;
    g_autofree gchar *owned = NULL;
    guint line_index;
    guint n_lines;

    g_return_val_if_fail(data != NULL, NULL);

    owned = g_strndup(data, length);
    split = g_strsplit(owned, "\n", -1);
    n_lines = g_strv_length(split);

    index = g_new0(YamlCommentIndex, 1);
    index->lines = g_array_sized_new(FALSE, TRUE, sizeof(YamlLineInfo), n_lines);
    g_array_set_clear_func(index->lines, (GDestroyNotify)line_info_clear);
    g_array_set_size(index->lines, n_lines);
    index->consumed = g_hash_table_new(g_direct_hash, g_direct_equal);

    for (line_index = 0; line_index < n_lines; line_index++)
    {
        g_array_index(index->lines, YamlLineInfo, line_index).last_token_end_col = -1;
    }

    /*
     * A source we cannot scan gets an index with no comments rather than a
     * failure.  The document parse that follows will report the syntax
     * error, and it will do so with a line and column; a second, vaguer
     * complaint from here would only obscure it.
     */
    if (!scan_token_extents(owned, length, index->lines))
        return index;

    for (line_index = 0; line_index < n_lines; line_index++)
    {
        YamlLineInfo *info = &g_array_index(index->lines, YamlLineInfo, line_index);
        const gchar *line = split[line_index];
        const gchar *p;
        gsize offset;

        if (info->inside_token)
        {
            info->kind = LINE_CONTENT;
            continue;
        }

        p = line;
        while (*p == ' ' || *p == '\t')
            p++;

        if (*p == '\0' || *p == '\r')
        {
            info->kind = LINE_BLANK;
            continue;
        }

        if (*p == '#')
        {
            info->kind = LINE_COMMENT;
            info->comment = extract_comment_text(line, (gsize)(p - line));
            continue;
        }

        info->kind = LINE_CONTENT;

        /*
         * Look for a trailing comment, but only past where the last token
         * on this line ended.  Searching the whole line would find the '#'
         * in `password: "a#b"` and cheerfully truncate the value on the way
         * back out.
         */
        if (info->last_token_end_col < 0)
            continue;

        offset = (gsize)info->last_token_end_col;
        if (offset > strlen(line))
            continue;

        for (p = line + offset; *p != '\0'; p++)
        {
            if (*p == '#')
            {
                info->comment = extract_comment_text(line, (gsize)(p - line));
                break;
            }
        }
    }

    return index;
}

void
yaml_comment_index_free(YamlCommentIndex *index)
{
    if (index == NULL)
        return;

    g_array_unref(index->lines);
    g_hash_table_destroy(index->consumed);
    g_free(index);
}

GPtrArray *
yaml_comment_index_take_leading(YamlCommentIndex *index, guint line)
{
    GPtrArray *out;
    GPtrArray *reversed;
    gint cursor;
    guint i;
    gboolean saw_comment = FALSE;

    g_return_val_if_fail(index != NULL, NULL);

    if (line == 0 || line > index->lines->len)
        return NULL;

    reversed = g_ptr_array_new_with_free_func(g_free);

    /*
     * Walk upward while the lines are comments, stopping at content OR at a
     * blank line.
     *
     * The blank line is the interesting part.  It is what separates a file's
     * header block from the first key beneath it:
     *
     *     # clawtilla configuration      <- about the file
     *     # second header line
     *                                    <- this blank is the separator
     *     # About the daemon.            <- about `daemon`
     *     daemon:
     *
     * Without it, `daemon` swallows the whole thing and the file header
     * reappears indented under the first key on the way back out.
     */
    for (cursor = (gint)line - 1; cursor >= 0; cursor--)
    {
        YamlLineInfo *info = &g_array_index(index->lines, YamlLineInfo, cursor);

        if (info->kind != LINE_COMMENT)
            break;

        if (g_hash_table_contains(index->consumed, GINT_TO_POINTER(cursor)))
            break;

        g_ptr_array_add(reversed, g_strdup(info->comment ? info->comment : ""));
        saw_comment = TRUE;

        g_hash_table_add(index->consumed, GINT_TO_POINTER(cursor));
    }

    if (!saw_comment)
    {
        g_ptr_array_unref(reversed);
        return NULL;
    }

    out = g_ptr_array_new_with_free_func(g_free);
    for (i = reversed->len; i > 0; i--)
        g_ptr_array_add(out, g_strdup(g_ptr_array_index(reversed, i - 1)));

    g_ptr_array_unref(reversed);

    if (out->len == 0)
    {
        g_ptr_array_unref(out);
        return NULL;
    }

    return out;
}

GPtrArray *
yaml_comment_index_take_header(YamlCommentIndex *index)
{
    GPtrArray *out;
    guint cursor;
    guint i;

    g_return_val_if_fail(index != NULL, NULL);

    /*
     * The header runs from the top of the file down to the first line that
     * is not a comment.  Taken downward rather than upward because the root
     * node's start mark is the first key's line, which the first key has an
     * equal claim to -- anchoring the header there would make whichever of
     * the two ran first win.
     *
     * It is only a header if a blank line follows it.  A comment block sitting
     * directly on top of the first key is about that key:
     *
     *     # what this file is        # how chatty the log is
     *                                log_level: info
     *     log_level: info
     *      ^ header                   ^ not a header
     *
     * Same separator rule as everywhere else, so there is one thing to
     * learn rather than two.
     */
    for (cursor = 0; cursor < index->lines->len; cursor++)
    {
        YamlLineInfo *info = &g_array_index(index->lines, YamlLineInfo, cursor);

        if (info->kind != LINE_COMMENT)
            break;

        if (g_hash_table_contains(index->consumed, GINT_TO_POINTER((gint)cursor)))
            return NULL;
    }

    if (cursor == 0)
        return NULL;

    if (cursor >= index->lines->len)
        return NULL;

    if (g_array_index(index->lines, YamlLineInfo, cursor).kind != LINE_BLANK)
        return NULL;

    out = g_ptr_array_new_with_free_func(g_free);

    for (i = 0; i < cursor; i++)
    {
        YamlLineInfo *info = &g_array_index(index->lines, YamlLineInfo, i);

        g_ptr_array_add(out, g_strdup(info->comment ? info->comment : ""));
        g_hash_table_add(index->consumed, GINT_TO_POINTER((gint)i));
    }

    if (out->len == 0)
    {
        g_ptr_array_unref(out);
        return NULL;
    }

    return out;
}

gboolean
yaml_comment_index_has_blank_before(YamlCommentIndex *index, guint line)
{
    gint cursor;

    g_return_val_if_fail(index != NULL, FALSE);

    if (line == 0 || line > index->lines->len)
        return FALSE;

    /* Step over the comment run, then look at what is above it. */
    for (cursor = (gint)line - 1; cursor >= 0; cursor--)
    {
        YamlLineInfo *info = &g_array_index(index->lines, YamlLineInfo, cursor);

        if (info->kind == LINE_COMMENT)
            continue;

        return (info->kind == LINE_BLANK);
    }

    /*
     * Reaching the top of the file is not a blank line: a leading newline
     * would be added to the output on every write.
     */
    return FALSE;
}

const gchar *
yaml_comment_index_get_trailing(YamlCommentIndex *index, guint line)
{
    YamlLineInfo *info;

    g_return_val_if_fail(index != NULL, NULL);

    if (line >= index->lines->len)
        return NULL;

    info = &g_array_index(index->lines, YamlLineInfo, line);

    if (info->kind != LINE_CONTENT)
        return NULL;

    return info->comment;
}
