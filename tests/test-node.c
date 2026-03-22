/* test_node.c
 *
 * Copyright 2025 Zach Podbielniak
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Unit tests for YamlNode, YamlMapping, and YamlSequence.
 */

#include <glib.h>
#include "yaml-glib.h"

/* Test scalar node creation and value retrieval */
static void
test_node_scalar_string(void)
{
    YamlNode *node;

    node = yaml_node_new_string("hello world");
    g_assert_nonnull(node);
    g_assert_cmpint(yaml_node_get_node_type(node), ==, YAML_NODE_SCALAR);
    g_assert_cmpstr(yaml_node_get_scalar(node), ==, "hello world");

    yaml_node_unref(node);
}

static void
test_node_scalar_int(void)
{
    YamlNode *node;

    node = yaml_node_new_int(42);
    g_assert_nonnull(node);
    g_assert_cmpint(yaml_node_get_node_type(node), ==, YAML_NODE_SCALAR);
    g_assert_cmpint(yaml_node_get_int(node), ==, 42);

    yaml_node_unref(node);
}

static void
test_node_scalar_double(void)
{
    YamlNode *node;

    node = yaml_node_new_double(3.14159);
    g_assert_nonnull(node);
    g_assert_cmpint(yaml_node_get_node_type(node), ==, YAML_NODE_SCALAR);
    g_assert_cmpfloat_with_epsilon(yaml_node_get_double(node), 3.14159, 0.00001);

    yaml_node_unref(node);
}

static void
test_node_scalar_boolean(void)
{
    YamlNode *node_true;
    YamlNode *node_false;

    node_true = yaml_node_new_boolean(TRUE);
    g_assert_nonnull(node_true);
    g_assert_true(yaml_node_get_boolean(node_true));

    node_false = yaml_node_new_boolean(FALSE);
    g_assert_nonnull(node_false);
    g_assert_false(yaml_node_get_boolean(node_false));

    yaml_node_unref(node_true);
    yaml_node_unref(node_false);
}

static void
test_node_null(void)
{
    YamlNode *node;

    node = yaml_node_new_null();
    g_assert_nonnull(node);
    g_assert_cmpint(yaml_node_get_node_type(node), ==, YAML_NODE_NULL);

    yaml_node_unref(node);
}

/* Test mapping operations */
static void
test_mapping_basic(void)
{
    YamlMapping *mapping;
    YamlNode *node;

    mapping = yaml_mapping_new();
    g_assert_nonnull(mapping);
    g_assert_cmpuint(yaml_mapping_get_size(mapping), ==, 0);

    /* Add some values */
    yaml_mapping_set_string_member(mapping, "name", "test");
    yaml_mapping_set_int_member(mapping, "count", 42);
    yaml_mapping_set_boolean_member(mapping, "active", TRUE);

    g_assert_cmpuint(yaml_mapping_get_size(mapping), ==, 3);
    g_assert_true(yaml_mapping_has_member(mapping, "name"));
    g_assert_true(yaml_mapping_has_member(mapping, "count"));
    g_assert_true(yaml_mapping_has_member(mapping, "active"));
    g_assert_false(yaml_mapping_has_member(mapping, "nonexistent"));

    /* Retrieve values */
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "name"), ==, "test");
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "count"), ==, 42);
    g_assert_true(yaml_mapping_get_boolean_member(mapping, "active"));

    /* Create node from mapping */
    node = yaml_node_new_mapping(mapping);
    g_assert_nonnull(node);
    g_assert_cmpint(yaml_node_get_node_type(node), ==, YAML_NODE_MAPPING);

    /* Verify we can get the mapping back */
    g_assert_true(yaml_node_get_mapping(node) == mapping);

    yaml_node_unref(node);
    yaml_mapping_unref(mapping);
}

static void
test_mapping_key_order(void)
{
    YamlMapping *mapping;
    const gchar *key;

    mapping = yaml_mapping_new();

    yaml_mapping_set_string_member(mapping, "first", "1");
    yaml_mapping_set_string_member(mapping, "second", "2");
    yaml_mapping_set_string_member(mapping, "third", "3");

    /* Keys should be returned in insertion order */
    key = yaml_mapping_get_key(mapping, 0);
    g_assert_cmpstr(key, ==, "first");

    key = yaml_mapping_get_key(mapping, 1);
    g_assert_cmpstr(key, ==, "second");

    key = yaml_mapping_get_key(mapping, 2);
    g_assert_cmpstr(key, ==, "third");

    yaml_mapping_unref(mapping);
}

