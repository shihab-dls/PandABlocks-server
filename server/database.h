/* Interface to database of record definitions. */




/* This function loads the three configuration databases into memory. */
error__t load_config_databases(
    const char *config_db, const char *types_db, const char *register_db);
