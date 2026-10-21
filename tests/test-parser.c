/* test_parser.c
 *
 * Copyright 2025 Zach Podbielniak
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Unit tests for YamlParser.
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include "yaml-glib.h"

/* Test parsing a simple mapping */
static void
test_parser_simple_mapping(void)
{
    YamlParser *parser;
    YamlNode *root;
    YamlMapping *mapping;
    GError *error = NULL;
    const gchar *yaml = "name: John\nage: 30\nactive: true\n";

    parser = yaml_parser_new();
    g_assert_nonnull(parser);

    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));
    g_assert_no_error(error);

    g_assert_cmpuint(yaml_parser_get_n_documents(parser), ==, 1);

    root = yaml_parser_get_root(parser);
    g_assert_nonnull(root);
    g_assert_cmpint(yaml_node_get_node_type(root), ==, YAML_NODE_MAPPING);

    mapping = yaml_node_get_mapping(root);
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "name"), ==, "John");
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "age"), ==, 30);
    g_assert_true(yaml_mapping_get_boolean_member(mapping, "active"));

    g_object_unref(parser);
}

/* Test parsing a sequence */
static void
test_parser_sequence(void)
{
    YamlParser *parser;
    YamlNode *root;
    YamlSequence *sequence;
    YamlNode *element;
    GError *error = NULL;
    const gchar *yaml = "- one\n- two\n- three\n";

    parser = yaml_parser_new();
    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));
    g_assert_no_error(error);

    root = yaml_parser_get_root(parser);
    g_assert_cmpint(yaml_node_get_node_type(root), ==, YAML_NODE_SEQUENCE);

    sequence = yaml_node_get_sequence(root);
    g_assert_cmpuint(yaml_sequence_get_length(sequence), ==, 3);

    element = yaml_sequence_get_element(sequence, 0);
    g_assert_cmpstr(yaml_node_get_scalar(element), ==, "one");

    element = yaml_sequence_get_element(sequence, 1);
    g_assert_cmpstr(yaml_node_get_scalar(element), ==, "two");

    element = yaml_sequence_get_element(sequence, 2);
    g_assert_cmpstr(yaml_node_get_scalar(element), ==, "three");

    g_object_unref(parser);
}

/* Test parsing nested structures */
static void
test_parser_nested(void)
{
    YamlParser *parser;
    YamlNode *root;
    YamlMapping *mapping;
    YamlNode *nested_node;
    YamlMapping *nested_mapping;
    GError *error = NULL;
    const gchar *yaml =
        "person:\n"
        "  name: Alice\n"
        "  address:\n"
        "    city: Wonderland\n"
        "    zip: 12345\n";

    parser = yaml_parser_new();
    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));
    g_assert_no_error(error);

    root = yaml_parser_get_root(parser);
    mapping = yaml_node_get_mapping(root);

    nested_node = yaml_mapping_get_member(mapping, "person");
    g_assert_nonnull(nested_node);
    g_assert_cmpint(yaml_node_get_node_type(nested_node), ==, YAML_NODE_MAPPING);

    nested_mapping = yaml_node_get_mapping(nested_node);
    g_assert_cmpstr(yaml_mapping_get_string_member(nested_mapping, "name"), ==, "Alice");

    nested_node = yaml_mapping_get_member(nested_mapping, "address");
    nested_mapping = yaml_node_get_mapping(nested_node);
    g_assert_cmpstr(yaml_mapping_get_string_member(nested_mapping, "city"), ==, "Wonderland");
    g_assert_cmpint(yaml_mapping_get_int_member(nested_mapping, "zip"), ==, 12345);

    g_object_unref(parser);
}