static void
test_mapping_remove(void)
{
    YamlMapping *mapping;

    mapping = yaml_mapping_new();

    yaml_mapping_set_string_member(mapping, "a", "1");
    yaml_mapping_set_string_member(mapping, "b", "2");
    yaml_mapping_set_string_member(mapping, "c", "3");

    g_assert_cmpuint(yaml_mapping_get_size(mapping), ==, 3);

    yaml_mapping_remove_member(mapping, "b");

    g_assert_cmpuint(yaml_mapping_get_size(mapping), ==, 2);
    g_assert_true(yaml_mapping_has_member(mapping, "a"));
    g_assert_false(yaml_mapping_has_member(mapping, "b"));
    g_assert_true(yaml_mapping_has_member(mapping, "c"));

    yaml_mapping_unref(mapping);
}

/* Test sequence operations */
static void
test_sequence_basic(void)
{
    YamlSequence *sequence;
    YamlNode *node;
    YamlNode *element;

    sequence = yaml_sequence_new();
    g_assert_nonnull(sequence);
    g_assert_cmpuint(yaml_sequence_get_length(sequence), ==, 0);

    /* Add some elements */
    yaml_sequence_add_string_element(sequence, "one");
    yaml_sequence_add_string_element(sequence, "two");
    yaml_sequence_add_string_element(sequence, "three");

    g_assert_cmpuint(yaml_sequence_get_length(sequence), ==, 3);

    /* Retrieve elements */
    element = yaml_sequence_get_element(sequence, 0);
    g_assert_nonnull(element);
    g_assert_cmpstr(yaml_node_get_scalar(element), ==, "one");

    element = yaml_sequence_get_element(sequence, 1);
    g_assert_cmpstr(yaml_node_get_scalar(element), ==, "two");

    element = yaml_sequence_get_element(sequence, 2);
    g_assert_cmpstr(yaml_node_get_scalar(element), ==, "three");

    /* Create node from sequence */
    node = yaml_node_new_sequence(sequence);
    g_assert_nonnull(node);
    g_assert_cmpint(yaml_node_get_node_type(node), ==, YAML_NODE_SEQUENCE);

    yaml_node_unref(node);
    yaml_sequence_unref(sequence);
}

static void
test_sequence_mixed_types(void)
{
    YamlSequence *sequence;
    YamlNode *element;

    sequence = yaml_sequence_new();

    yaml_sequence_add_string_element(sequence, "hello");
    yaml_sequence_add_int_element(sequence, 42);
    yaml_sequence_add_double_element(sequence, 3.14);
    yaml_sequence_add_boolean_element(sequence, TRUE);
    yaml_sequence_add_null_element(sequence);

    g_assert_cmpuint(yaml_sequence_get_length(sequence), ==, 5);

    element = yaml_sequence_get_element(sequence, 0);
    g_assert_cmpint(yaml_node_get_node_type(element), ==, YAML_NODE_SCALAR);

    element = yaml_sequence_get_element(sequence, 4);
    g_assert_cmpint(yaml_node_get_node_type(element), ==, YAML_NODE_NULL);

    yaml_sequence_unref(sequence);
}

/* Test node sealing (immutability) */
static void
test_node_seal(void)
{
    YamlMapping *mapping;
    YamlNode *node;

    mapping = yaml_mapping_new();
    yaml_mapping_set_string_member(mapping, "key", "value");

    node = yaml_node_new_mapping(mapping);
    g_assert_false(yaml_node_is_immutable(node));

    yaml_node_seal(node);
    g_assert_true(yaml_node_is_immutable(node));

    yaml_node_unref(node);
    yaml_mapping_unref(mapping);
}

/* Test node reference counting */
static void
test_node_refcount(void)
{
    YamlNode *node;
    YamlNode *ref;

    node = yaml_node_new_string("test");
    g_assert_nonnull(node);

    ref = yaml_node_ref(node);
    g_assert_true(ref == node);

    /* Should not crash - both refs should be valid */
    g_assert_cmpstr(yaml_node_get_scalar(node), ==, "test");
    g_assert_cmpstr(yaml_node_get_scalar(ref), ==, "test");

    yaml_node_unref(ref);
    yaml_node_unref(node);
}

/* ================================================================
 * Node edge case tests
 * ================================================================ */

