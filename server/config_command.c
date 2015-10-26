/* Interface for configuration commands. */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "error.h"
#include "hashtable.h"
#include "database.h"
#include "parse.h"

#include "config_command.h"


struct multiline_state {
    config_command_get_more_t *get_more;
    int index;
};



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/* This structure is used to hold the context require to complete an entity
 * operation. */
struct entity_context {
    /* Fields derived from parsing the entity target. */
    const struct config_block *block;       // Block database entry
    unsigned int number;                    // Block number, within valid range
    const struct field_entry *field;        // Field database entry
    const struct field_subfield *subfield;  // Subfield data, may be absent

    struct config_connection *connection;   // Connection from request
    const struct entity_actions *actions;   // Actions for this context
};


/* This structure contains the implementations of the various entity operations,
 * depending on the exact structure of the entity request.  The fields in this
 * structure are a close reflection of the fields in config_command set. */
struct entity_actions {
    error__t (*put)(
        struct entity_context *context,
        const char *value, command_error_t *command_error);
    void (*get)(
        struct entity_context *context,
        char result[], command_error_t *command_error, void **multiline);
};


/* Implements  block.*?  command, returns list of fields. */
static void block_meta_get(
    struct entity_context *context,
    char result[], command_error_t *command_error, void **multiline)
{
    *command_error = FAIL_("block.*? not implemented yet");
}


/* Implements  block.field=  command. */
static error__t block_field_put(
    struct entity_context *context,
    const char *value, command_error_t *command_error)
{
    *command_error = FAIL_("block.field= not implemented yet");
    return ERROR_OK;
}


/* Implements  block.field?  command. */
static void block_field_get(
    struct entity_context *context,
    char result[], command_error_t *command_error, void **multiline)
{
    *command_error = FAIL_("block.field? not implemented yet");
}


/* Implements  block.field.*?  command. */
static void field_meta_get(
    struct entity_context *context,
    char result[], command_error_t *command_error, void **multiline)
{
    *command_error = FAIL_("block.field.*? not implemented yet");
}


/* Implements  block.field.subfield=  command. */
static error__t subfield_put(
    struct entity_context *context,
    const char *value, command_error_t *command_error)
{
    *command_error = FAIL_("block.field.subfield= not implemented yet");
    return ERROR_OK;
}


/* Implements  block.field.subfield?  command. */
static void subfield_get(
    struct entity_context *context,
    char result[], command_error_t *command_error, void **multiline)
{
    *command_error = FAIL_("block.field.subfield? not implemented yet");
}


/* Implements  block.*  commands. */
static const struct entity_actions block_meta_actions = {
    .get = block_meta_get,          // block.*?
};

/* Implements  block.field  commands. */
static const struct entity_actions block_field_actions = {
    .put = block_field_put,         // block.field=value
    .get = block_field_get,         // block.field?
};

/* Implements  block.field.*  commands. */
static const struct entity_actions field_meta_actions = {
    .get = field_meta_get,          // block.field.*?
};

/* Implements  block.field.subfield  commands. */
static const struct entity_actions subfield_actions = {
    .put = subfield_put,            // block.field.subfield=value
    .get = subfield_get,            // block.field.subfield?
};



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Parsing and dispatch of entity set and get. */


/* Parses first part of entity target name:
 *
 *  block [number] "."
 *
 * and sets *number_present accordingly. */
static command_error_t parse_block_name(
    const char **input, struct entity_context *context,
    unsigned int *max_number, bool *number_present)
{
    char block[MAX_NAME_LENGTH];
    return
        /* Parse and look up the block name. */
        parse_name(input, block, MAX_NAME_LENGTH)  ?:
        TEST_OK_(context->block = lookup_block(block, max_number),
            "No such block")  ?:

        /* Parse the number or flag its absence, and if present check that it's
         * in valid range. */
        IF_ELSE(isdigit(**input),
            DO(*number_present = true)  ?:
            parse_uint(input, &context->number)  ?:
            TEST_OK_(context->number < *max_number, "Block number too large"),
        //else
            DO(*number_present = false; context->number = 0))  ?:

        /* Finally eat the trailing . */
        parse_char(input, '.');
}


/* Block number must be present or defaulted for normal field and subfield
 * commands. */
static command_error_t check_block_number(
    unsigned int max_number, bool number_present)
{
    return TEST_OK_(number_present || max_number == 1, "Missing block number");
}


/* For meta-data queries we don't allow the number to be present. */
static command_error_t check_no_number(bool number_present)
{
    return TEST_OK_(!number_present, "Block number not allowed");
}


/* This parses the  field  part of the entity name and does the final number
 * checking. */
static command_error_t parse_field_name(
    const char **input, struct entity_context *context)
{
    char field[MAX_NAME_LENGTH];
    return
        /* Process the field. */
        parse_name(input, field, MAX_NAME_LENGTH)  ?:
        TEST_OK_(context->field = lookup_field(context->block, field),
            "No such field");
}


/* Parses  subfield  part of entity name if present and looks it up. */
static command_error_t parse_subfield_name(
    const char **input, struct entity_context *context)
{
    char subfield[MAX_NAME_LENGTH];
    return
        parse_name(input, subfield, MAX_NAME_LENGTH)  ?:
        TEST_OK_(context->subfield = lookup_subfield(context->field, subfield),
            "No such subfield");
}


/* Parses name into two or three sub-fields separated by dots, according to the
 * following syntax:
 *
 *  block [number] "." ( "*" | field [ "." ( "*" | meta ) ]
 *
 * One of the following four commands is parsed and the context->actions field
 * is set accordingly:
 *
 *  block.*                             block_meta_actions
 *  block[number].field                 block_field_actions
 *  block.field.*                       field_meta_actions
 *  block[number].field.subfield        subfield_actions
 *
 * The number is only optional if there is only one instance of the block. */
