/* For each field there is a field "class" which implements the interface to
 * the hardware. */

struct put_table_writer;
struct connection_result;

struct class;


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Class field access. */

/* Reads value from associated register, or error if reading not supported. */
error__t class_read(
    struct class *class, unsigned int number, uint32_t *value, bool refresh);

/* Writes value to register, or returns error if not supported. */
error__t class_write(struct class *class, unsigned int number, uint32_t value);


/* This method will only be called if there is no type support for the class. */
error__t class_get(
    struct class *class, unsigned int number,
    const struct connection_result *result);

/* Direct implementation of table support. */
error__t class_put_table(
    struct class *class, unsigned int number,
    bool append, struct put_table_writer *writer);


/* Allocates and returns a fresh change index. */
uint64_t get_change_index(void);

/* Retrieves change set for the given class. */
void get_class_change_set(
    struct class *class, const uint64_t report_index[], bool changes[]);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Class inititialisation. */

/* Performs class initialisation and creates any associated type. */
error__t create_class(
    const char *class_name, const char **line,
    unsigned int block_base, unsigned int count,
    struct class **class, struct type **type);

/* Parse register definition line. */
error__t class_parse_register(
    struct class *class, const char *block_name, const char *field_name,
    const char **line);

/* To be called after database loading is complete to ensure that all classes
 * have their required register assignments. */
error__t validate_class(struct class *class);

/* Returns name of class. */
const char *get_class_name(struct class *class);

/* This should be called during shutdown for each created class. */
void destroy_class(struct class *class);
