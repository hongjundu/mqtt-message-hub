#include "slist.h"
#include "ini_parser.h"
#include <stdio.h>
#include <string.h>
#include <errno.h> 
#include <stdlib.h>

struct tag_value_pair{
    struct slist_node node;
    char * szTag;
    char * szValue;
};
typedef struct tag_value_pair tag_value;

static void delete_tag_value_pair(struct tag_value_pair *node)
{
    if (node->szTag) {
        free(node->szTag);
        node->szTag = NULL;
    }
	if(node->szValue) {
		free(node->szValue);
		node->szValue = NULL;
	}
    if(node) {
		free(node);
    }
}

static void _tag_value_free(struct slist_node *node)
{
    if(node) delete_tag_value_pair((struct tag_value_pair *)node);
}

static int _tag_value_hittest(struct slist_node * node, void *key)
{
    return strcmp((char*)key, ((struct tag_value_pair*)node)->szTag);
}

static struct single_list * new_tag_value_list()
{
    return new_single_list(_tag_value_free, _tag_value_hittest);
}

static struct tag_value_pair *new_tag_value_pair()
{
    struct tag_value_pair * pair = (struct tag_value_pair *)malloc(sizeof(struct tag_value_pair));
    pair->node.next = 0;
    pair->szTag = 0;
    pair->szValue = 0;
    return pair;
}

static struct tag_value_pair * make_tag_value_pair(char * tag, char * value)
{
    struct tag_value_pair *pair = 0;
    if(!tag || !value)return 0;

    pair = (struct tag_value_pair*)malloc(sizeof(struct tag_value_pair));
    pair->szTag = strdup(tag);
    pair->szValue = strdup(value);
    pair->node.next = 0;
    return pair;
}


static struct tag_value_pair * parse_line(char *line, int len)
{
    struct tag_value_pair * pair = 0;
    int count = 0;
    char * p = line;
    char * end = 0;
    char * start = line;
    if(!p) return 0;
    while(*p == ' ') ++p;


    /*blank line*/
    if(p - line == len ||
            *p == '\r' ||
            *p == '\n' ||
            *p == '\0') return 0;

    /*do not support group*/
    if(*p == '[') return 0;
    /*comments*/
    if(*p == '#') return 0;

    /* extract key */
    start = p;
    end = line + len;
    while(*p != '=' && p!= end) ++p;
    if(p == end)
    {
        /* none '=' , invalid line */
        return 0;
    }
    end = p - 1;
    while(*end == ' ') --end; /* skip blank at the end */
    count = end - start + 1;

    pair = new_tag_value_pair();
    pair->szTag = (char *)malloc(count + 1);
    strncpy(pair->szTag, start, count);
    pair->szTag[count] = 0;

    /* extract value */
    ++p;
    end = line + len; /* next pos of the last char */
    while( *p == ' ' && p != end) ++p;
    if(p == end)
    {
        delete_tag_value_pair(pair);
        return 0;
    }
    start = p;
    --end; /* to the last char */
    if(*end == '\n') { *end = 0; --end; }
    if(*end == '\r') { *end = 0; --end; }
    count = end - start + 1;
    if(count > 0)
    {
        pair->szValue = (char *)malloc(count + 1);
        strncpy(pair->szValue, start, count);
        pair->szValue[count] = 0;
    }

    /* release empty key-value pair */
    if(!pair->szValue)
    {
        delete_tag_value_pair(pair);
        return 0;
    }

    return pair;
}

static int _parse_file(struct ini_parser * ini, const char *file){
	ini->keyvalues->clear(ini->keyvalues);
    FILE * fp = fopen(file, "r");
    if(fp)
    {
        struct tag_value_pair * pair = 0;
        char buf[1024] = {0};
        while(fgets(buf, 1024, fp))
        {
            pair = parse_line(buf, strlen(buf));
            if(pair)
            {
                ini->keyvalues->add(ini->keyvalues, (struct slist_node *)pair);
            }
        }
        fclose(fp);
        return ini->keyvalues->size;
    }
    return -1;
}

static int _parse_text(struct ini_parser * ini, const char * text){
    const char *p = text;
    const char * start = 0;
    struct tag_value_pair * pair = 0;
    if(!text) return -1;

    while(1)
    {
        start = p;
        while(*p != '\n' && *p != '\0' )++p;
        if(*p == '\0') break;

        pair = parse_line((char *)start, p - start);
        if(pair) ini->keyvalues->add(ini->keyvalues, (struct slist_node *)pair);

        ++p;
    }

    return ini->keyvalues->size;
}

static char * _value(struct ini_parser * ini, const char * key){
    struct tag_value_pair * pair = NODE_T(ini->keyvalues->find_by_key(ini->keyvalues, (void *)key), struct tag_value_pair);
    if(pair) return pair->szValue;
    return 0;
}

static void _set_value(struct ini_parser * ini, const char * key, const char *value){
    struct tag_value_pair * pair = NODE_T(ini->keyvalues->find_by_key(ini->keyvalues, (void *)key), struct tag_value_pair);
    if(pair)
    {
        if(pair->szValue) free(pair->szValue);
        pair->szValue = strdup(value);
    }
    else
    {
        ini->keyvalues->add(ini->keyvalues, (struct slist_node *)make_tag_value_pair((char *)key, (char *)value));
    }
}

static void _remove(struct ini_parser * ini, const char * key){
    struct tag_value_pair * pair = NODE_T(ini->keyvalues->find_by_key(ini->keyvalues, (char *)key), struct tag_value_pair);
    if(pair)ini->keyvalues->remove(ini->keyvalues, (struct slist_node *)pair);
}

static void write_keyvalue(struct tag_value_pair * pair, FILE *fp)
{
    fputs(pair->szTag, fp);
    fputc('=', fp);
    fputs(pair->szValue, fp);
    fputc('\n', fp);
}

static int _save_to_file(struct ini_parser * ini, const char * file){
    if(ini->keyvalues->size > 0)
    {
        FILE * fp = fopen(file, "w");
        if(fp)
        {
            struct tag_value_pair * pair = NODE_T(ini->keyvalues->head,struct tag_value_pair);
            while(pair != 0)
            {
                write_keyvalue(pair, fp);
                pair = NODE_T(pair->node.next, struct tag_value_pair);
            }

            fclose(fp);
            return 0;
        }
    }
    return -1;
}

static void _delete_ini_parser(struct ini_parser *ini){
    if(ini)
    {
        ini->keyvalues->deletor(ini->keyvalues);
        free(ini);
    }
}

struct ini_parser * new_ini_parser(){
    struct ini_parser * ini = (struct ini_parser*)malloc(sizeof(struct ini_parser));
    ini->keyvalues = new_tag_value_list();
    ini->parse_file = _parse_file;
    ini->parse_string = _parse_text;
    ini->value = _value;
    ini->set_value = _set_value;
    ini->remove = _remove;
    ini->save_to_file = _save_to_file;
    ini->deletor = _delete_ini_parser;
    return ini;
}