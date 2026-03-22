/* test-document.c
 *
 * Copyright 2025 Zach Podbielniak
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Unit tests for YamlDocument GObject type.
 */

#include <glib.h>
#include <json-glib/json-glib.h>
#include "yaml-glib.h"

/* ── yaml_document_new ────────────────────────────────────────────── */

/* Create an empty document, verify root is NULL */
static void
test_document_new(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();

    g_assert_nonnull(doc);
    g_assert_true(YAML_IS_DOCUMENT(doc));
    g_assert_null(yaml_document_get_root(doc));
}

/* ── yaml_document_new_with_root ──────────────────────────────────── */

/* Create a document with a string node root, verify root value */
static void
test_document_new_with_root(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("hello");
    g_autoptr(YamlDocument) doc = yaml_document_new_with_root(node);
    YamlNode *root;

    g_assert_nonnull(doc);

    root = yaml_document_get_root(doc);
    g_assert_nonnull(root);
    g_assert_cmpint(yaml_node_get_node_type(root), ==, YAML_NODE_SCALAR);
    g_assert_cmpstr(yaml_node_get_scalar(root), ==, "hello");
}

/* ── yaml_document_set_root / get_root ────────────────────────────── */

/* Set a root node and retrieve it */
static void
test_document_set_get_root(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();
    g_autoptr(YamlNode) node = yaml_node_new_int(42);
    YamlNode *root;

    g_assert_null(yaml_document_get_root(doc));

    yaml_document_set_root(doc, node);
    root = yaml_document_get_root(doc);

    g_assert_nonnull(root);
    g_assert_true(root == node);
    g_assert_cmpint(yaml_node_get_int(root), ==, 42);
}

/* Set root to NULL to clear it */
static void
test_document_set_root_null(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("value");
    g_autoptr(YamlDocument) doc = yaml_document_new_with_root(node);

    g_assert_nonnull(yaml_document_get_root(doc));

    yaml_document_set_root(doc, NULL);
    g_assert_null(yaml_document_get_root(doc));
}

/* Replace root by calling set_root twice with different nodes */
static void
test_document_replace_root(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();
    g_autoptr(YamlNode) first = yaml_node_new_string("first");
    g_autoptr(YamlNode) second = yaml_node_new_string("second");

    yaml_document_set_root(doc, first);
    g_assert_cmpstr(yaml_node_get_scalar(yaml_document_get_root(doc)), ==, "first");

    yaml_document_set_root(doc, second);
    g_assert_cmpstr(yaml_node_get_scalar(yaml_document_get_root(doc)), ==, "second");
}

/* ── yaml_document_dup_root ───────────────────────────────────────── */

/* dup_root returns a new reference to the same node */
static void
test_document_dup_root(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("duped");
    g_autoptr(YamlDocument) doc = yaml_document_new_with_root(node);
    YamlNode *duped;

    duped = yaml_document_dup_root(doc);
    g_assert_nonnull(duped);
    g_assert_true(duped == node);
    g_assert_cmpstr(yaml_node_get_scalar(duped), ==, "duped");

    /* Caller owns the returned reference, must unref */
    yaml_node_unref(duped);
}

/* dup_root on an empty document returns NULL */
static void
test_document_dup_root_empty(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();
    YamlNode *duped;

    duped = yaml_document_dup_root(doc);
    g_assert_null(duped);
}

/* ── yaml_document_steal_root ─────────────────────────────────────── */

/* Steal root transfers ownership; document root becomes NULL */
static void
test_document_steal_root(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("stolen");
    g_autoptr(YamlDocument) doc = yaml_document_new_with_root(node);
    YamlNode *stolen;

    stolen = yaml_document_steal_root(doc);
    g_assert_nonnull(stolen);
    g_assert_cmpstr(yaml_node_get_scalar(stolen), ==, "stolen");

    /* Document should now have NULL root */
    g_assert_null(yaml_document_get_root(doc));

    /* Caller owns the stolen reference */
    yaml_node_unref(stolen);
}

/* Steal root from an empty document returns NULL */
static void
test_document_steal_root_empty(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();
    YamlNode *stolen;

    stolen = yaml_document_steal_root(doc);
    g_assert_null(stolen);
}

/* ── yaml_document_seal / is_immutable ────────────────────────────── */

/* Seal a document and verify it becomes immutable */
static void
test_document_seal(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("sealed");
    g_autoptr(YamlDocument) doc = yaml_document_new_with_root(node);

    g_assert_false(yaml_document_is_immutable(doc));

    yaml_document_seal(doc);

    g_assert_true(yaml_document_is_immutable(doc));
}

