/* test_builder.c
 *
 * Copyright 2025 Zach Podbielniak
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Unit tests for YamlBuilder and YamlGenerator.
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include "yaml-glib.h"

/* Test building a simple mapping */
static void
test_builder_mapping(void)
{
    YamlBuilder *builder;
    YamlNode *root;
    YamlMapping *mapping;

    builder = yaml_builder_new();
    g_assert_nonnull(builder);

    yaml_builder_begin_mapping(builder);
    yaml_builder_set_member_name(builder, "name");
    yaml_builder_add_string_value(builder, "Alice");
    yaml_builder_set_member_name(builder, "age");
    yaml_builder_add_int_value(builder, 30);
    yaml_builder_set_member_name(builder, "active");
    yaml_builder_add_boolean_value(builder, TRUE);
    yaml_builder_end_mapping(builder);

    root = yaml_builder_get_root(builder);
    g_assert_nonnull(root);
    g_assert_cmpint(yaml_node_get_node_type(root), ==, YAML_NODE_MAPPING);

    mapping = yaml_node_get_mapping(root);
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "name"), ==, "Alice");
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "age"), ==, 30);
    g_assert_true(yaml_mapping_get_boolean_member(mapping, "active"));

    g_object_unref(builder);
}

/* Test building a sequence */
static void
test_builder_sequence(void)
{
    YamlBuilder *builder;
    YamlNode *root;
    YamlSequence *sequence;
    YamlNode *element;

    builder = yaml_builder_new();

    yaml_builder_begin_sequence(builder);
    yaml_builder_add_string_value(builder, "first");
    yaml_builder_add_string_value(builder, "second");
    yaml_builder_add_string_value(builder, "third");
    yaml_builder_end_sequence(builder);

    root = yaml_builder_get_root(builder);
    g_assert_cmpint(yaml_node_get_node_type(root), ==, YAML_NODE_SEQUENCE);

    sequence = yaml_node_get_sequence(root);
    g_assert_cmpuint(yaml_sequence_get_length(sequence), ==, 3);

    element = yaml_sequence_get_element(sequence, 0);
    g_assert_cmpstr(yaml_node_get_scalar(element), ==, "first");

    g_object_unref(builder);
}

/* Test building nested structures */
static void
test_builder_nested(void)
{
    YamlBuilder *builder;
    YamlNode *root;
    YamlMapping *mapping;
    YamlNode *nested_node;
    YamlSequence *sequence;

    builder = yaml_builder_new();

    yaml_builder_begin_mapping(builder);
    yaml_builder_set_member_name(builder, "items");
    yaml_builder_begin_sequence(builder);
    yaml_builder_add_string_value(builder, "a");
    yaml_builder_add_string_value(builder, "b");
    yaml_builder_end_sequence(builder);
    yaml_builder_set_member_name(builder, "metadata");
    yaml_builder_begin_mapping(builder);
    yaml_builder_set_member_name(builder, "count");
    yaml_builder_add_int_value(builder, 2);
    yaml_builder_end_mapping(builder);
    yaml_builder_end_mapping(builder);

    root = yaml_builder_get_root(builder);
    mapping = yaml_node_get_mapping(root);

    nested_node = yaml_mapping_get_member(mapping, "items");
    g_assert_cmpint(yaml_node_get_node_type(nested_node), ==, YAML_NODE_SEQUENCE);
    sequence = yaml_node_get_sequence(nested_node);
    g_assert_cmpuint(yaml_sequence_get_length(sequence), ==, 2);

    nested_node = yaml_mapping_get_member(mapping, "metadata");
    g_assert_cmpint(yaml_node_get_node_type(nested_node), ==, YAML_NODE_MAPPING);

    g_object_unref(builder);
}

/* Test builder chaining */
static void
test_builder_chaining(void)
{
    YamlBuilder *builder;
    YamlBuilder *result;
    YamlNode *root;

    builder = yaml_builder_new();

    /* All methods should return the builder for chaining */
    result = yaml_builder_begin_mapping(builder);
    g_assert_true(result == builder);

    result = yaml_builder_set_member_name(builder, "key");
    g_assert_true(result == builder);

    result = yaml_builder_add_string_value(builder, "value");
    g_assert_true(result == builder);

    result = yaml_builder_end_mapping(builder);
    g_assert_true(result == builder);

    root = yaml_builder_get_root(builder);
    g_assert_nonnull(root);

    g_object_unref(builder);
}