/* Test parsing multiple documents */
static void
test_parser_multi_document(void)
{
    YamlParser *parser;
    YamlDocument *doc;
    YamlNode *root;
    GError *error = NULL;
    const gchar *yaml =
        "---\n"
        "first: document\n"
        "---\n"
        "second: document\n"
        "...\n";

    parser = yaml_parser_new();
    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));
    g_assert_no_error(error);

    g_assert_cmpuint(yaml_parser_get_n_documents(parser), ==, 2);

    doc = yaml_parser_get_document(parser, 0);
    g_assert_nonnull(doc);
    root = yaml_document_get_root(doc);
    g_assert_cmpstr(
        yaml_mapping_get_string_member(yaml_node_get_mapping(root), "first"),
        ==, "document"
    );

    doc = yaml_parser_get_document(parser, 1);
    g_assert_nonnull(doc);
    root = yaml_document_get_root(doc);
    g_assert_cmpstr(
        yaml_mapping_get_string_member(yaml_node_get_mapping(root), "second"),
        ==, "document"
    );

    g_object_unref(parser);
}

/* Test immutable parser mode */
static void
test_parser_immutable(void)
{
    YamlParser *parser;
    YamlDocument *doc;
    GError *error = NULL;
    const gchar *yaml = "key: value\n";

    parser = yaml_parser_new_immutable();
    g_assert_true(yaml_parser_get_immutable(parser));

    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));
    g_assert_no_error(error);

    doc = yaml_parser_get_document(parser, 0);
    g_assert_true(yaml_document_is_immutable(doc));

    g_object_unref(parser);
}

/* Test parser reset */
static void
test_parser_reset(void)
{
    YamlParser *parser;
    GError *error = NULL;
    const gchar *yaml1 = "first: true\n";
    const gchar *yaml2 = "second: true\n";

    parser = yaml_parser_new();

    g_assert_true(yaml_parser_load_from_data(parser, yaml1, -1, &error));
    g_assert_cmpuint(yaml_parser_get_n_documents(parser), ==, 1);

    yaml_parser_reset(parser);
    g_assert_cmpuint(yaml_parser_get_n_documents(parser), ==, 0);

    g_assert_true(yaml_parser_load_from_data(parser, yaml2, -1, &error));
    g_assert_cmpuint(yaml_parser_get_n_documents(parser), ==, 1);

    g_object_unref(parser);
}

/* Test parsing scalar values */
static void
test_parser_scalar_types(void)
{
    YamlParser *parser;
    YamlNode *root;
    YamlMapping *mapping;
    GError *error = NULL;
    const gchar *yaml =
        "string: hello\n"
        "integer: 42\n"
        "float: 3.14\n"
        "bool_true: true\n"
        "bool_false: false\n"
        "null_value: null\n"
        "empty: ~\n";

    parser = yaml_parser_new();
    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));
    g_assert_no_error(error);

    root = yaml_parser_get_root(parser);
    mapping = yaml_node_get_mapping(root);

    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "string"), ==, "hello");
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "integer"), ==, 42);
    g_assert_cmpfloat_with_epsilon(
        yaml_mapping_get_double_member(mapping, "float"), 3.14, 0.01
    );
    g_assert_true(yaml_mapping_get_boolean_member(mapping, "bool_true"));
    g_assert_false(yaml_mapping_get_boolean_member(mapping, "bool_false"));

    g_object_unref(parser);
}

/* Test YAML 1.2 core-schema integer forms */
static void
test_parser_int_bases(void)
{
    YamlParser *parser;
    YamlMapping *mapping;
    YamlNode *node;
    GError *error = NULL;
    const gchar *yaml =
        "hex: 0x1ED\n"
        "hex_upper: 0xFFFF\n"
        "octal: 0o17\n"
        "padded: 0x001\n"
        "zero: 0x0\n"
        "negative: -12\n"
        "plus: +7\n"
        "leading_zero: 010\n"
        "not_hex: 0xZZ\n"
        "bare_prefix: 0x\n"
        "signed_hex: 0x-1\n"
        "too_big: 0x8000000000000000\n";

    parser = yaml_parser_new();
    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));
    g_assert_no_error(error);
    mapping = yaml_node_get_mapping(yaml_parser_get_root(parser));

    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "hex"), ==, 0x1ED);
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "hex_upper"), ==, 0xFFFF);
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "octal"), ==, 15);
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "padded"), ==, 1);
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "zero"), ==, 0);
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "negative"), ==, -12);
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "plus"), ==, 7);
    /* YAML 1.2 has no leading-zero octal */
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "leading_zero"), ==, 10);

    /* malformed forms stay strings */
    node = yaml_mapping_get_member(mapping, "not_hex");
    g_assert_cmpstr(yaml_node_get_string(node), ==, "0xZZ");
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "not_hex"), ==, 0);
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "bare_prefix"), ==, 0);
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "signed_hex"), ==, 0);
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "too_big"), ==, 0);

    g_object_unref(parser);
}