/* Sealing a document also seals its root node */
static void
test_document_seal_seals_root(void)
{
    g_autoptr(YamlNode) node = yaml_node_new_string("root-sealed");
    g_autoptr(YamlDocument) doc = yaml_document_new_with_root(node);

    g_assert_false(yaml_node_is_immutable(node));

    yaml_document_seal(doc);

    g_assert_true(yaml_node_is_immutable(node));
}

/* Seal an empty document (no root) should not crash */
static void
test_document_seal_empty(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();

    g_assert_false(yaml_document_is_immutable(doc));

    yaml_document_seal(doc);

    g_assert_true(yaml_document_is_immutable(doc));
}

/* Sealing an already-sealed document is a no-op */
static void
test_document_seal_idempotent(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();

    yaml_document_seal(doc);
    g_assert_true(yaml_document_is_immutable(doc));

    /* Second seal should not crash */
    yaml_document_seal(doc);
    g_assert_true(yaml_document_is_immutable(doc));
}

/* ── yaml_document_set_version / get_version ──────────────────────── */

/* Default version should be 1.2 */
static void
test_document_version_default(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();
    guint major = 0;
    guint minor = 0;

    yaml_document_get_version(doc, &major, &minor);

    g_assert_cmpuint(major, ==, 1);
    g_assert_cmpuint(minor, ==, 2);
}

/* Set version to 1.1 and read it back */
static void
test_document_set_version(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();
    guint major = 0;
    guint minor = 0;

    yaml_document_set_version(doc, 1, 1);
    yaml_document_get_version(doc, &major, &minor);

    g_assert_cmpuint(major, ==, 1);
    g_assert_cmpuint(minor, ==, 1);
}

/* get_version with NULL out-params should not crash */
static void
test_document_get_version_null_params(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();
    guint major = 0;
    guint minor = 0;

    /* Pass NULL for one or both params */
    yaml_document_get_version(doc, &major, NULL);
    g_assert_cmpuint(major, ==, 1);

    yaml_document_get_version(doc, NULL, &minor);
    g_assert_cmpuint(minor, ==, 2);

    yaml_document_get_version(doc, NULL, NULL);
}

/* ── yaml_document_add_tag_directive / get_tag_directives ─────────── */

/* Add a single tag directive and verify it in the hash table */
static void
test_document_tag_directive_single(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();
    GHashTable *tags;
    const gchar *prefix;

    yaml_document_add_tag_directive(doc, "!e!", "tag:example.com,2024:");

    tags = yaml_document_get_tag_directives(doc);
    g_assert_nonnull(tags);
    g_assert_cmpuint(g_hash_table_size(tags), ==, 1);

    prefix = g_hash_table_lookup(tags, "!e!");
    g_assert_cmpstr(prefix, ==, "tag:example.com,2024:");
}

/* Add multiple tag directives */
static void
test_document_tag_directive_multiple(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();
    GHashTable *tags;

    yaml_document_add_tag_directive(doc, "!e!", "tag:example.com,2024:");
    yaml_document_add_tag_directive(doc, "!yaml!", "tag:yaml.org,2002:");
    yaml_document_add_tag_directive(doc, "!local!", "tag:local,2025:");

    tags = yaml_document_get_tag_directives(doc);
    g_assert_cmpuint(g_hash_table_size(tags), ==, 3);

    g_assert_cmpstr(g_hash_table_lookup(tags, "!e!"), ==, "tag:example.com,2024:");
    g_assert_cmpstr(g_hash_table_lookup(tags, "!yaml!"), ==, "tag:yaml.org,2002:");
    g_assert_cmpstr(g_hash_table_lookup(tags, "!local!"), ==, "tag:local,2025:");
}

/* Overwrite an existing tag directive with the same handle */
static void
test_document_tag_directive_overwrite(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();
    GHashTable *tags;

    yaml_document_add_tag_directive(doc, "!e!", "tag:old.com,2024:");
    yaml_document_add_tag_directive(doc, "!e!", "tag:new.com,2025:");

    tags = yaml_document_get_tag_directives(doc);
    g_assert_cmpuint(g_hash_table_size(tags), ==, 1);
    g_assert_cmpstr(g_hash_table_lookup(tags, "!e!"), ==, "tag:new.com,2025:");
}

/* Empty document should have an empty (but non-NULL) tag table */
static void
test_document_tag_directive_empty(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();
    GHashTable *tags;

    tags = yaml_document_get_tag_directives(doc);
    g_assert_nonnull(tags);
    g_assert_cmpuint(g_hash_table_size(tags), ==, 0);
}

/* ── yaml_document_from_json_node ─────────────────────────────────── */