/* Test builder reset */
static void
test_builder_reset(void)
{
    YamlBuilder *builder;
    YamlNode *root;

    builder = yaml_builder_new();

    yaml_builder_begin_mapping(builder);
    yaml_builder_set_member_name(builder, "key");
    yaml_builder_add_string_value(builder, "value");
    yaml_builder_end_mapping(builder);

    root = yaml_builder_get_root(builder);
    g_assert_nonnull(root);

    yaml_builder_reset(builder);

    root = yaml_builder_get_root(builder);
    g_assert_null(root);

    g_object_unref(builder);
}

/* Test immutable builder */
static void
test_builder_immutable(void)
{
    YamlBuilder *builder;
    YamlNode *root;

    builder = yaml_builder_new_immutable();
    g_assert_true(yaml_builder_get_immutable(builder));

    yaml_builder_begin_mapping(builder);
    yaml_builder_set_member_name(builder, "key");
    yaml_builder_add_string_value(builder, "value");
    yaml_builder_end_mapping(builder);

    root = yaml_builder_get_root(builder);
    g_assert_true(yaml_node_is_immutable(root));

    g_object_unref(builder);
}

/* Test generator basic output */
static void
test_generator_basic(void)
{
    YamlBuilder *builder;
    YamlGenerator *generator;
    YamlNode *root;
    gchar *output;
    gsize length;
    GError *error = NULL;

    builder = yaml_builder_new();
    yaml_builder_begin_mapping(builder);
    yaml_builder_set_member_name(builder, "name");
    yaml_builder_add_string_value(builder, "test");
    yaml_builder_end_mapping(builder);

    root = yaml_builder_dup_root(builder);

    generator = yaml_generator_new();
    yaml_generator_set_root(generator, root);

    output = yaml_generator_to_data(generator, &length, &error);
    g_assert_no_error(error);
    g_assert_nonnull(output);
    g_assert_cmpuint(length, >, 0);

    /* Output should contain our key-value pair */
    g_assert_nonnull(strstr(output, "name"));
    g_assert_nonnull(strstr(output, "test"));

    g_free(output);
    yaml_node_unref(root);
    g_object_unref(generator);
    g_object_unref(builder);
}

/* Test generator configuration */
static void
test_generator_config(void)
{
    YamlGenerator *generator;

    generator = yaml_generator_new();

    /* Test indent */
    yaml_generator_set_indent(generator, 4);
    g_assert_cmpuint(yaml_generator_get_indent(generator), ==, 4);

    /* Test canonical */
    yaml_generator_set_canonical(generator, TRUE);
    g_assert_true(yaml_generator_get_canonical(generator));

    /* Test unicode */
    yaml_generator_set_unicode(generator, FALSE);
    g_assert_false(yaml_generator_get_unicode(generator));

    /* Test explicit markers */
    yaml_generator_set_explicit_start(generator, TRUE);
    g_assert_true(yaml_generator_get_explicit_start(generator));

    yaml_generator_set_explicit_end(generator, TRUE);
    g_assert_true(yaml_generator_get_explicit_end(generator));

    g_object_unref(generator);
}

