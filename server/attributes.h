/* Attribute definitions.
 *
 * The definitions of attributes are more exposed than for other entites because
 * their implementation is shared between classes and types. */

struct class;
struct attr;
struct connection_result;


struct attr_methods {
    /* Name of this attribute. */
    const char *name;
    /* Set if this attribute contributes to the ATTR change set. */
    bool in_change_set;

    error__t (*format)(
        struct class *class, void *data, unsigned int number,
        char result[], size_t length);

    /* Reads attribute value.  Only need to implement this for multi-line
     * results, otherwise just implement format. */
    error__t (*get_many)(
        struct class *class, void *data, unsigned int number,
        struct connection_result *result);

    /* Writes attribute value. */
    error__t (*put)(
        struct class *class, void *data, unsigned int number,
        const char *value);
};


/* Retrieves current value of attribute:  block<n>.field.attr?  */
error__t attr_get(
    struct attr *attr, unsigned int number,
    struct connection_result *result);

/* Calls attr format method. */
error__t attr_format(
    struct attr *attr, unsigned int number, char result[], size_t length);


/* Writes value to attribute:  block<n>.field.attr=value  */
error__t attr_put(struct attr *attr, unsigned int number, const char *value);

/* Retrieves change set for attribute. */
void get_attr_change_set(
    struct attr *attr, uint64_t report_index, bool change_set[]);

/* Name of attribute. */
const char *get_attr_name(struct attr *attr);


/* This function creates an attribute with the given class and type pointers and
 * inserts it into the given attr_map.  If an attribute with the same name is
 * already present it is silently replaced. */
void create_attribute(
    const struct attr_methods *methods,
    struct class *class, void *data, unsigned int count,
    struct hash_table *attr_map);

/* Thus function walks the given map of attributes and deletes all attributes.
 * The map should be deleted after this. */
void delete_attributes(struct hash_table *attr_map);