/* Verify is_null returns TRUE for null nodes and FALSE for non-null */
static void
test_node_is_null(void)
{
    g_autoptr(YamlNode) null_node = yaml_node_new_null();
    g_autoptr(YamlNode) str_node = yaml_node_new_string("not null");

    g_assert_nonnull(null_node);
    g_assert_true(yaml_node_is_null(null_node));

    g_assert_nonnull(str_node);
    g_assert_false(yaml_node_is_null(str_node));
}

/* Verify deep copy of string and mapping nodes */
static void
test_node_copy(void)
{
    g_autoptr(YamlNode) str_node = NULL;
    g_autoptr(YamlNode) str_copy = NULL;
    g_autoptr(YamlMapping) mapping = NULL;
    g_autoptr(YamlNode) map_node = NULL;
    g_autoptr(YamlNode) map_copy = NULL;
    YamlMapping *copy_mapping;

    /* Copy a string node and verify the value matches */
    str_node = yaml_node_new_string("original");
    str_copy = yaml_node_copy(str_node);

    g_assert_nonnull(str_copy);
    g_assert_true(str_copy != str_node);
    g_assert_cmpint(yaml_node_get_node_type(str_copy), ==, YAML_NODE_SCALAR);
    g_assert_cmpstr(yaml_node_get_string(str_copy), ==, "original");

    /* Copy a mapping node with members and verify deep copy */
    mapping = yaml_mapping_new();
    yaml_mapping_set_string_member(mapping, "key1", "value1");
    yaml_mapping_set_int_member(mapping, "key2", 99);

    map_node = yaml_node_new_mapping(mapping);
    map_copy = yaml_node_copy(map_node);

    g_assert_nonnull(map_copy);
    g_assert_true(map_copy != map_node);
    g_assert_cmpint(yaml_node_get_node_type(map_copy), ==, YAML_NODE_MAPPING);

    copy_mapping = yaml_node_get_mapping(map_copy);
    g_assert_nonnull(copy_mapping);
    g_assert_cmpuint(yaml_mapping_get_size(copy_mapping), ==, 2);
    g_assert_cmpstr(yaml_mapping_get_string_member(copy_mapping, "key1"), ==, "value1");
    g_assert_cmpint(yaml_mapping_get_int_member(copy_mapping, "key2"), ==, 99);
}

/* Verify equality checks for nodes of same and different types/values */
static void
test_node_equal(void)
{
    g_autoptr(YamlNode) str_a = yaml_node_new_string("hello");
    g_autoptr(YamlNode) str_b = yaml_node_new_string("hello");
    g_autoptr(YamlNode) str_c = yaml_node_new_string("world");
    g_autoptr(YamlNode) int_a = yaml_node_new_int(42);
    g_autoptr(YamlNode) int_b = yaml_node_new_int(42);
    g_autoptr(YamlNode) int_c = yaml_node_new_int(99);

    /* Equal string nodes */
    g_assert_true(yaml_node_equal(str_a, str_b));

    /* Equal int nodes */
    g_assert_true(yaml_node_equal(int_a, int_b));

    /* Unequal same-type nodes */
    g_assert_false(yaml_node_equal(str_a, str_c));
    g_assert_false(yaml_node_equal(int_a, int_c));

    /* Different types should not be equal */
    g_assert_false(yaml_node_equal(str_a, int_a));
}

/* Verify that equal values produce the same hash */
static void
test_node_hash(void)
{
    g_autoptr(YamlNode) a = yaml_node_new_string("test");
    g_autoptr(YamlNode) b = yaml_node_new_string("test");
    g_autoptr(YamlNode) c = yaml_node_new_string("different");
    guint hash_a;
    guint hash_b;
    guint hash_c;

    hash_a = yaml_node_hash(a);
    hash_b = yaml_node_hash(b);
    hash_c = yaml_node_hash(c);

    /* Same values must produce same hash */
    g_assert_cmpuint(hash_a, ==, hash_b);

    /* Different values may differ (not guaranteed, but very likely) */
    (void)hash_c;
}

/* Verify set_string, set_int, set_double, set_boolean mutators */
static void
test_node_setters(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("initial");

    /* Change string value */
    yaml_node_set_string(node, "changed");
    g_assert_cmpstr(yaml_node_get_string(node), ==, "changed");

    /* Change to int */
    yaml_node_set_int(node, 123);
    g_assert_cmpint(yaml_node_get_int(node), ==, 123);

    /* Change to double */
    yaml_node_set_double(node, 2.718);
    g_assert_cmpfloat_with_epsilon(yaml_node_get_double(node), 2.718, 0.001);

    /* Change to boolean */
    yaml_node_set_boolean(node, TRUE);
    g_assert_true(yaml_node_get_boolean(node));
}