/* Test error handling */
static void
test_parser_error(void)
{
    YamlParser *parser;
    GError *error = NULL;
    const gchar *invalid_yaml = "key: [unclosed bracket";

    parser = yaml_parser_new();
    g_assert_false(yaml_parser_load_from_data(parser, invalid_yaml, -1, &error));
    g_assert_error(error, YAML_GLIB_PARSER_ERROR, YAML_GLIB_PARSER_ERROR_PARSE);
    g_error_free(error);

    g_object_unref(parser);
}

/* Test dup_root returns a new reference */
static void
test_parser_dup_root(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GError) error = NULL;
    const gchar *yaml = "key: value\n";

    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));

    /* dup_root gives us an owned reference */
    YamlNode *duped = yaml_parser_dup_root(parser);
    g_assert_nonnull(duped);
    g_assert_cmpint(yaml_node_get_node_type(duped), ==, YAML_NODE_MAPPING);
    yaml_node_unref(duped);
}

/* Test steal_root transfers ownership */
static void
test_parser_steal_root(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GError) error = NULL;
    const gchar *yaml = "key: value\n";

    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));

    YamlNode *stolen = yaml_parser_steal_root(parser);
    g_assert_nonnull(stolen);

    /* After steal, get_root should return NULL */
    g_assert_null(yaml_parser_get_root(parser));

    yaml_node_unref(stolen);
}

/* Test dup_document returns an owned reference */
static void
test_parser_dup_document(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GError) error = NULL;
    const gchar *yaml = "hello: world\n";

    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));
    g_assert_cmpuint(yaml_parser_get_n_documents(parser), ==, 1);

    /* dup_document gives owned ref */
    YamlDocument *duped = yaml_parser_dup_document(parser, 0);
    g_assert_nonnull(duped);

    YamlNode *root = yaml_document_get_root(duped);
    g_assert_nonnull(root);
    g_assert_cmpint(yaml_node_get_node_type(root), ==, YAML_NODE_MAPPING);

    g_object_unref(duped);
}

/* Test set_immutable on an existing parser */
static void
test_parser_set_immutable(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GError) error = NULL;
    const gchar *yaml = "key: value\n";

    g_assert_false(yaml_parser_get_immutable(parser));
    yaml_parser_set_immutable(parser, TRUE);
    g_assert_true(yaml_parser_get_immutable(parser));

    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));

    /* Document should be sealed */
    YamlDocument *doc = yaml_parser_get_document(parser, 0);
    g_assert_true(yaml_document_is_immutable(doc));
}

/* Test parsing empty input */
static void
test_parser_empty_input(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GError) error = NULL;

    /* Empty string should succeed but produce no documents */
    g_assert_true(yaml_parser_load_from_data(parser, "", 0, &error));
    g_assert_no_error(error);
    g_assert_cmpuint(yaml_parser_get_n_documents(parser), ==, 0);
    g_assert_null(yaml_parser_get_root(parser));
}

/* Test parsing with explicit length (not -1) */
static void
test_parser_explicit_length(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GError) error = NULL;
    /* String is longer, but we only pass 10 bytes: "key: value" */
    const gchar *yaml = "key: value\nextra: data\n";

    g_assert_true(yaml_parser_load_from_data(parser, yaml, 10, &error));
    g_assert_no_error(error);

    YamlNode *root = yaml_parser_get_root(parser);
    g_assert_nonnull(root);
}

