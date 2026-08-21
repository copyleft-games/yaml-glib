/* yaml-comments.h
 *
 * Copyright 2026 Zach Podbielniak
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Private: recovery of comments from YAML source text.
 *
 * libyaml throws comments away.  They are not tokens, they never reach the
 * document API, and there is no option to keep them -- so a parse/emit
 * round-trip through this library silently deleted every comment in the
 * file.  Tolerable for machine data, not for a config file a person edits.
 *
 * This recovers them from the source text and indexes them by line, so the
 * parser can attach each one to whatever node begins on the next content
 * line.
 *
 * The one thing that makes this correct rather than a regex guess: a '#'
 * is only a comment when it is not inside a scalar.  `password: "a#b"` and
 * a block scalar containing a shell script both contain '#' characters
 * that are content.  So before looking at any text, we run libyaml's own
 * scanner over the source and record exactly which columns of which lines
 * are covered by tokens.  Anything a token covers is content, by
 * definition, and is never considered.
 */

#ifndef __YAML_COMMENTS_H__
#define __YAML_COMMENTS_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _YamlCommentIndex YamlCommentIndex;

/*
 * Builds a line-indexed view of the comments in @data.
 *
 * Returns NULL when the source cannot be scanned (malformed YAML); the
 * caller carries on without comments rather than failing the parse, since
 * the real parser will report the syntax error with a better message.
 */
YamlCommentIndex *
yaml_comment_index_new(const gchar *data,
                       gsize        length);

void
yaml_comment_index_free(YamlCommentIndex *index);

/*
 * Collects the run of whole-line comments immediately above @line
 * (0-based), in source order, with '#' and one following space removed.
 * Blank lines inside the run are kept as empty strings; blank lines at the
 * start of the result are trimmed.
 *
 * Returns: (transfer full) (nullable): a GPtrArray of gchar*, or NULL when
 *   there is no comment above @line.
 */
GPtrArray *
yaml_comment_index_take_leading(YamlCommentIndex *index,
                                guint             line);

/*
 * Whether a blank line sits above @line, or above the comment run that
 * ends at @line - 1.  Used to keep the spacing between sections.
 */
gboolean
yaml_comment_index_has_blank_before(YamlCommentIndex *index,
                                    guint             line);

/*
 * Takes the comment block at the very top of the file -- the run of
 * comment lines from line 0 down to the first blank or content line.
 *
 * Separate from take_leading() because the root node's start mark is the
 * first key's line, so both have an equal claim to anything above it and
 * whichever ran first would win.  Taken downward from the top instead.
 *
 * Returns: (transfer full) (nullable): a GPtrArray of gchar*, or NULL
 */
GPtrArray *
yaml_comment_index_take_header(YamlCommentIndex *index);

/*
 * The comment sharing @line with content, if any.
 *
 * Returns: (transfer none) (nullable): the comment text, or NULL
 */
const gchar *
yaml_comment_index_get_trailing(YamlCommentIndex *index,
                                guint             line);

G_END_DECLS

#endif /* __YAML_COMMENTS_H__ */