/* Verify set_tag and get_tag, including NULL tag and tag change */
static void
test_node_tag(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("tagged");

    /* Initially no tag */
    g_assert_null(yaml_node_get_tag(node));

    /* Set a tag */
    yaml_node_set_tag(node, "!!str");
    g_assert_cmpstr(yaml_node_get_tag(node), ==, "!!str");

    /* Change the tag */
    yaml_node_set_tag(node, "!!custom");
    g_assert_cmpstr(yaml_node_get_tag(node), ==, "!!custom");

    /* Clear the tag by setting NULL */
    yaml_node_set_tag(node, NULL);
    g_assert_null(yaml_node_get_tag(node));
}

/* Verify set_anchor and get_anchor, including NULL anchor */
static void
test_node_anchor(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("anchored");

    /* Initially no anchor */
    g_assert_null(yaml_node_get_anchor(node));

    /* Set an anchor */
    yaml_node_set_anchor(node, "my_anchor");
    g_assert_cmpstr(yaml_node_get_anchor(node), ==, "my_anchor");

    /* Clear anchor by setting NULL */
    yaml_node_set_anchor(node, NULL);
    g_assert_null(yaml_node_get_anchor(node));
}

/* Verify set and get for all scalar styles */
static void
test_node_scalar_style(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("styled");

    /* Default should be ANY */
    g_assert_cmpint(yaml_node_get_scalar_style(node), ==, YAML_SCALAR_STYLE_ANY);

    /* Set PLAIN */
    yaml_node_set_scalar_style(node, YAML_SCALAR_STYLE_PLAIN);
    g_assert_cmpint(yaml_node_get_scalar_style(node), ==, YAML_SCALAR_STYLE_PLAIN);

    /* Set LITERAL */
    yaml_node_set_scalar_style(node, YAML_SCALAR_STYLE_LITERAL);
    g_assert_cmpint(yaml_node_get_scalar_style(node), ==, YAML_SCALAR_STYLE_LITERAL);

    /* Set FOLDED */
    yaml_node_set_scalar_style(node, YAML_SCALAR_STYLE_FOLDED);
    g_assert_cmpint(yaml_node_get_scalar_style(node), ==, YAML_SCALAR_STYLE_FOLDED);

    /* Set SINGLE_QUOTED */
    yaml_node_set_scalar_style(node, YAML_SCALAR_STYLE_SINGLE_QUOTED);
    g_assert_cmpint(yaml_node_get_scalar_style(node), ==, YAML_SCALAR_STYLE_SINGLE_QUOTED);

    /* Set DOUBLE_QUOTED */
    yaml_node_set_scalar_style(node, YAML_SCALAR_STYLE_DOUBLE_QUOTED);
    g_assert_cmpint(yaml_node_get_scalar_style(node), ==, YAML_SCALAR_STYLE_DOUBLE_QUOTED);
}

/* Verify set_parent and get_parent on a node */
static void
test_node_parent(void)
{
    g_autoptr(YamlNode) parent = yaml_node_new_mapping(NULL);
    g_autoptr(YamlNode) child = yaml_node_new_string("child");

    /* Initially no parent */
    g_assert_null(yaml_node_get_parent(child));

    /* Set parent */
    yaml_node_set_parent(child, parent);
    g_assert_true(yaml_node_get_parent(child) == parent);

    /* Clear parent */
    yaml_node_set_parent(child, NULL);
    g_assert_null(yaml_node_get_parent(child));
}

/* Verify dup_string returns an owned copy of the string */
static void
test_node_dup_string(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("owned copy");
    g_autofree gchar *dup = NULL;

    dup = yaml_node_dup_string(node);
    g_assert_nonnull(dup);
    g_assert_cmpstr(dup, ==, "owned copy");

    /* Verify it is a distinct allocation from the internal string */
    g_assert_true(dup != yaml_node_get_string(node));
}

/* Verify type mismatch accessors return default/fallback values.
 *
 * All scalars (string, int, double, boolean) share YAML_NODE_SCALAR.
 * get_string returns the underlying scalar text for ANY scalar node.
 * get_int/get_double attempt to parse the scalar string as a number.
 * get_boolean returns FALSE if the node was not created as boolean.
 * get_mapping/get_sequence return NULL for scalar nodes.
 */