/* Convert a JSON object to a YAML document */
static void
test_document_from_json_node(void)
{
    g_autoptr(JsonBuilder) jb = json_builder_new();
    g_autoptr(JsonNode) json_root = NULL;
    g_autoptr(YamlDocument) doc = NULL;
    YamlNode *root;
    YamlMapping *mapping;

    /* Build a JSON object: {"name": "test", "count": 42} */
    json_builder_begin_object(jb);
    json_builder_set_member_name(jb, "name");
    json_builder_add_string_value(jb, "test");
    json_builder_set_member_name(jb, "count");
    json_builder_add_int_value(jb, 42);
    json_builder_end_object(jb);

    json_root = json_builder_get_root(jb);
    g_assert_nonnull(json_root);

    doc = yaml_document_from_json_node(json_root);
    g_assert_nonnull(doc);

    root = yaml_document_get_root(doc);
    g_assert_nonnull(root);
    g_assert_cmpint(yaml_node_get_node_type(root), ==, YAML_NODE_MAPPING);

    mapping = yaml_node_get_mapping(root);
    g_assert_nonnull(mapping);
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "name"), ==, "test");
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "count"), ==, 42);
}

/* ── yaml_document_to_json_node ───────────────────────────────────── */

/* Convert a YAML document with a mapping root to JSON */
static void
test_document_to_json_node(void)
{
    g_autoptr(YamlMapping) mapping = yaml_mapping_new();
    g_autoptr(YamlNode) node = NULL;
    g_autoptr(YamlDocument) doc = NULL;
    g_autoptr(JsonNode) json_root = NULL;
    JsonObject *json_obj;

    yaml_mapping_set_string_member(mapping, "key", "value");
    yaml_mapping_set_int_member(mapping, "num", 99);

    node = yaml_node_new_mapping(mapping);
    doc = yaml_document_new_with_root(node);

    json_root = yaml_document_to_json_node(doc);
    g_assert_nonnull(json_root);
    g_assert_cmpint(json_node_get_node_type(json_root), ==, JSON_NODE_OBJECT);

    json_obj = json_node_get_object(json_root);
    g_assert_nonnull(json_obj);
    g_assert_cmpstr(json_object_get_string_member(json_obj, "key"), ==, "value");
    g_assert_cmpint(json_object_get_int_member(json_obj, "num"), ==, 99);
}

/* Convert an empty document (NULL root) to JSON gives JSON NULL */
static void
test_document_to_json_node_null_root(void)
{
    g_autoptr(YamlDocument) doc = yaml_document_new();
    g_autoptr(JsonNode) json_root = NULL;

    json_root = yaml_document_to_json_node(doc);
    g_assert_nonnull(json_root);
    g_assert_cmpint(json_node_get_node_type(json_root), ==, JSON_NODE_NULL);
}

/* ── main ─────────────────────────────────────────────────────────── */

int
main(
    int   argc,
    char *argv[]
)
{
    g_test_init(&argc, &argv, NULL);

    /* Constructor tests */
    g_test_add_func("/document/new", test_document_new);
    g_test_add_func("/document/new_with_root", test_document_new_with_root);

    /* Root node get/set tests */
    g_test_add_func("/document/set_get_root", test_document_set_get_root);
    g_test_add_func("/document/set_root_null", test_document_set_root_null);
    g_test_add_func("/document/replace_root", test_document_replace_root);

    /* dup_root tests */
    g_test_add_func("/document/dup_root", test_document_dup_root);
    g_test_add_func("/document/dup_root_empty", test_document_dup_root_empty);

    /* steal_root tests */
    g_test_add_func("/document/steal_root", test_document_steal_root);
    g_test_add_func("/document/steal_root_empty", test_document_steal_root_empty);

    /* Seal / immutability tests */
    g_test_add_func("/document/seal", test_document_seal);
    g_test_add_func("/document/seal_seals_root", test_document_seal_seals_root);
    g_test_add_func("/document/seal_empty", test_document_seal_empty);
    g_test_add_func("/document/seal_idempotent", test_document_seal_idempotent);

    /* Version directive tests */
    g_test_add_func("/document/version_default", test_document_version_default);
    g_test_add_func("/document/set_version", test_document_set_version);
    g_test_add_func("/document/get_version_null_params", test_document_get_version_null_params);

    /* Tag directive tests */
    g_test_add_func("/document/tag_directive_single", test_document_tag_directive_single);
    g_test_add_func("/document/tag_directive_multiple", test_document_tag_directive_multiple);
    g_test_add_func("/document/tag_directive_overwrite", test_document_tag_directive_overwrite);
    g_test_add_func("/document/tag_directive_empty", test_document_tag_directive_empty);

    /* JSON interoperability tests */
    g_test_add_func("/document/from_json_node", test_document_from_json_node);
    g_test_add_func("/document/to_json_node", test_document_to_json_node);
    g_test_add_func("/document/to_json_node_null_root", test_document_to_json_node_null_root);

    return g_test_run();
}
