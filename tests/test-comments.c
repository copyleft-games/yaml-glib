/* test-comments.c
 *
 * Copyright 2026 Zach Podbielniak
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Comment capture and emission.
 *
 * libyaml discards comments, so before this existed a parse/emit round-trip
 * silently deleted every comment in the file.  These tests are mostly about
 * the cases where naive comment handling goes wrong: a '#' that is content
 * rather than a comment, a comment claimed by the wrong node, and scalars
 * quietly changing type on the way through.
 */

#include <yaml-glib.h>
#include <string.h>

static YamlNode *
parse_with_comments(YamlParser **out_parser, const gchar *src)
{
    YamlParser *parser = yaml_parser_new();
    GError *error = NULL;

    yaml_parser_set_capture_comments(parser, TRUE);
    g_assert_true(yaml_parser_load_from_data(parser, src, -1, &error));
    g_assert_no_error(error);

    *out_parser = parser;
    return yaml_parser_get_root(parser);
}

static gchar *
render_with_comments(YamlNode *root)
{
    g_autoptr(YamlGenerator) generator = yaml_generator_new();

    yaml_generator_set_emit_comments(generator, TRUE);
    yaml_generator_set_root(generator, root);

    return yaml_generator_to_data(generator, NULL, NULL);
}

static const gchar *
nth_comment(YamlNode *node, guint index)
{
    GPtrArray *comments = yaml_node_get_leading_comments(node);

    g_assert_nonnull(comments);
    g_assert_cmpuint(comments->len, >, index);

    return g_ptr_array_index(comments, index);
}

/* Capture is opt-in, so an ordinary parse must behave exactly as before. */
static void
test_capture_is_opt_in(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    YamlNode *root;
    YamlNode *key;

    g_assert_false(yaml_parser_get_capture_comments(parser));
    g_assert_true(yaml_parser_load_from_data(parser,
                                             "# a comment\nkey: value\n",
                                             -1, NULL));

    root = yaml_parser_get_root(parser);
    key = yaml_mapping_get_member(yaml_node_get_mapping(root), "key");

    g_assert_false(yaml_node_has_comments(key));
    g_assert_null(yaml_node_get_leading_comments(key));
}

static void
test_leading_and_trailing(void)
{
    g_autoptr(YamlParser) parser = NULL;
    YamlNode *root;
    YamlNode *key;

    root = parse_with_comments(&parser,
                               "# first line\n"
                               "# second line\n"
                               "key: value   # on the same line\n");

    key = yaml_mapping_get_member(yaml_node_get_mapping(root), "key");

    g_assert_cmpstr(nth_comment(key, 0), ==, "first line");
    g_assert_cmpstr(nth_comment(key, 1), ==, "second line");
    g_assert_cmpstr(yaml_node_get_trailing_comment(key), ==, "on the same line");
}

/*
 * A blank line separates a file's header from the first key.  Without that
 * rule the first key swallows the header, and it reappears indented under
 * that key the first time the file is written back.
 */
static void
test_blank_line_separates_header_from_first_key(void)
{
    g_autoptr(YamlParser) parser = NULL;
    YamlNode *root;
    YamlNode *key;

    root = parse_with_comments(&parser,
                               "# about the file\n"
                               "\n"
                               "# about the key\n"
                               "key: value\n");

    key = yaml_mapping_get_member(yaml_node_get_mapping(root), "key");

    g_assert_cmpstr(nth_comment(root, 0), ==, "about the file");
    g_assert_cmpuint(yaml_node_get_leading_comments(root)->len, ==, 1);

    g_assert_cmpstr(nth_comment(key, 0), ==, "about the key");
    g_assert_cmpuint(yaml_node_get_leading_comments(key)->len, ==, 1);
}

/*
 * Conversion is depth-first, so without care the innermost node reaches a
 * comment first: here the scalar `chief` would claim it rather than the
 * sequence entry the comment is plainly about.
 */
static void
test_sequence_entry_claims_its_comment(void)
{
    g_autoptr(YamlParser) parser = NULL;
    YamlNode *root;
    YamlNode *agents;
    YamlNode *first;
    YamlNode *id;

    root = parse_with_comments(&parser,
                               "agents:\n"
                               "  # the first agent\n"
                               "  - id: chief\n");

    agents = yaml_mapping_get_member(yaml_node_get_mapping(root), "agents");
    first = yaml_sequence_get_element(yaml_node_get_sequence(agents), 0);
    id = yaml_mapping_get_member(yaml_node_get_mapping(first), "id");

    g_assert_cmpstr(nth_comment(first, 0), ==, "the first agent");
    g_assert_false(yaml_node_has_comments(id));
}

/* A '#' inside a quoted scalar is content, not a comment. */
static void
test_hash_inside_quoted_scalar_is_not_a_comment(void)
{
    g_autoptr(YamlParser) parser = NULL;
    YamlNode *root;
    YamlNode *motd;

    root = parse_with_comments(&parser, "motd: \"has a # inside\"\n");
    motd = yaml_mapping_get_member(yaml_node_get_mapping(root), "motd");

    g_assert_cmpstr(yaml_node_get_string(motd), ==, "has a # inside");
    g_assert_null(yaml_node_get_trailing_comment(motd));
}