static void
test_node_type_mismatch(void)
{
    g_autoptr(YamlNode) str_node = yaml_node_new_string("hello");
    g_autoptr(YamlNode) int_node = yaml_node_new_int(42);
    g_autoptr(YamlNode) null_node = yaml_node_new_null();

    /* get_int on a non-numeric string parses to 0 */
    g_assert_cmpint(yaml_node_get_int(str_node), ==, 0);

    /* get_string on an int node returns the scalar representation */
    g_assert_nonnull(yaml_node_get_string(int_node));
    g_assert_cmpstr(yaml_node_get_string(int_node), ==, "42");

    /* get_boolean on a non-boolean scalar returns FALSE */
    g_assert_false(yaml_node_get_boolean(str_node));

    /* get_double on a non-numeric string parses to 0.0 */
    g_assert_cmpfloat_with_epsilon(yaml_node_get_double(str_node), 0.0, 0.001);

    /* get_mapping on a scalar node returns NULL */
    g_assert_null(yaml_node_get_mapping(str_node));

    /* get_sequence on a scalar node returns NULL */
    g_assert_null(yaml_node_get_sequence(str_node));

    /* get_string on a null node returns NULL (not a scalar) */
    g_assert_null(yaml_node_get_string(null_node));

    /* get_int on a null node returns 0 */
    g_assert_cmpint(yaml_node_get_int(null_node), ==, 0);
}

/* ================================================================
 * Mapping edge case tests
 * ================================================================ */

/* Verify a new empty mapping has size 0 and has_member returns FALSE */
static void
test_mapping_empty(void)
{
    g_autoptr(YamlMapping) mapping = yaml_mapping_new();

    g_assert_nonnull(mapping);
    g_assert_cmpuint(yaml_mapping_get_size(mapping), ==, 0);
    g_assert_false(yaml_mapping_has_member(mapping, "anything"));
}

/* Verify set/get double members on a mapping */
static void
test_mapping_double_member(void)
{
    g_autoptr(YamlMapping) mapping = yaml_mapping_new();

    yaml_mapping_set_double_member(mapping, "pi", 3.14159);
    yaml_mapping_set_double_member(mapping, "e", 2.71828);

    g_assert_cmpuint(yaml_mapping_get_size(mapping), ==, 2);
    g_assert_cmpfloat_with_epsilon(
        yaml_mapping_get_double_member(mapping, "pi"), 3.14159, 0.00001);
    g_assert_cmpfloat_with_epsilon(
        yaml_mapping_get_double_member(mapping, "e"), 2.71828, 0.00001);
}

/* Verify set/get null members on a mapping */
static void
test_mapping_null_member(void)
{
    g_autoptr(YamlMapping) mapping = yaml_mapping_new();

    yaml_mapping_set_null_member(mapping, "nothing");
    yaml_mapping_set_string_member(mapping, "something", "value");

    g_assert_cmpuint(yaml_mapping_get_size(mapping), ==, 2);
    g_assert_true(yaml_mapping_get_null_member(mapping, "nothing"));
    g_assert_false(yaml_mapping_get_null_member(mapping, "something"));
}

/* Verify nested mapping and sequence members */
static void
test_mapping_nested(void)
{
    g_autoptr(YamlMapping) outer = yaml_mapping_new();
    g_autoptr(YamlMapping) inner = yaml_mapping_new();
    g_autoptr(YamlSequence) seq = yaml_sequence_new();
    YamlMapping *retrieved_map;
    YamlSequence *retrieved_seq;

    /* Build inner mapping */
    yaml_mapping_set_string_member(inner, "nested_key", "nested_value");

    /* Build inner sequence */
    yaml_sequence_add_string_element(seq, "item1");
    yaml_sequence_add_int_element(seq, 42);

    /* Set nested members */
    yaml_mapping_set_mapping_member(outer, "child_map", inner);
    yaml_mapping_set_sequence_member(outer, "child_seq", seq);

    g_assert_cmpuint(yaml_mapping_get_size(outer), ==, 2);

    /* Retrieve and verify nested mapping */
    retrieved_map = yaml_mapping_get_mapping_member(outer, "child_map");
    g_assert_nonnull(retrieved_map);
    g_assert_cmpstr(
        yaml_mapping_get_string_member(retrieved_map, "nested_key"),
        ==, "nested_value");

    /* Retrieve and verify nested sequence */
    retrieved_seq = yaml_mapping_get_sequence_member(outer, "child_seq");
    g_assert_nonnull(retrieved_seq);
    g_assert_cmpuint(yaml_sequence_get_length(retrieved_seq), ==, 2);
    g_assert_cmpstr(yaml_sequence_get_string_element(retrieved_seq, 0), ==, "item1");
    g_assert_cmpint(yaml_sequence_get_int_element(retrieved_seq, 1), ==, 42);
}