/* Test file I/O parsing */
static void
test_parser_load_from_file(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GError) error = NULL;
    g_autofree gchar *tmpfile = NULL;
    gint fd;
    const gchar *yaml = "name: test\nvalue: 42\n";

    /* Write YAML to a temp file */
    fd = g_file_open_tmp("yaml-test-XXXXXX", &tmpfile, &error);
    g_assert_no_error(error);
    g_assert_cmpint(write(fd, yaml, strlen(yaml)), ==, (gssize)strlen(yaml));
    close(fd);

    g_assert_true(yaml_parser_load_from_file(parser, tmpfile, &error));
    g_assert_no_error(error);

    YamlNode *root = yaml_parser_get_root(parser);
    g_assert_nonnull(root);
    g_assert_cmpint(yaml_node_get_node_type(root), ==, YAML_NODE_MAPPING);

    YamlMapping *mapping = yaml_node_get_mapping(root);
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "name"), ==, "test");
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "value"), ==, 42);

    g_unlink(tmpfile);
}

/* Test file I/O with nonexistent file */
static void
test_parser_load_from_file_not_found(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    GError *error = NULL;

    g_assert_false(yaml_parser_load_from_file(parser,
                   "/nonexistent/path/file.yaml", &error));
    g_assert_nonnull(error);
    g_error_free(error);
}

/* Test various error conditions */
static void
test_parser_error_tab_indentation(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    GError *error = NULL;
    /* Tabs in indentation are invalid YAML */
    const gchar *yaml = "key:\n\t- invalid\n";

    gboolean result = yaml_parser_load_from_data(parser, yaml, -1, &error);
    /* libyaml may or may not reject tabs; just verify it doesn't crash */
    if (!result)
    {
        g_assert_nonnull(error);
        g_error_free(error);
    }
}

/* Test parsing null/tilde values */
static void
test_parser_null_values(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GError) error = NULL;
    const gchar *yaml =
        "null1: null\n"
        "null2: ~\n"
        "null3:\n";

    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));
    YamlNode *root = yaml_parser_get_root(parser);
    YamlMapping *mapping = yaml_node_get_mapping(root);

    /* All three should be null nodes */
    YamlNode *n1 = yaml_mapping_get_member(mapping, "null1");
    YamlNode *n2 = yaml_mapping_get_member(mapping, "null2");
    YamlNode *n3 = yaml_mapping_get_member(mapping, "null3");

    g_assert_nonnull(n1);
    g_assert_nonnull(n2);
    g_assert_nonnull(n3);
    g_assert_true(yaml_node_is_null(n1));
    g_assert_true(yaml_node_is_null(n2));
    g_assert_true(yaml_node_is_null(n3));
}

/* A quoted "null", "~" or "" is a string, never null (YAML 1.2) */
static void
test_parser_quoted_null_is_string(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GError) error = NULL;
    const gchar *yaml =
        "double: \"null\"\n"
        "single: 'NULL'\n"
        "tilde: \"~\"\n"
        "empty: \"\"\n"
        "plain: Null\n";
    YamlMapping *mapping;

    g_assert_true(yaml_parser_load_from_data(parser, yaml, -1, &error));
    g_assert_no_error(error);
    mapping = yaml_node_get_mapping(yaml_parser_get_root(parser));

    g_assert_false(yaml_node_is_null(yaml_mapping_get_member(mapping, "double")));
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "double"), ==, "null");
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "single"), ==, "NULL");
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "tilde"), ==, "~");
    g_assert_false(yaml_node_is_null(yaml_mapping_get_member(mapping, "empty")));
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "empty"), ==, "");
    g_assert_true(yaml_node_is_null(yaml_mapping_get_member(mapping, "plain")));
}

/*
 * A document that references itself is not a stack overflow.
 *
 * libyaml registers an anchor before it loads the anchored node's
 * children, so `a: &x { self: *x }` really is a cyclic yaml_document_t.
 * convert_yaml_node() recursed into it until the stack ran out --
 * twenty-seven bytes to kill any process that reads a YAML file
 * somebody else can write.  The @anchors table that exists to stop this
 * was created, threaded through every recursive call and destroyed, and
 * never once inserted into or looked up.
 */
