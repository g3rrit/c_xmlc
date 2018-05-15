#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sxmlc.h"

#define BUFSIZE 8192
#define STR_S 256

struct string_node
{
    char *val;
    struct string_node *next;
};

void string_node_append(struct string_node **entry, char *data);

void string_node_print(struct string_node *entry);

// ----- XML NODE ------

struct xml_node
{
    XMLNode *node;
    char *text;
};

void print_xml_node(struct xml_node *node)
{
    printf("node ->\ntag: %s\n", node->node->tag);

    printf("text: %s\n", node->text);

    printf("%i attributes\n", node->node->n_attributes);

    for(int i = 0; i < node->node->n_attributes; i++)
    {
        printf("att[%i] name: %s value: %s\n", i, node->node->attributes[i].name, node->node->attributes[i].value);
    }
}
// ----- XML NODE ------

// ----- COMPILE_UNIT -----
struct compile_unit
{
    struct string_node *option;

    char *compiler;
    char *target;
    int size;

    //the active text inside a node
    struct xml_node *active_node;
};

void compile_unit_init(struct compile_unit *c_unit, char *target);

void compile_unit_delete(struct compile_unit *c_unit);

void compile_unit_print(struct compile_unit *c_unit);

char *get_compile_str(struct compile_unit *c_unit);

// ----- COMPILE_UNIT -----

void parse_file(struct compile_unit *c_unit, char *path, char *pre_str);

void fill_str(char *str);

int main(int argc, char *argv[])
{
    printf("Starting XMLC\n");

    char *target;
    if(argc == 2)
        target = argv[1];
    else
        target = "default";

    printf("target: %s\n", target);

    struct compile_unit c_unit;
    compile_unit_init(&c_unit, target);

    parse_file(&c_unit, "build.xml", 0);

    compile_unit_print(&c_unit);

    if(!c_unit.option)
    {
        printf("!--no options files specified--!\n");
        goto cleanup;
    }

    //invoke gcc
    char *compile_str = get_compile_str(&c_unit);
    printf("compile_str: %s\n", compile_str);
    system(compile_str);

    if(compile_str)
        free(compile_str);

cleanup:
    compile_unit_delete(&c_unit);

    return 0;
}

// ------ Callbacks ------

int start_doc(SAX_Data *sd)
{
    printf("starting doc\n");
    return 1;
}

int start_node(const XMLNode *node, SAX_Data *sd)
{
    printf("starting nodes\n");

    struct compile_unit *c_unit = sd->user;
    
    c_unit->active_node->node = node;

    return 1;
}

int end_node(const XMLNode* node, SAX_Data* sd)
{
    printf("end node\n");

    struct compile_unit *c_unit = sd->user;

    print_xml_node(c_unit->active_node);

    return 1;
}

int new_text(SXML_CHAR *text, SAX_Data *sd)
{
    printf("new text found\n");

    struct compile_unit *c_unit = sd->user;

    c_unit->active_node->text = text;

    return 1;
}

int end_doc(SAX_Data *sd)
{
    printf("end document\n");
    return 1;
}

int on_error(ParseError error_num, int line_number, SAX_Data* sd)
{
    printf("error!\n");
    return 1;
}


int all_event(XMLEvent event, const XMLNode* node, SXML_CHAR* text, const int n, SAX_Data* sd)
{
    return 1;
}

// ------ Callbacks ------

void parse_file(struct compile_unit *c_unit, char *path, char *pre_str)
{
    XMLDoc doc;
    XMLDoc_init(&doc);

    SAX_Callbacks sax = { 
        .start_doc=&start_doc, 
        .start_node=&start_node, 
        .end_node=&end_node,
        .new_text=&new_text, 
        .end_doc=&end_doc, 
        .on_error=&on_error,
        .all_event=&all_event
    };

    XMLDoc_parse_file_SAX(path, &sax, c_unit);
}

void fill_str(char *str)
{
    for(int i = 0; i < STR_S; i++)
        str[i] = 0;
}

// ----- COMPILE_UNIT -----

void compile_unit_init(struct compile_unit *c_unit, char *target)
{
    c_unit->target = target;

    c_unit->option = 0;

    c_unit->compiler = 0;

    c_unit->size = 4;

    c_unit->active_node = malloc(sizeof(struct xml_node));
}

void free_string_node(struct string_node *node)
{
    do
    {
        if(node->val)
            free(node->val);
        struct string_node *f_node = node;
        node = node->next;
        if(f_node)
            free(f_node);
    }
    while(node);
}

void compile_unit_delete(struct compile_unit *c_unit)
{
    if(c_unit->option)
        free_string_node(c_unit->option);

    free(c_unit->active_node);
}

void compile_unit_print(struct compile_unit *c_unit)
{
    printf("printing compile_unit:\n");
    printf("size: %i\n", c_unit->size);

    printf("options files:\n");
    string_node_print(c_unit->option);
}

char *get_compile_str(struct compile_unit *c_unit)
{
    char *res = 0;
    res = malloc(c_unit->size + 4);

    for(int i = 0; i < c_unit->size + 4; i++)
        res[i] = 0;

    if(!c_unit->compiler)
    {
        strcpy(res, "gcc -o ");
        res += 7;
    }
    else
    {
        strcpy(res, c_unit->compiler);
        res += strlen(c_unit->compiler) + 1;
    }

    struct string_node *node;

    node = c_unit->option;
    if(node)
    {
        while(1)
        {
            strcpy(res, node->val);
            res += strlen(node->val);
            *res = ' ';
            res++;

            if(node->next)
                node = node->next;
            else
                break;
        }
    }

    *res = 0;

    res -= c_unit->size + 4;
    return res;
}



// ----- COMPILE_UNIT -----

void string_node_append(struct string_node **entry, char *data)
{
    if(*entry)
    {
        while((*entry)->next)
            entry = &(*entry)->next;
        entry = &(*entry)->next;
    }

    (*entry) = malloc(sizeof(struct string_node));
    (*entry)->val = malloc(strlen(data) + 1);
    strcpy((*entry)->val, data);
    (*entry)->val[strlen(data)] = 0;
    (*entry)->next = 0;
}

void string_node_print(struct string_node *entry)
{
    if(!entry)
        return;
    
    while(1)
    {
        printf("--val: %s\n", entry->val);
        if(entry->next)
            entry = entry->next;
        else
            break;
    }
}