/* Callback data structure for foreach test */
typedef struct {
    guint  count;
    GList *keys;
} ForeachMappingData;

/* Callback that records visited member names */
static void
mapping_foreach_cb(
    YamlMapping *mapping,
    const gchar *member_name,
    YamlNode    *member_node,
    gpointer     user_data
)
{
    ForeachMappingData *data = (ForeachMappingData *)user_data;

    (void)mapping;
    (void)member_node;

    data->count++;
    data->keys = g_list_append(data->keys, (gpointer)member_name);
}

/* Verify foreach_member visits all members */
static void
test_mapping_foreach(void)
{
    g_autoptr(YamlMapping) mapping = yaml_mapping_new();
    ForeachMappingData data = { 0, NULL };

    yaml_mapping_set_string_member(mapping, "alpha", "a");
    yaml_mapping_set_string_member(mapping, "beta", "b");
    yaml_mapping_set_string_member(mapping, "gamma", "c");

    yaml_mapping_foreach_member(mapping, mapping_foreach_cb, &data);

    g_assert_cmpuint(data.count, ==, 3);

    /* Verify all keys were visited */
    g_assert_nonnull(g_list_find_custom(data.keys, "alpha", (GCompareFunc)g_strcmp0));
    g_assert_nonnull(g_list_find_custom(data.keys, "beta", (GCompareFunc)g_strcmp0));
    g_assert_nonnull(g_list_find_custom(data.keys, "gamma", (GCompareFunc)g_strcmp0));

    g_list_free(data.keys);
}

/* Verify get_members returns a list of all member names */
static void
test_mapping_get_members(void)
{
    g_autoptr(YamlMapping) mapping = yaml_mapping_new();
    GList *members;

    yaml_mapping_set_string_member(mapping, "x", "1");
    yaml_mapping_set_string_member(mapping, "y", "2");
    yaml_mapping_set_string_member(mapping, "z", "3");

    members = yaml_mapping_get_members(mapping);
    g_assert_nonnull(members);
    g_assert_cmpuint(g_list_length(members), ==, 3);

    /* Check all expected keys are present */
    g_assert_nonnull(g_list_find_custom(members, "x", (GCompareFunc)g_strcmp0));
    g_assert_nonnull(g_list_find_custom(members, "y", (GCompareFunc)g_strcmp0));
    g_assert_nonnull(g_list_find_custom(members, "z", (GCompareFunc)g_strcmp0));

    g_list_free(members);
}

/* Verify sealing makes a mapping immutable */
static void
test_mapping_seal(void)
{
    g_autoptr(YamlMapping) mapping = yaml_mapping_new();

    yaml_mapping_set_string_member(mapping, "key", "value");

    g_assert_false(yaml_mapping_is_immutable(mapping));

    yaml_mapping_seal(mapping);

    g_assert_true(yaml_mapping_is_immutable(mapping));
}

/* Verify equality for identical and different mappings */
static void
test_mapping_equal(void)
{
    g_autoptr(YamlMapping) a = yaml_mapping_new();
    g_autoptr(YamlMapping) b = yaml_mapping_new();
    g_autoptr(YamlMapping) c = yaml_mapping_new();

    yaml_mapping_set_string_member(a, "name", "alice");
    yaml_mapping_set_int_member(a, "age", 30);

    yaml_mapping_set_string_member(b, "name", "alice");
    yaml_mapping_set_int_member(b, "age", 30);

    yaml_mapping_set_string_member(c, "name", "bob");
    yaml_mapping_set_int_member(c, "age", 25);

    /* Identical content should be equal */
    g_assert_true(yaml_mapping_equal(a, b));

    /* Different content should not be equal */
    g_assert_false(yaml_mapping_equal(a, c));
}

/* Verify that overwriting a key replaces the value without increasing size */
static void
test_mapping_overwrite(void)
{
    g_autoptr(YamlMapping) mapping = yaml_mapping_new();

    yaml_mapping_set_string_member(mapping, "key", "first");
    g_assert_cmpuint(yaml_mapping_get_size(mapping), ==, 1);
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "key"), ==, "first");

    /* Overwrite the same key */
    yaml_mapping_set_string_member(mapping, "key", "second");
    g_assert_cmpuint(yaml_mapping_get_size(mapping), ==, 1);
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "key"), ==, "second");
}

/* ================================================================
 * Sequence edge case tests
 * ================================================================ */

