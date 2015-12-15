/* Definitions of methods for bit_out and pos_out classes and bit_mux and
 * pos_mux types. */

struct class;


error__t initialise_capture(void);

void terminate_capture(void);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Type definitions. */

/* Converts hardware index into a printable string. */
error__t bit_mux_format(
    void *type_data, unsigned int number,
    unsigned int value, char result[], size_t length);
error__t pos_mux_format(
    void *type_data, unsigned int number,
    unsigned int value, char result[], size_t length);

/* Converts _out field name into hardware multiplexer index. */
error__t bit_mux_parse(
    void *type_data, unsigned int number,
    const char *string, unsigned int *value);
error__t pos_mux_parse(
    void *type_data, unsigned int number,
    const char *string, unsigned int *value);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Refresh methods used to bring cached _out values up to date with values read
 * from hardware. */

void do_bit_out_refresh(uint64_t change_index);
void do_pos_out_refresh(uint64_t change_index);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Capture enumeration. */

/* *CAPTURE? implementation: returns list of all captured fields. */
void report_capture_list(struct connection_result *result);

/* Resets all capture bits. */
void reset_capture_list(void);

/* *BITSn? implementation, reports bit names in specific capture block. */
void report_capture_bits(struct connection_result *result, unsigned int group);

/* *POSITIONS? implementation, reports all position names. */
void report_capture_positions(struct connection_result *result);


/* Capture class api. */
extern const struct class_methods bit_out_class_methods;
extern const struct class_methods pos_out_class_methods;