static command_error_t compute_entity_handler(
    const char *input, struct entity_context *context)
{
    unsigned int max_number;
    bool number_present;
    return
        /* Consume the  block.  part of the input. */
        parse_block_name(&input, context, &max_number, &number_present)  ?:

        IF_ELSE(read_char(&input, '*'),
            /*  block.*  */
            check_no_number(number_present)  ?:
            DO(context->actions = &block_meta_actions),

        //else
            /* If not block meta-query then field name must follow. */
            parse_field_name(&input, context)  ?:
            IF_ELSE(read_char(&input, '.'),
                /* There's a further * or a subfield. */
                IF_ELSE(read_char(&input, '*'),
                    /*  block.field.*  */
                    check_no_number(number_present)  ?:
                    DO(context->actions = &field_meta_actions),
                //else
                    /*  block.field.subfield  */
                    parse_subfield_name(&input, context)  ?:
                    check_block_number(max_number, number_present)  ?:
                    DO(context->actions = &subfield_actions)
                ),
            //else
                /*  block.field  */
                check_block_number(max_number, number_present)  ?:
                DO(context->actions = &block_field_actions)
            )
        )  ?:

        /* Make sure there's nothing left over in the field. */
        parse_eos(&input);
}


/* Process  entity=value  commands. */
static error__t process_entity_put(
    struct config_connection *connection,
    const char *name, const char *value, command_error_t *command_error)
{
    struct entity_context context = {};
    *command_error =
        compute_entity_handler(name, &context)  ?:
        TEST_OK_(context.actions->put, "Field not writeable");
    return IF(!*command_error,
        context.actions->put(&context, value, command_error));
}


/* Process  entity?  commands. */
static void process_entity_get(
    struct config_connection *connection, const char *name,
    char result[], command_error_t *command_error, void **multiline)
{
    struct entity_context context;
    *command_error =
        compute_entity_handler(name, &context)  ?:
        TEST_OK_(context.actions->get, "Field not readable");
    if (!*command_error)
        context.actions->get(&context, result, command_error, multiline);
}


/* Handler for multi-line responses. */
static bool process_entity_get_more(void *multiline, char result[])
{
    ASSERT_FAIL();
}


const struct config_command_set entity_commands = {
    .put = process_entity_put,
    .get = process_entity_get,
    .get_more = process_entity_get_more,
};



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System commands. */


/* *IDN?
 *
 * Returns simple system identification. */

static void system_get_idn(
    struct config_connection *connection, const char *name,
    char result[], command_error_t *command_error, void **multiline)
{
    strcpy(result, "PandA");
}


/* *BLOCKS?
 *
 * Returns formatted list of all the blocks in the system. */

static bool system_get_more_blocks(void *multiline, char result[])
{
    struct multiline_state *state = multiline;

    const struct config_block *block;
    const char *name;
    unsigned int count;
    if (walk_blocks_list(&state->index, &block, &name, &count))
    {
        snprintf(result, MAX_VALUE_LENGTH, "%s %d", name, count);
        return true;
    }
    else
    {
        free(state);
        return false;
    }
}

static void system_get_blocks(
    struct config_connection *connection, const char *name,
    char result[], command_error_t *command_error, void **multiline)
{
    struct multiline_state *state = malloc(sizeof(struct multiline_state));
    *state = (struct multiline_state) {
        .get_more = system_get_more_blocks,
        .index = 0, };

    if (system_get_more_blocks(state, result))
        *multiline = state;
    else
        *command_error = FAIL_("No blocks found!");
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System commands dispatch. */


struct command_table_entry {
    const char *name;
    struct config_command_set command;
};


static const struct command_table_entry command_table_list[] = {
    { "IDN", { .get = system_get_idn } },
    { "BLOCKS", {
        .get = system_get_blocks, .get_more = system_get_more_blocks } },
};

static struct hash_table *command_table;



/* Process  *command=value  commands. */
static error__t process_system_put(
    struct config_connection *connection,
    const char *name, const char *value, command_error_t *command_error)
{
    const struct config_command_set *commands =
        hash_table_lookup(command_table, name);
    if (commands  &&  commands->put)
        return commands->put(connection, name, value, command_error);
    else
    {
        *command_error = FAIL_("Unknown target");
        return ERROR_OK;
    }
}


/* Process  *command?  commands. */
static void process_system_get(
    struct config_connection *connection, const char *name,
    char result[], command_error_t *command_error, void **multiline)
{
    const struct config_command_set *commands =
        hash_table_lookup(command_table, name);
    if (commands  &&  commands->get)
        commands->get(connection, name, result, command_error, multiline);
    else
        *command_error = FAIL_("Unknown value");
}


/* Processes multi-line  *command?  responses. */
static bool process_system_get_more(void *multiline, char result[])
{
    struct multiline_state *state = multiline;
    return state->get_more(multiline, result);
}


const struct config_command_set system_commands = {
    .put = process_system_put,
    .get = process_system_get,
    .get_more = process_system_get_more,
};



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

error__t initialise_config_command(void)
{
    /* Don't need to copy the keys, they're in the command table! */
    command_table = hash_table_create(false);
    for (unsigned int i = 0; i < ARRAY_SIZE(command_table_list); i ++)
    {
        const struct command_table_entry *entry = &command_table_list[i];
        hash_table_insert_const(command_table, entry->name, &entry->command);
    }
    return ERROR_OK;
}