/* Verify a new empty sequence has length 0 */
static void
test_sequence_empty(void)
{
    g_autoptr(YamlSequence) seq = yaml_sequence_new();

    g_assert_nonnull(seq);
    g_assert_cmpuint(yaml_sequence_get_length(seq), ==, 0);
}

/* Verify typed getter convenience functions on a sequence */
static void
test_sequence_typed_getters(void)
{
    g_autoptr(YamlSequence) seq = yaml_sequence_new();

    yaml_sequence_add_string_element(seq, "hello");
    yaml_sequence_add_int_element(seq, 99);
    yaml_sequence_add_double_element(seq, 1.5);
    yaml_sequence_add_boolean_element(seq, TRUE);

    g_assert_cmpstr(yaml_sequence_get_string_element(seq, 0), ==, "hello");
    g_assert_cmpint(yaml_sequence_get_int_element(seq, 1), ==, 99);
    g_assert_cmpfloat_with_epsilon(
        yaml_sequence_get_double_element(seq, 2), 1.5, 0.001);
    g_assert_true(yaml_sequence_get_boolean_element(seq, 3));
}

/* Verify removing an element decreases the length */
static void
test_sequence_remove(void)
{
    g_autoptr(YamlSequence) seq = yaml_sequence_new();

    yaml_sequence_add_string_element(seq, "a");
    yaml_sequence_add_string_element(seq, "b");
    yaml_sequence_add_string_element(seq, "c");

    g_assert_cmpuint(yaml_sequence_get_length(seq), ==, 3);

    /* Remove the middle element */
    yaml_sequence_remove_element(seq, 1);

    g_assert_cmpuint(yaml_sequence_get_length(seq), ==, 2);
    g_assert_cmpstr(yaml_sequence_get_string_element(seq, 0), ==, "a");
    g_assert_cmpstr(yaml_sequence_get_string_element(seq, 1), ==, "c");
}

/* Callback data structure for sequence foreach test */
typedef struct {
    guint  count;
    gint64 sum;
} ForeachSequenceData;

/* Callback that counts elements and sums int values */
static void
sequence_foreach_cb(
    YamlSequence *sequence,
    guint         index_,
    YamlNode     *element_node,
    gpointer      user_data
)
{
    ForeachSequenceData *data = (ForeachSequenceData *)user_data;

    (void)sequence;
    (void)index_;

    data->count++;
    if (yaml_node_get_node_type(element_node) == YAML_NODE_SCALAR) {
        data->sum += yaml_node_get_int(element_node);
    }
}

/* Verify foreach_element visits all elements */
static void
test_sequence_foreach(void)
{
    g_autoptr(YamlSequence) seq = yaml_sequence_new();
    ForeachSequenceData data = { 0, 0 };

    yaml_sequence_add_int_element(seq, 10);
    yaml_sequence_add_int_element(seq, 20);
    yaml_sequence_add_int_element(seq, 30);

    yaml_sequence_foreach_element(seq, sequence_foreach_cb, &data);

    g_assert_cmpuint(data.count, ==, 3);
    g_assert_cmpint(data.sum, ==, 60);
}

/* Verify sealing makes a sequence immutable */
static void
test_sequence_seal(void)
{
    g_autoptr(YamlSequence) seq = yaml_sequence_new();

    yaml_sequence_add_string_element(seq, "item");

    g_assert_false(yaml_sequence_is_immutable(seq));

    yaml_sequence_seal(seq);

    g_assert_true(yaml_sequence_is_immutable(seq));
}

/* Verify equality for identical and different sequences */
static void
test_sequence_equal(void)
{
    g_autoptr(YamlSequence) a = yaml_sequence_new();
    g_autoptr(YamlSequence) b = yaml_sequence_new();
    g_autoptr(YamlSequence) c = yaml_sequence_new();

    yaml_sequence_add_string_element(a, "one");
    yaml_sequence_add_int_element(a, 2);

    yaml_sequence_add_string_element(b, "one");
    yaml_sequence_add_int_element(b, 2);

    yaml_sequence_add_string_element(c, "one");
    yaml_sequence_add_int_element(c, 3);

    /* Identical content should be equal */
    g_assert_true(yaml_sequence_equal(a, b));

    /* Different content should not be equal */
    g_assert_false(yaml_sequence_equal(a, c));
}