static void
test_parser_self_referential_alias(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GError) error = NULL;
    gboolean ok;

    ok = yaml_parser_load_from_data(parser,
                                    "a: &anchor\n"
                                    "  self: *anchor\n",
                                    -1, &error);

    /* Either answer is fine.  Dying is not. */
    if (!ok)
        g_assert_nonnull(error);
    else
        g_assert_nonnull(yaml_parser_get_document(parser, 0));
}

/*
 * And an anchor referenced many times is converted once.
 *
 * An alias and its anchor are the same yaml_node_t, so without the memo
 * each reference converted the whole subtree again: nine levels of nine
 * references is 9^9 nodes.  A 493-byte file exhausted four gigabytes in
 * six seconds -- a billion laughs, from a parser used on config files.
 *
 * Asserted by completing at all, since the exponential version does not.
 */
static void
test_parser_repeated_alias_is_converted_once(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GError) error = NULL;
    g_autoptr(GString) yaml = g_string_new("l0: &a0 x\n");
    guint level;

    for (level = 1; level < 9; level++)
    {
        guint ref;

        g_string_append_printf(yaml, "l%u: &a%u [", level, level);

        for (ref = 0; ref < 9; ref++)
            g_string_append_printf(yaml, "%s*a%u", ref > 0 ? ", " : "",
                                   level - 1);

        g_string_append(yaml, "]\n");
    }

    g_assert_true(yaml_parser_load_from_data(parser, yaml->str, -1,
                                             &error));
    g_assert_no_error(error);
    g_assert_nonnull(yaml_parser_get_document(parser, 0));
}

/*
 * Malformed input with a NULL GError ** is a FALSE return, not a crash.
 *
 * GLib's contract is that every GError ** may be NULL: g_set_error()
 * honours it, and callers that only care about success rely on it.  The
 * parse-error path then handed `*error` to the ::error signal and so
 * dereferenced the NULL the caller passed -- an unterminated flow
 * sequence segfaulted any process that ignored the error.  Each load
 * entry point is exercised, since they all funnel into the same path.
 */
#define MALFORMED_YAML "matches: ["

/* ::error handler: records that it ran and what it was given */
static void
on_parser_error(
    YamlParser *parser,
    GError     *error,
    gpointer    user_data
)
{
    gboolean *seen = (gboolean *)user_data;

    (void)parser;
    g_assert_nonnull(error);
    g_assert_error(error, YAML_GLIB_PARSER_ERROR,
                   YAML_GLIB_PARSER_ERROR_PARSE);
    *seen = TRUE;
}

/* Writes MALFORMED_YAML to a fresh temp file; caller unlinks + frees */
static gchar *
write_malformed_tmpfile(void)
{
    gchar *path = NULL;
    gint fd;

    fd = g_file_open_tmp("yaml-glib-null-error-XXXXXX.yaml", &path, NULL);
    g_assert_cmpint(fd, >=, 0);
    g_assert_true(g_file_set_contents(path, MALFORMED_YAML, -1, NULL));
    close(fd);

    return path;
}

static void
test_parser_null_error_load_from_data(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    gboolean seen = FALSE;

    g_signal_connect(parser, "error", G_CALLBACK(on_parser_error), &seen);

    g_assert_false(yaml_parser_load_from_data(parser, MALFORMED_YAML, -1,
                                              NULL));
    /* the signal still carries the error, even though the caller didn't */
    g_assert_true(seen);
}

static void
test_parser_null_error_load_from_file(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autofree gchar *path = write_malformed_tmpfile();

    g_assert_false(yaml_parser_load_from_file(parser, path, NULL));
    g_unlink(path);
}

static void
test_parser_null_error_load_from_gfile(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autofree gchar *path = write_malformed_tmpfile();
    g_autoptr(GFile) file = g_file_new_for_path(path);

    g_assert_false(yaml_parser_load_from_gfile(parser, file, NULL, NULL));
    g_unlink(path);
}

static void
test_parser_null_error_load_from_stream(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GInputStream) stream = NULL;

    stream = g_memory_input_stream_new_from_data(MALFORMED_YAML, -1, NULL);
    g_assert_false(yaml_parser_load_from_stream(parser, stream, NULL, NULL));
}