/* Lines inside a block scalar are content, however much they look like
 * comments -- a shell script's shebang is the obvious case. */
static void
test_block_scalar_content_is_not_comments(void)
{
    g_autoptr(YamlParser) parser = NULL;
    YamlNode *root;
    YamlNode *script;

    root = parse_with_comments(&parser,
                               "script: |\n"
                               "  #!/bin/sh\n"
                               "  # a shell comment\n"
                               "  echo hi\n"
                               "after: 1\n");

    script = yaml_mapping_get_member(yaml_node_get_mapping(root), "script");

    g_assert_cmpstr(yaml_node_get_string(script), ==,
                    "#!/bin/sh\n# a shell comment\necho hi\n");
    g_assert_false(yaml_node_has_comments(
        yaml_mapping_get_member(yaml_node_get_mapping(root), "after")));
}

/*
 * The bug this guards: yaml-glib models every scalar as a string, so a
 * writer that guesses will quote anything that could be read back as
 * another type -- turning `enabled: true` into `enabled: 'true'` and
 * changing a boolean into a string.
 */
static void
test_round_trip_preserves_scalar_types(void)
{
    g_autoptr(YamlParser) parser = NULL;
    g_autofree gchar *out = NULL;
    YamlNode *root;

    root = parse_with_comments(&parser,
                               "enabled: true\n"
                               "retries: 3\n"
                               "version: '1.0'\n"
                               "name: plain\n");

    out = render_with_comments(root);

    g_assert_nonnull(strstr(out, "enabled: true"));
    g_assert_nonnull(strstr(out, "retries: 3"));
    g_assert_null(strstr(out, "'true'"));
    g_assert_null(strstr(out, "'3'"));

    /* And a quoted string stays quoted, or it comes back as a number. */
    g_assert_nonnull(strstr(out, "version: '1.0'"));
}

/* Backslashes must survive; single-quoted style is what makes that free. */
static void
test_round_trip_preserves_backslashes(void)
{
    g_autoptr(YamlParser) parser = NULL;
    g_autoptr(YamlParser) reparsed = NULL;
    g_autofree gchar *out = NULL;
    YamlNode *root;
    YamlNode *again;

    root = parse_with_comments(&parser, "path: 'C:\\dir\\file'\n");
    out = render_with_comments(root);

    again = parse_with_comments(&reparsed, out);
    g_assert_cmpstr(
        yaml_node_get_string(
            yaml_mapping_get_member(yaml_node_get_mapping(again), "path")),
        ==, "C:\\dir\\file");
}

/*
 * Writing the file twice must produce the same bytes.  A round-trip that
 * drifts -- a blank line gained, a comment marker doubled -- turns every
 * save into a spurious diff.
 */
static void
test_round_trip_is_idempotent(void)
{
    static const gchar *src =
        "# header\n"
        "\n"
        "# about daemon\n"
        "daemon:\n"
        "  socket: /run/x.sock  # where clients dial in\n"
        "  #\n"
        "  # a block with an empty line in it\n"
        "  level: info\n"
        "\n"
        "agents:\n"
        "  # first\n"
        "  - id: chief\n"
        "    tools: []\n"
        "  - id: other\n"
        "    env: {}\n";
    g_autoptr(YamlParser) first = NULL;
    g_autoptr(YamlParser) second = NULL;
    g_autofree gchar *out1 = NULL;
    g_autofree gchar *out2 = NULL;

    out1 = render_with_comments(parse_with_comments(&first, src));
    out2 = render_with_comments(parse_with_comments(&second, out1));

    g_assert_cmpstr(out1, ==, out2);

    /* And the things we care about are actually still in there. */
    g_assert_nonnull(strstr(out1, "# header"));
    g_assert_nonnull(strstr(out1, "# about daemon"));
    g_assert_nonnull(strstr(out1, "# where clients dial in"));
    g_assert_nonnull(strstr(out1, "# first"));
    g_assert_nonnull(strstr(out1, "tools: []"));
    g_assert_nonnull(strstr(out1, "env: {}"));
}

/* Blank lines between sections are structure, not noise. */
static void
test_blank_lines_between_sections_survive(void)
{
    g_autoptr(YamlParser) parser = NULL;
    g_autofree gchar *out = NULL;

    out = render_with_comments(parse_with_comments(&parser,
                                                   "a: 1\n"
                                                   "\n"
                                                   "b: 2\n"));

    g_assert_nonnull(strstr(out, "a: 1\n\nb: 2"));

    /* But never a leading blank line. */
    g_assert_cmpint(out[0], !=, '\n');
}