/* Verify nested mapping and sequence elements in a sequence */
static void
test_sequence_nested(void)
{
    g_autoptr(YamlSequence) outer = yaml_sequence_new();
    g_autoptr(YamlMapping) inner_map = yaml_mapping_new();
    g_autoptr(YamlSequence) inner_seq = yaml_sequence_new();
    YamlMapping *retrieved_map;
    YamlSequence *retrieved_seq;

    /* Build nested structures */
    yaml_mapping_set_string_member(inner_map, "key", "value");
    yaml_sequence_add_int_element(inner_seq, 1);
    yaml_sequence_add_int_element(inner_seq, 2);

    /* Add nested structures to the outer sequence */
    yaml_sequence_add_mapping_element(outer, inner_map);
    yaml_sequence_add_sequence_element(outer, inner_seq);

    g_assert_cmpuint(yaml_sequence_get_length(outer), ==, 2);

    /* Retrieve and verify nested mapping */
    retrieved_map = yaml_sequence_get_mapping_element(outer, 0);
    g_assert_nonnull(retrieved_map);
    g_assert_cmpstr(
        yaml_mapping_get_string_member(retrieved_map, "key"), ==, "value");

    /* Retrieve and verify nested sequence */
    retrieved_seq = yaml_sequence_get_sequence_element(outer, 1);
    g_assert_nonnull(retrieved_seq);
    g_assert_cmpuint(yaml_sequence_get_length(retrieved_seq), ==, 2);
    g_assert_cmpint(yaml_sequence_get_int_element(retrieved_seq, 0), ==, 1);
    g_assert_cmpint(yaml_sequence_get_int_element(retrieved_seq, 1), ==, 2);
}

int
main(
    int   argc,
    char *argv[]
)
{
    g_test_init(&argc, &argv, NULL);

    /* Scalar tests */
    g_test_add_func("/node/scalar/string", test_node_scalar_string);
    g_test_add_func("/node/scalar/int", test_node_scalar_int);
    g_test_add_func("/node/scalar/double", test_node_scalar_double);
    g_test_add_func("/node/scalar/boolean", test_node_scalar_boolean);
    g_test_add_func("/node/null", test_node_null);

    /* Mapping tests */
    g_test_add_func("/mapping/basic", test_mapping_basic);
    g_test_add_func("/mapping/key_order", test_mapping_key_order);
    g_test_add_func("/mapping/remove", test_mapping_remove);

    /* Sequence tests */
    g_test_add_func("/sequence/basic", test_sequence_basic);
    g_test_add_func("/sequence/mixed_types", test_sequence_mixed_types);

    /* Node features */
    g_test_add_func("/node/seal", test_node_seal);
    g_test_add_func("/node/refcount", test_node_refcount);

    /* Node edge cases */
    g_test_add_func("/node/is_null", test_node_is_null);
    g_test_add_func("/node/copy", test_node_copy);
    g_test_add_func("/node/equal", test_node_equal);
    g_test_add_func("/node/hash", test_node_hash);
    g_test_add_func("/node/setters", test_node_setters);
    g_test_add_func("/node/tag", test_node_tag);
    g_test_add_func("/node/anchor", test_node_anchor);
    g_test_add_func("/node/scalar_style", test_node_scalar_style);
    g_test_add_func("/node/parent", test_node_parent);
    g_test_add_func("/node/dup_string", test_node_dup_string);
    g_test_add_func("/node/type_mismatch", test_node_type_mismatch);

    /* Mapping edge cases */
    g_test_add_func("/mapping/empty", test_mapping_empty);
    g_test_add_func("/mapping/double_member", test_mapping_double_member);
    g_test_add_func("/mapping/null_member", test_mapping_null_member);
    g_test_add_func("/mapping/nested", test_mapping_nested);
    g_test_add_func("/mapping/foreach", test_mapping_foreach);
    g_test_add_func("/mapping/get_members", test_mapping_get_members);
    g_test_add_func("/mapping/seal", test_mapping_seal);
    g_test_add_func("/mapping/equal", test_mapping_equal);
    g_test_add_func("/mapping/overwrite", test_mapping_overwrite);

    /* Sequence edge cases */
    g_test_add_func("/sequence/empty", test_sequence_empty);
    g_test_add_func("/sequence/typed_getters", test_sequence_typed_getters);
    g_test_add_func("/sequence/remove", test_sequence_remove);
    g_test_add_func("/sequence/foreach", test_sequence_foreach);
    g_test_add_func("/sequence/seal", test_sequence_seal);
    g_test_add_func("/sequence/equal", test_sequence_equal);
    g_test_add_func("/sequence/nested", test_sequence_nested);

    return g_test_run();
}