/* Test roundtrip: parse -> build -> generate -> parse */
static void
test_roundtrip(void)
{
    YamlParser *parser1;
    YamlParser *parser2;
    YamlGenerator *generator;
    YamlNode *root1;
    YamlNode *root2;
    YamlMapping *mapping1;
    YamlMapping *mapping2;
    gchar *output;
    GError *error = NULL;
    const gchar *yaml = "name: John\nage: 30\n";

    /* Parse original */
    parser1 = yaml_parser_new();
    g_assert_true(yaml_parser_load_from_data(parser1, yaml, -1, &error));
    root1 = yaml_parser_dup_root(parser1);

    /* Generate YAML */
    generator = yaml_generator_new();
    yaml_generator_set_root(generator, root1);
    output = yaml_generator_to_data(generator, NULL, &error);
    g_assert_no_error(error);

    /* Parse generated YAML */
    parser2 = yaml_parser_new();
    g_assert_true(yaml_parser_load_from_data(parser2, output, -1, &error));
    root2 = yaml_parser_get_root(parser2);

    /* Compare results */
    mapping1 = yaml_node_get_mapping(root1);
    mapping2 = yaml_node_get_mapping(root2);

    g_assert_cmpstr(
        yaml_mapping_get_string_member(mapping1, "name"),
        ==,
        yaml_mapping_get_string_member(mapping2, "name")
    );
    g_assert_cmpint(
        yaml_mapping_get_int_member(mapping1, "age"),
        ==,
        yaml_mapping_get_int_member(mapping2, "age")
    );

    g_free(output);
    yaml_node_unref(root1);
    g_object_unref(parser1);
    g_object_unref(parser2);
    g_object_unref(generator);
}

/* Test builder null, double, and int value adds */
static void
test_builder_all_scalar_types(void)
{
    g_autoptr(YamlBuilder) builder = yaml_builder_new();
    YamlNode *root;
    YamlMapping *mapping;

    yaml_builder_begin_mapping(builder);

    yaml_builder_set_member_name(builder, "null_val");
    yaml_builder_add_null_value(builder);

    yaml_builder_set_member_name(builder, "double_val");
    yaml_builder_add_double_value(builder, 3.14);

    yaml_builder_set_member_name(builder, "int_val");
    yaml_builder_add_int_value(builder, 42);

    yaml_builder_set_member_name(builder, "str_val");
    yaml_builder_add_string_value(builder, "hello");

    yaml_builder_set_member_name(builder, "bool_val");
    yaml_builder_add_boolean_value(builder, TRUE);

    yaml_builder_end_mapping(builder);

    root = yaml_builder_get_root(builder);
    g_assert_nonnull(root);
    mapping = yaml_node_get_mapping(root);

    /* Verify null */
    YamlNode *null_node = yaml_mapping_get_member(mapping, "null_val");
    g_assert_nonnull(null_node);
    g_assert_true(yaml_node_is_null(null_node));

    /* Verify double */
    g_assert_cmpfloat_with_epsilon(
        yaml_mapping_get_double_member(mapping, "double_val"), 3.14, 0.001);

    /* Verify int */
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "int_val"), ==, 42);

    /* Verify string */
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "str_val"),
                    ==, "hello");

    /* Verify bool */
    g_assert_true(yaml_mapping_get_boolean_member(mapping, "bool_val"));
}

/* Test steal_root transfers ownership from builder */
static void
test_builder_steal_root(void)
{
    g_autoptr(YamlBuilder) builder = yaml_builder_new();

    yaml_builder_begin_mapping(builder);
    yaml_builder_set_member_name(builder, "key");
    yaml_builder_add_string_value(builder, "value");
    yaml_builder_end_mapping(builder);

    YamlNode *stolen = yaml_builder_steal_root(builder);
    g_assert_nonnull(stolen);

    /* After steal, get_root should return NULL */
    g_assert_null(yaml_builder_get_root(builder));

    yaml_node_unref(stolen);
}

/* Test get_document from builder */
static void
test_builder_get_document(void)
{
    g_autoptr(YamlBuilder) builder = yaml_builder_new();

    yaml_builder_begin_mapping(builder);
    yaml_builder_set_member_name(builder, "key");
    yaml_builder_add_string_value(builder, "value");
    yaml_builder_end_mapping(builder);

    YamlDocument *doc = yaml_builder_get_document(builder);
    g_assert_nonnull(doc);

    YamlNode *root = yaml_document_get_root(doc);
    g_assert_nonnull(root);
    g_assert_cmpint(yaml_node_get_node_type(root), ==, YAML_NODE_MAPPING);
}