/* Async completion: finish with a NULL GError ** and quit the loop */
static void
on_stream_loaded(
    GObject      *source,
    GAsyncResult *result,
    gpointer      user_data
)
{
    GMainLoop *loop = (GMainLoop *)user_data;

    g_assert_false(yaml_parser_load_from_stream_finish(YAML_PARSER(source),
                                                       result, NULL));
    g_main_loop_quit(loop);
}

static void
on_gfile_loaded(
    GObject      *source,
    GAsyncResult *result,
    gpointer      user_data
)
{
    GMainLoop *loop = (GMainLoop *)user_data;

    g_assert_false(yaml_parser_load_from_gfile_finish(YAML_PARSER(source),
                                                      result, NULL));
    g_main_loop_quit(loop);
}

static void
test_parser_null_error_load_from_stream_async(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GInputStream) stream = NULL;
    g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

    stream = g_memory_input_stream_new_from_data(MALFORMED_YAML, -1, NULL);
    yaml_parser_load_from_stream_async(parser, stream, NULL,
                                       on_stream_loaded, loop);
    g_main_loop_run(loop);
}

static void
test_parser_null_error_load_from_gfile_async(void)
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autofree gchar *path = write_malformed_tmpfile();
    g_autoptr(GFile) file = g_file_new_for_path(path);
    g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

    yaml_parser_load_from_gfile_async(parser, file, NULL,
                                      on_gfile_loaded, loop);
    g_main_loop_run(loop);
    g_unlink(path);
}

int
main(
    int   argc,
    char *argv[]
)
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/parser/simple_mapping", test_parser_simple_mapping);
    g_test_add_func("/parser/sequence", test_parser_sequence);
    g_test_add_func("/parser/nested", test_parser_nested);
    g_test_add_func("/parser/multi_document", test_parser_multi_document);
    g_test_add_func("/parser/immutable", test_parser_immutable);
    g_test_add_func("/parser/reset", test_parser_reset);
    g_test_add_func("/parser/scalar_types", test_parser_scalar_types);
    g_test_add_func("/parser/int_bases", test_parser_int_bases);
    g_test_add_func("/parser/error", test_parser_error);
    g_test_add_func("/parser/dup_root", test_parser_dup_root);
    g_test_add_func("/parser/steal_root", test_parser_steal_root);
    g_test_add_func("/parser/dup_document", test_parser_dup_document);
    g_test_add_func("/parser/set_immutable", test_parser_set_immutable);
    g_test_add_func("/parser/empty_input", test_parser_empty_input);
    g_test_add_func("/parser/explicit_length", test_parser_explicit_length);
    g_test_add_func("/parser/load_from_file", test_parser_load_from_file);
    g_test_add_func("/parser/load_from_file_not_found",
                    test_parser_load_from_file_not_found);
    g_test_add_func("/parser/error_tab_indentation",
                    test_parser_error_tab_indentation);
    g_test_add_func("/parser/null_values", test_parser_null_values);
    g_test_add_func("/parser/quoted_null_is_string", test_parser_quoted_null_is_string);
    g_test_add_func("/parser/self_referential_alias",
                    test_parser_self_referential_alias);
    g_test_add_func("/parser/repeated_alias_converted_once",
                    test_parser_repeated_alias_is_converted_once);
    g_test_add_func("/parser/null_error/load_from_data",
                    test_parser_null_error_load_from_data);
    g_test_add_func("/parser/null_error/load_from_file",
                    test_parser_null_error_load_from_file);
    g_test_add_func("/parser/null_error/load_from_gfile",
                    test_parser_null_error_load_from_gfile);
    g_test_add_func("/parser/null_error/load_from_stream",
                    test_parser_null_error_load_from_stream);
    g_test_add_func("/parser/null_error/load_from_stream_async",
                    test_parser_null_error_load_from_stream_async);
    g_test_add_func("/parser/null_error/load_from_gfile_async",
                    test_parser_null_error_load_from_gfile_async);

    return g_test_run();
}