/* An explicitly empty collection must not come back as null. */
static void
test_empty_collections_round_trip(void)
{
    g_autoptr(YamlParser) parser = NULL;
    g_autoptr(YamlParser) reparsed = NULL;
    g_autofree gchar *out = NULL;
    YamlNode *root;

    out = render_with_comments(parse_with_comments(&parser,
                                                   "tools: []\n"
                                                   "env: {}\n"));

    root = parse_with_comments(&reparsed, out);

    g_assert_cmpint(yaml_node_get_node_type(
        yaml_mapping_get_member(yaml_node_get_mapping(root), "tools")),
        ==, YAML_NODE_SEQUENCE);
    g_assert_cmpint(yaml_node_get_node_type(
        yaml_mapping_get_member(yaml_node_get_mapping(root), "env")),
        ==, YAML_NODE_MAPPING);
}

/* Comments set programmatically are emitted too. */
static void
test_comments_can_be_set_by_hand(void)
{
    g_autoptr(YamlNode) root = yaml_node_new_mapping(yaml_mapping_new());
    g_autoptr(YamlNode) value = yaml_node_new_string("info");
    g_autofree gchar *out = NULL;

    yaml_node_add_leading_comment(value, "How chatty the log is.");
    yaml_node_set_trailing_comment(value, "default: info");
    yaml_mapping_set_member(yaml_node_get_mapping(root), "log_level", value);

    out = render_with_comments(root);

    g_assert_nonnull(strstr(out, "# How chatty the log is."));
    g_assert_nonnull(strstr(out, "# default: info"));
}

/* An empty trailing comment is no comment, or a bare '#' accumulates. */
static void
test_empty_trailing_comment_is_cleared(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("x");

    yaml_node_set_trailing_comment(node, "");
    g_assert_null(yaml_node_get_trailing_comment(node));

    yaml_node_set_trailing_comment(node, "real");
    g_assert_cmpstr(yaml_node_get_trailing_comment(node), ==, "real");

    yaml_node_set_trailing_comment(node, NULL);
    g_assert_null(yaml_node_get_trailing_comment(node));
}

/* Copies carry comments, or a round-trip through yaml_node_copy() strips
 * them without anybody asking. */
static void
test_copy_carries_comments(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("x");
    g_autoptr(YamlNode) copy = NULL;

    yaml_node_add_leading_comment(node, "kept");
    yaml_node_set_trailing_comment(node, "also kept");
    yaml_node_set_blank_before(node, TRUE);

    copy = yaml_node_copy(node);

    g_assert_cmpstr(nth_comment(copy, 0), ==, "kept");
    g_assert_cmpstr(yaml_node_get_trailing_comment(copy), ==, "also kept");
    g_assert_true(yaml_node_get_blank_before(copy));
}

/* A file that is nothing but comments has no node to hang them on; it must
 * not crash, which is the only promise worth making here. */
static void
test_comment_only_document(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();

    yaml_parser_set_capture_comments(parser, TRUE);
    yaml_parser_load_from_data(parser, "# just a comment\n", -1, NULL);
    g_assert_null(yaml_parser_get_root(parser));
}

/* An empty document, and a document that is a bare scalar. */
static void
test_degenerate_documents(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autofree gchar *out = NULL;
    YamlNode *root;

    yaml_parser_set_capture_comments(parser, TRUE);
    g_assert_true(yaml_parser_load_from_data(parser, "# lead\njust-a-scalar\n",
                                             -1, NULL));
    root = yaml_parser_get_root(parser);
    g_assert_nonnull(root);

    out = render_with_comments(root);
    g_assert_nonnull(strstr(out, "just-a-scalar"));
}

int
main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/comments/capture-is-opt-in", test_capture_is_opt_in);
    g_test_add_func("/comments/leading-and-trailing", test_leading_and_trailing);
    g_test_add_func("/comments/header-separated-by-blank",
                    test_blank_line_separates_header_from_first_key);
    g_test_add_func("/comments/sequence-entry-claims",
                    test_sequence_entry_claims_its_comment);
    g_test_add_func("/comments/hash-in-quoted-scalar",
                    test_hash_inside_quoted_scalar_is_not_a_comment);
    g_test_add_func("/comments/block-scalar-content",
                    test_block_scalar_content_is_not_comments);
    g_test_add_func("/comments/preserves-scalar-types",
                    test_round_trip_preserves_scalar_types);
    g_test_add_func("/comments/preserves-backslashes",
                    test_round_trip_preserves_backslashes);
    g_test_add_func("/comments/idempotent", test_round_trip_is_idempotent);
    g_test_add_func("/comments/blank-lines-survive",
                    test_blank_lines_between_sections_survive);
    g_test_add_func("/comments/empty-collections",
                    test_empty_collections_round_trip);
    g_test_add_func("/comments/set-by-hand", test_comments_can_be_set_by_hand);
    g_test_add_func("/comments/empty-trailing-cleared",
                    test_empty_trailing_comment_is_cleared);
    g_test_add_func("/comments/copy-carries", test_copy_carries_comments);
    g_test_add_func("/comments/comment-only-document",
                    test_comment_only_document);
    g_test_add_func("/comments/degenerate-documents",
                    test_degenerate_documents);

    return g_test_run();
}
