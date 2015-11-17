/* Multiplexer lookup.
 *
 * Functions for converting between multiplexer indexes and field names. */

struct mux_lookup;


/* Adds a set of entries to the given mux lookup table. */
error__t add_mux_indices(
    struct mux_lookup *lookup,
    const char *block_name, const char *field_name, unsigned int count,
    unsigned int indices[]);


/* Converts MUX name into hardware index to write to register. */
error__t mux_lookup_name(
    struct mux_lookup *lookup, const char *name, unsigned int *ix);

/* Converts hardware index to MUX name or returns error, returns name in given
 * buffer. */
error__t mux_lookup_index(
    struct mux_lookup *lookup, unsigned int ix, char result[], size_t length);


extern struct mux_lookup *bit_mux_lookup;   // bit_{in,out} fields
extern struct mux_lookup *pos_mux_lookup;   // pos_{in,out} fields

error__t initialise_mux_lookup(void);

void terminate_mux_lookup(void);
