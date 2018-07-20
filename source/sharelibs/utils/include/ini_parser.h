struct single_list;

struct ini_parser {
    struct single_list * keyvalues;
    int (*parse_file)(struct ini_parser *, const char * file);
    int (*parse_string)(struct ini_parser *, const char *text);
    char * (*value)(struct ini_parser *, const char * key);
    void (*set_value)(struct ini_parser *, const char * key, const char * value);
    void (*remove)(struct ini_parser *, const char *key);
    int (*save_to_file)(struct ini_parser *, const char * file);
    void (*deletor)(struct ini_parser *ini);
};

struct ini_parser * new_ini_parser();