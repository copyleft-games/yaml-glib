/* test-gobject.c
 *
 * Copyright 2025 Zach Podbielniak
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Unit tests for yaml-gobject.h and yaml-serializable.h APIs.
 */

#include <glib.h>
#include <glib-object.h>
#include "yaml-glib.h"

/* ── TestPerson - simple GObject for serialization tests ────────── */

#define TEST_TYPE_PERSON (test_person_get_type())
G_DECLARE_FINAL_TYPE(TestPerson, test_person, TEST, PERSON, GObject)

struct _TestPerson
{
    GObject parent_instance;
    gchar *name;
    gint age;
    gdouble height;
    gboolean active;
};

G_DEFINE_TYPE(TestPerson, test_person, G_TYPE_OBJECT)

enum
{
    PROP_0,
    PROP_NAME,
    PROP_AGE,
    PROP_HEIGHT,
    PROP_ACTIVE,
    N_PROPERTIES
};

static GParamSpec *person_props[N_PROPERTIES] = { NULL, };

static void
test_person_finalize(GObject *object)
{
    TestPerson *self = TEST_PERSON(object);

    g_clear_pointer(&self->name, g_free);

    G_OBJECT_CLASS(test_person_parent_class)->finalize(object);
}

static void
test_person_set_property(
    GObject      *object,
    guint         prop_id,
    const GValue *value,
    GParamSpec   *pspec
)
{
    TestPerson *self = TEST_PERSON(object);

    switch (prop_id)
    {
    case PROP_NAME:
        g_free(self->name);
        self->name = g_value_dup_string(value);
        break;
    case PROP_AGE:
        self->age = g_value_get_int(value);
        break;
    case PROP_HEIGHT:
        self->height = g_value_get_double(value);
        break;
    case PROP_ACTIVE:
        self->active = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
test_person_get_property(
    GObject    *object,
    guint       prop_id,
    GValue     *value,
    GParamSpec *pspec
)
{
    TestPerson *self = TEST_PERSON(object);

    switch (prop_id)
    {
    case PROP_NAME:
        g_value_set_string(value, self->name);
        break;
    case PROP_AGE:
        g_value_set_int(value, self->age);
        break;
    case PROP_HEIGHT:
        g_value_set_double(value, self->height);
        break;
    case PROP_ACTIVE:
        g_value_set_boolean(value, self->active);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
test_person_class_init(TestPersonClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = test_person_finalize;
    object_class->set_property = test_person_set_property;
    object_class->get_property = test_person_get_property;

    person_props[PROP_NAME] = g_param_spec_string(
        "name", "Name", "The person's name",
        NULL,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
    );

    person_props[PROP_AGE] = g_param_spec_int(
        "age", "Age", "The person's age",
        0, G_MAXINT, 0,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
    );

    person_props[PROP_HEIGHT] = g_param_spec_double(
        "height", "Height", "The person's height in meters",
        0.0, 3.0, 0.0,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
    );

    person_props[PROP_ACTIVE] = g_param_spec_boolean(
        "active", "Active", "Whether the person is active",
        FALSE,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
    );

    g_object_class_install_properties(object_class, N_PROPERTIES, person_props);
}

static void
test_person_init(TestPerson *self)
{
    self->name = NULL;
    self->age = 0;
    self->height = 0.0;
    self->active = FALSE;
}

/*
 * test_person_new:
 * @name: the person's name
 * @age: the person's age
 * @height: the person's height
 * @active: whether the person is active
 *
 * Creates a new TestPerson with the given properties.
 *
 * Returns: (transfer full): a new TestPerson
 */
static TestPerson *
test_person_new(
    const gchar *name,
    gint         age,
    gdouble      height,
    gboolean     active
)
{
    return g_object_new(
        TEST_TYPE_PERSON,
        "name", name,
        "age", age,
        "height", height,
        "active", active,
        NULL
    );
}

/* ── Test: yaml_gobject_serialize ───────────────────────────────── */

static void
test_gobject_serialize(void)
{
    g_autoptr(TestPerson) person = NULL;
    g_autoptr(YamlNode) node = NULL;
    YamlMapping *mapping;

    person = test_person_new("Alice", 30, 1.72, TRUE);
    node = yaml_gobject_serialize(G_OBJECT(person));

    g_assert_nonnull(node);
    g_assert_cmpint(yaml_node_get_node_type(node), ==, YAML_NODE_MAPPING);

    mapping = yaml_node_get_mapping(node);
    g_assert_nonnull(mapping);

    /* Verify all four properties are present with correct values */
    g_assert_true(yaml_mapping_has_member(mapping, "name"));
    g_assert_cmpstr(yaml_mapping_get_string_member(mapping, "name"), ==, "Alice");

    g_assert_true(yaml_mapping_has_member(mapping, "age"));
    g_assert_cmpint(yaml_mapping_get_int_member(mapping, "age"), ==, 30);

    g_assert_true(yaml_mapping_has_member(mapping, "height"));
    g_assert_cmpfloat_with_epsilon(
        yaml_mapping_get_double_member(mapping, "height"), 1.72, 0.001
    );

    g_assert_true(yaml_mapping_has_member(mapping, "active"));
    g_assert_true(yaml_mapping_get_boolean_member(mapping, "active"));
}

/* ── Test: yaml_gobject_deserialize ─────────────────────────────── */

static void
test_gobject_deserialize(void)
{
    g_autoptr(YamlBuilder) builder = NULL;
    g_autoptr(YamlNode) node = NULL;
    GObject *obj;
    TestPerson *person;

    /* Build a mapping node representing a person */
    builder = yaml_builder_new();
    yaml_builder_begin_mapping(builder);

    yaml_builder_set_member_name(builder, "name");
    yaml_builder_add_string_value(builder, "Bob");

    yaml_builder_set_member_name(builder, "age");
    yaml_builder_add_int_value(builder, 25);

    yaml_builder_set_member_name(builder, "height");
    yaml_builder_add_double_value(builder, 1.85);

    yaml_builder_set_member_name(builder, "active");
    yaml_builder_add_boolean_value(builder, FALSE);

    yaml_builder_end_mapping(builder);

    node = yaml_builder_steal_root(builder);
    g_assert_nonnull(node);

    /* Deserialize into a TestPerson */
    obj = yaml_gobject_deserialize(TEST_TYPE_PERSON, node);
    g_assert_nonnull(obj);
    g_assert_true(TEST_IS_PERSON(obj));

    person = TEST_PERSON(obj);
    g_assert_cmpstr(person->name, ==, "Bob");
    g_assert_cmpint(person->age, ==, 25);
    g_assert_cmpfloat_with_epsilon(person->height, 1.85, 0.001);
    g_assert_false(person->active);

    g_object_unref(obj);
}

/* ── Test: yaml_gobject_from_data ───────────────────────────────── */

static void
test_gobject_from_data(void)
{
    const gchar *yaml_str =
        "name: Charlie\n"
        "age: 40\n"
        "height: 1.68\n"
        "active: true\n";
    g_autoptr(GError) error = NULL;
    GObject *obj;
    TestPerson *person;

    obj = yaml_gobject_from_data(TEST_TYPE_PERSON, yaml_str, -1, &error);
    g_assert_no_error(error);
    g_assert_nonnull(obj);
    g_assert_true(TEST_IS_PERSON(obj));

    person = TEST_PERSON(obj);
    g_assert_cmpstr(person->name, ==, "Charlie");
    g_assert_cmpint(person->age, ==, 40);
    g_assert_cmpfloat_with_epsilon(person->height, 1.68, 0.001);
    g_assert_true(person->active);

    g_object_unref(obj);
}

/* ── Test: yaml_gobject_to_data ─────────────────────────────────── */

static void
test_gobject_to_data(void)
{
    g_autoptr(TestPerson) person = NULL;
    g_autofree gchar *data = NULL;
    gsize length = 0;

    person = test_person_new("Diana", 55, 1.60, FALSE);

    data = yaml_gobject_to_data(G_OBJECT(person), &length);
    g_assert_nonnull(data);
    g_assert_cmpuint(length, >, 0);

    /* Verify the output contains expected property values */
    g_assert_nonnull(g_strstr_len(data, -1, "Diana"));
    g_assert_nonnull(g_strstr_len(data, -1, "55"));
    g_assert_nonnull(g_strstr_len(data, -1, "name"));
    g_assert_nonnull(g_strstr_len(data, -1, "age"));
    g_assert_nonnull(g_strstr_len(data, -1, "height"));
    g_assert_nonnull(g_strstr_len(data, -1, "active"));
}

/* ── Test: roundtrip serialize -> deserialize ───────────────────── */

static void
test_gobject_roundtrip(void)
{
    g_autoptr(TestPerson) original = NULL;
    g_autoptr(YamlNode) node = NULL;
    GObject *obj;
    TestPerson *restored;

    original = test_person_new("Eve", 28, 1.75, TRUE);

    /* Serialize to YAML node */
    node = yaml_gobject_serialize(G_OBJECT(original));
    g_assert_nonnull(node);

    /* Deserialize back into a new object */
    obj = yaml_gobject_deserialize(TEST_TYPE_PERSON, node);
    g_assert_nonnull(obj);
    g_assert_true(TEST_IS_PERSON(obj));

    restored = TEST_PERSON(obj);

    /* Verify all properties survived the roundtrip */
    g_assert_cmpstr(restored->name, ==, original->name);
    g_assert_cmpint(restored->age, ==, original->age);
    g_assert_cmpfloat_with_epsilon(restored->height, original->height, 0.001);
    g_assert_cmpint(restored->active, ==, original->active);

    g_object_unref(obj);
}

/* ── Test: deserialize with non-mapping node returns NULL ───────── */

static void
test_gobject_deserialize_non_mapping(void)
{
    g_autoptr(YamlNode) scalar_node = NULL;
    GObject *obj;

    /* A scalar node is not a mapping, deserialization should fail */
    scalar_node = yaml_node_new_string("not a mapping");

    /* Expect a warning from the library about the wrong node type */
    g_test_expect_message(
        G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
        "*expected mapping node*"
    );

    obj = yaml_gobject_deserialize(TEST_TYPE_PERSON, scalar_node);
    g_assert_null(obj);

    g_test_assert_expected_messages();
}

/* ── Test: from_data with invalid YAML sets error ───────────────── */

static void
test_gobject_from_data_invalid_yaml(void)
{
    const gchar *bad_yaml = ":\n  :\n    - [\n  invalid: {{{";
    g_autoptr(GError) error = NULL;
    GObject *obj;

    obj = yaml_gobject_from_data(TEST_TYPE_PERSON, bad_yaml, -1, &error);

    /*
     * The parser should either set an error or the node won't be a mapping.
     * Either way the object should be NULL.
     */
    g_assert_null(obj);
}

/* ── Test: from_data with empty string ──────────────────────────── */

static void
test_gobject_from_data_empty(void)
{
    g_autoptr(GError) error = NULL;
    GObject *obj;

    obj = yaml_gobject_from_data(TEST_TYPE_PERSON, "", -1, &error);

    /* Empty input should produce no root node, so result is NULL */
    g_assert_null(obj);
}

/* ── Test: default_serialize_property for fundamental types ─────── */

static void
test_serializable_default_serialize_property(void)
{
    GValue str_val = G_VALUE_INIT;
    GValue int_val = G_VALUE_INIT;
    GValue dbl_val = G_VALUE_INIT;
    GValue bool_val = G_VALUE_INIT;
    g_autoptr(YamlNode) str_node = NULL;
    g_autoptr(YamlNode) int_node = NULL;
    g_autoptr(YamlNode) dbl_node = NULL;
    g_autoptr(YamlNode) bool_node = NULL;

    /* String */
    g_value_init(&str_val, G_TYPE_STRING);
    g_value_set_string(&str_val, "hello");

    str_node = yaml_serializable_default_serialize_property(
        NULL, "test-string", &str_val, person_props[PROP_NAME]
    );
    g_assert_nonnull(str_node);
    g_assert_cmpint(yaml_node_get_node_type(str_node), ==, YAML_NODE_SCALAR);
    g_assert_cmpstr(yaml_node_get_string(str_node), ==, "hello");
    g_value_unset(&str_val);

    /* Integer */
    g_value_init(&int_val, G_TYPE_INT);
    g_value_set_int(&int_val, 42);

    int_node = yaml_serializable_default_serialize_property(
        NULL, "test-int", &int_val, person_props[PROP_AGE]
    );
    g_assert_nonnull(int_node);
    g_assert_cmpint(yaml_node_get_node_type(int_node), ==, YAML_NODE_SCALAR);
    g_assert_cmpint(yaml_node_get_int(int_node), ==, 42);
    g_value_unset(&int_val);

    /* Double */
    g_value_init(&dbl_val, G_TYPE_DOUBLE);
    g_value_set_double(&dbl_val, 3.14);

    dbl_node = yaml_serializable_default_serialize_property(
        NULL, "test-double", &dbl_val, person_props[PROP_HEIGHT]
    );
    g_assert_nonnull(dbl_node);
    g_assert_cmpint(yaml_node_get_node_type(dbl_node), ==, YAML_NODE_SCALAR);
    g_assert_cmpfloat_with_epsilon(yaml_node_get_double(dbl_node), 3.14, 0.001);
    g_value_unset(&dbl_val);

    /* Boolean */
    g_value_init(&bool_val, G_TYPE_BOOLEAN);
    g_value_set_boolean(&bool_val, TRUE);

    bool_node = yaml_serializable_default_serialize_property(
        NULL, "test-boolean", &bool_val, person_props[PROP_ACTIVE]
    );
    g_assert_nonnull(bool_node);
    g_assert_true(yaml_node_get_boolean(bool_node));
    g_value_unset(&bool_val);
}

/* ── Test: default_deserialize_property for fundamental types ───── */

static void
test_serializable_default_deserialize_property(void)
{
    g_autoptr(YamlNode) str_node = NULL;
    g_autoptr(YamlNode) int_node = NULL;
    g_autoptr(YamlNode) dbl_node = NULL;
    g_autoptr(YamlNode) bool_node = NULL;
    GValue str_val = G_VALUE_INIT;
    GValue int_val = G_VALUE_INIT;
    GValue dbl_val = G_VALUE_INIT;
    GValue bool_val = G_VALUE_INIT;
    gboolean ret;

    /* String */
    str_node = yaml_node_new_string("world");
    g_value_init(&str_val, G_TYPE_STRING);

    ret = yaml_serializable_default_deserialize_property(
        NULL, "name", &str_val, person_props[PROP_NAME], str_node
    );
    g_assert_true(ret);
    g_assert_cmpstr(g_value_get_string(&str_val), ==, "world");
    g_value_unset(&str_val);

    /* Integer */
    int_node = yaml_node_new_int(99);
    g_value_init(&int_val, G_TYPE_INT);

    ret = yaml_serializable_default_deserialize_property(
        NULL, "age", &int_val, person_props[PROP_AGE], int_node
    );
    g_assert_true(ret);
    g_assert_cmpint(g_value_get_int(&int_val), ==, 99);
    g_value_unset(&int_val);

    /* Double */
    dbl_node = yaml_node_new_double(2.718);
    g_value_init(&dbl_val, G_TYPE_DOUBLE);

    ret = yaml_serializable_default_deserialize_property(
        NULL, "height", &dbl_val, person_props[PROP_HEIGHT], dbl_node
    );
    g_assert_true(ret);
    g_assert_cmpfloat_with_epsilon(g_value_get_double(&dbl_val), 2.718, 0.001);
    g_value_unset(&dbl_val);

    /* Boolean */
    bool_node = yaml_node_new_boolean(FALSE);
    g_value_init(&bool_val, G_TYPE_BOOLEAN);

    ret = yaml_serializable_default_deserialize_property(
        NULL, "active", &bool_val, person_props[PROP_ACTIVE], bool_node
    );
    g_assert_true(ret);
    g_assert_false(g_value_get_boolean(&bool_val));
    g_value_unset(&bool_val);
}

int
main(
    int   argc,
    char *argv[]
)
{
    g_test_init(&argc, &argv, NULL);

    /* GObject serialization */
    g_test_add_func("/gobject/serialize", test_gobject_serialize);
    g_test_add_func("/gobject/deserialize", test_gobject_deserialize);
    g_test_add_func("/gobject/from_data", test_gobject_from_data);
    g_test_add_func("/gobject/to_data", test_gobject_to_data);
    g_test_add_func("/gobject/roundtrip", test_gobject_roundtrip);

    /* Error paths */
    g_test_add_func("/gobject/deserialize_non_mapping",
                     test_gobject_deserialize_non_mapping);
    g_test_add_func("/gobject/from_data_invalid_yaml",
                     test_gobject_from_data_invalid_yaml);
    g_test_add_func("/gobject/from_data_empty",
                     test_gobject_from_data_empty);

    /* Default serialization helpers */
    g_test_add_func("/serializable/default_serialize_property",
                     test_serializable_default_serialize_property);
    g_test_add_func("/serializable/default_deserialize_property",
                     test_serializable_default_deserialize_property);

    return g_test_run();
}
