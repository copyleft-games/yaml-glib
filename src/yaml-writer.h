/* yaml-writer.h
 *
 * Copyright 2026 Zach Podbielniak
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Private: a block-style YAML writer that can emit comments.
 *
 * libyaml's emitter cannot.  Comments are not part of its event model, and
 * there is no way to interleave raw text into its output without wrecking
 * the column tracking it uses for indentation and line wrapping.  So when
 * a caller asks for comments, generation goes through this writer instead.
 *
 * It emits block style only, which is the point: comments and flow style
 * do not mix, and a configuration file people edit wants block style
 * anyway.  Callers who do not need comments keep the libyaml path, which
 * handles flow style, canonical output and anchors.
 */

#ifndef __YAML_WRITER_H__
#define __YAML_WRITER_H__

#include <glib.h>
#include "yaml-node.h"

G_BEGIN_DECLS

/*
 * Renders @root as block-style YAML, including any comments attached to
 * its nodes.
 *
 * Returns: (transfer full): the document text, always newline-terminated
 */
gchar *
yaml_writer_render(YamlNode *root,
                   guint     indent_spaces);

G_END_DECLS

#endif /* __YAML_WRITER_H__ */