/* Test add_value embeds an existing node */
static void
test_builder_add_value(void)
{
    g_autoptr(YamlBuilder) builder = yaml_builder_new();
    g_autoptr(YamlNode) existing = yaml_node_new_string("existing");

    yaml_builder_begin_mapping(builder);
    yaml_builder_set_member_name(builder, "embedded");
    yaml_builder_add_value(builder, existing);
    yaml_builder_end_mapping(builder);

    YamlNode *root = yaml_builder_get_root(builder);
    YamlMapping *mapping = yaml_node_get_mapping(root);
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "embedded"),
                    ==, "existing");
}

/* Test generator to_file */
static void
test_generator_to_file(void)
{
    g_autoptr(YamlBuilder) builder = yaml_builder_new();
    g_autoptr(YamlGenerator) generator = yaml_generator_new();
    g_autoptr(GError) error = NULL;
    g_autofree gchar *tmpfile = NULL;
    g_autofree gchar *contents = NULL;
    gsize length;
    gint fd;

    yaml_builder_begin_mapping(builder);
    yaml_builder_set_member_name(builder, "key");
    yaml_builder_add_string_value(builder, "value");
    yaml_builder_end_mapping(builder);

    yaml_generator_set_root(generator, yaml_builder_get_root(builder));

    fd = g_file_open_tmp("yaml-gen-XXXXXX", &tmpfile, &error);
    g_assert_no_error(error);
    close(fd);

    g_assert_true(yaml_generator_to_file(generator, tmpfile, &error));
    g_assert_no_error(error);

    /* Read back and verify */
    g_assert_true(g_file_get_contents(tmpfile, &contents, &length, &error));
    g_assert_nonnull(strstr(contents, "key"));
    g_assert_nonnull(strstr(contents, "value"));

    g_unlink(tmpfile);
}

/* Test generator explicit start/end markers in output */
static void
test_generator_explicit_markers(void)
{
    g_autoptr(YamlBuilder) builder = yaml_builder_new();
    g_autoptr(YamlGenerator) generator = yaml_generator_new();
    g_autoptr(GError) error = NULL;

    yaml_builder_begin_mapping(builder);
    yaml_builder_set_member_name(builder, "key");
    yaml_builder_add_string_value(builder, "value");
    yaml_builder_end_mapping(builder);

    yaml_generator_set_root(generator, yaml_builder_get_root(builder));
    yaml_generator_set_explicit_start(generator, TRUE);
    yaml_generator_set_explicit_end(generator, TRUE);

    g_autofree gchar *output = yaml_generator_to_data(generator, NULL, &error);
    g_assert_no_error(error);
    g_assert_nonnull(output);

    /* Output should start with --- and end with ... */
    g_assert_nonnull(strstr(output, "---"));
    g_assert_nonnull(strstr(output, "..."));
}

/* Test generator get_root returns what was set */
static void
test_generator_get_root(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("hello");
    g_autoptr(YamlGenerator) generator = yaml_generator_new();

    yaml_generator_set_root(generator, node);
    g_assert_true(yaml_generator_get_root(generator) == node);
}

int
main(
    int   argc,
    char *argv[]
)
{
    g_test_init(&argc, &argv, NULL);

    /* Builder tests */
    g_test_add_func("/builder/mapping", test_builder_mapping);
    g_test_add_func("/builder/sequence", test_builder_sequence);
    g_test_add_func("/builder/nested", test_builder_nested);
    g_test_add_func("/builder/chaining", test_builder_chaining);
    g_test_add_func("/builder/reset", test_builder_reset);
    g_test_add_func("/builder/immutable", test_builder_immutable);
    g_test_add_func("/builder/all_scalar_types", test_builder_all_scalar_types);
    g_test_add_func("/builder/steal_root", test_builder_steal_root);
    g_test_add_func("/builder/get_document", test_builder_get_document);
    g_test_add_func("/builder/add_value", test_builder_add_value);

    /* Generator tests */
    g_test_add_func("/generator/basic", test_generator_basic);
    g_test_add_func("/generator/config", test_generator_config);
    g_test_add_func("/generator/to_file", test_generator_to_file);
    g_test_add_func("/generator/explicit_markers",
                    test_generator_explicit_markers);
    g_test_add_func("/generator/get_root", test_generator_get_root);

    /* Integration tests */
    g_test_add_func("/integration/roundtrip", test_roundtrip);

    return g_test_run();
}
