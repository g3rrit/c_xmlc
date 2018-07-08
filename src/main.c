#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sxmlc.h"

#define BUFSIZE 8192
#define STR_S 256

#define MAX_ATTRIBS 5

struct string_node
{
    char *val;
    struct string_node *next;
};

void string_node_append(struct string_node **entry, char *data);

void string_node_print(struct string_node *entry);

// ----- XML NODE ------

struct attrib_node
{
    char *name;
    char *value;
};

struct xml_node
{
    char *tag;
    char *text;
    
    struct attrib_node attributes[MAX_ATTRIBS];
    int att_count;
};

void xml_node_init(struct xml_node *this, char *tag)
{
    if(tag)
    {
        int len = strlen(tag);
        this->tag = malloc(len + 1);
        strcpy(this->tag, tag);
        this->tag[len] = 0;
    }
    else
    {
        this->tag = 0;
    }

    this->att_count = 0;
}

void xml_node_text(struct xml_node *this, char *text)
{
    int len = strlen(text);
    this->text = malloc(len + 1);
    strcpy(this->text, text);
    this->text[len] = 0;
}

void xml_node_add(struct xml_node *this, char *name, char *value)
{
    struct attrib_node *temp = &this->attributes[this->att_count];

    int n_len = strlen(name);
    int v_len = strlen(value);
    temp->name = malloc(n_len + 1);
    strcpy(temp->name, name);
    temp->name[n_len] = 0;

    temp->value = malloc(v_len + 1);
    strcpy(temp->value, value);
    temp->value[v_len] = 0;

    this->att_count++;
}

void xml_node_delete(struct xml_node *this)
{
    struct attrib_node *temp;
    for(int i = 0; i < this->att_count; i++)
    {
        temp = &this->attributes[i];
        free(temp->name);
        free(temp->value);
    }

    if(this->tag)
        free(this->tag);
    if(this->tag)
        free(this->text);

    this->att_count = 0;
}


void print_xml_node(struct xml_node *node)
{
    printf("node ->\ntag: %s\n", node->tag);

    printf("text: %s\n", node->text);

    printf("%i attributes\n", node->att_count);

    for(int i = 0; i < node->att_count; i++)
    {
        printf("att[%i] name: %s value: %s\n", i, node->attributes[i].name, node->attributes[i].value);
    }
}
// ----- XML NODE ------

// ----- COMPILE_UNIT -----
struct compile_unit
{
    struct string_node *option;

    char *compiler;
    int size;

    //the active text inside a node
    struct xml_node *active_node;

    char *pre_str;
};

void compile_unit_init(struct compile_unit *c_unit);

void compile_unit_delete(struct compile_unit *c_unit);

void compile_unit_print(struct compile_unit *c_unit);

char *get_compile_str(struct compile_unit *c_unit);

// ----- COMPILE_UNIT -----

void parse_file(struct compile_unit *c_unit, char *path);

void fill_str(char *str);

int main(int argc, char *argv[])
{
    printf("Starting XMLC\n");

    struct compile_unit c_unit;
    compile_unit_init(&c_unit);

    parse_file(&c_unit, "build.xml");

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
    return 1;
}

int start_node(const XMLNode *node, SAX_Data *sd)
{
    struct compile_unit *c_unit = sd->user;

    if(!strcmp(node->tag, "build"))
    {
        if(node->n_attributes < 1)
        {
            if(c_unit->compiler)
                return 1;
            printf("no compiler selected\n");
            return 0;
        }

        c_unit->compiler = malloc(strlen(node->attributes[0].value));
        strcpy(c_unit->compiler, node->attributes[0].value);

        c_unit->size += strlen(node->attributes[0].value);

        printf("compiler selected: %s\n", c_unit->compiler);
            
        return 1;
    }
    
    xml_node_delete(c_unit->active_node);
    xml_node_init(c_unit->active_node, node->tag);

    for(int i = 0; i < node->n_attributes; i++)
        xml_node_add(c_unit->active_node, node->attributes[i].name, node->attributes[i].value);

    return 1;
}

int end_node(const XMLNode* node, SAX_Data* sd)
{
    struct compile_unit *c_unit = sd->user;

#define c_node c_unit->active_node

    if(!strcmp(node->tag, "include"))
    {
        char *b_path = c_node->text;
        for(; *b_path == ' '; b_path++);
        char *e_path = b_path;
        for(; *e_path != ' '; e_path++);
        *e_path = 0;
        parse_file(c_unit, b_path);
        return 1;
    }
    else if(strcmp(node->tag, "option"))
        return 1;

    //handle node
    char *prefix = 0;
    int include = 0;

#define DATA_SIZE 2048
    char data[DATA_SIZE];
    memset(data, 0, DATA_SIZE);

    for(int i = 0; i < c_node->att_count; i++)
    {
        //could check for value aswell
        if(!strcmp(c_node->attributes[i].name, "include"))
            include = 1;
        else if(!strcmp(c_node->attributes[i].name, "prefix"))
            prefix = c_node->attributes[i].value;
    }

    //replace newline with space
    for(char *c = c_node->text; *c != 0; c++)
        if(*c == '\n')
            *c = ' ';
        else if(*c == '\t')
            *c = ' ';
        else if(*c == '\r')
            *c = ' ';


    int p_len = 0;
    if(prefix)
        p_len = strlen(prefix);

    int d_pos = 0;
    int t_pos = 0;

    char *curr_c = 0;
    char *next_c = 0;
    curr_c = c_node->text;
    while(1)
    {
        for(; *curr_c == ' '; curr_c++)
            if(*curr_c == 0)
                goto append;

        for(next_c = curr_c; *next_c != ' '; next_c++)
            if(*next_c == 0)
                goto append;

        int s_len = next_c - curr_c;

        //printf("next size: %i\n", next_c - curr_c + p_len + 5);

        //data = realloc(data, next_c - curr_c + p_len + 5);

        data[d_pos] = ' ';
        d_pos++;
        if(prefix)
            memcpy(data + d_pos, prefix, p_len);

        d_pos += p_len; 

        memcpy(data + d_pos, curr_c, s_len);
        d_pos += s_len;

        curr_c += s_len;

    }
append:
    data[d_pos] = 0;

    c_unit->size += d_pos;

    string_node_append(&c_unit->option, data);

#undef c_node
    return 1;
}

int new_text(SXML_CHAR *text, SAX_Data *sd)
{
    struct compile_unit *c_unit = sd->user;

    xml_node_text(c_unit->active_node, text);

    return 1;
}

int end_doc(SAX_Data *sd)
{
    return 1;
}

int on_error(ParseError error_num, int line_number, SAX_Data* sd)
{
    printf("error parsing file\n");
    return 1;
}


int all_event(XMLEvent event, const XMLNode* node, SXML_CHAR* text, const int n, SAX_Data* sd)
{
    return 1;
}

// ------ Callbacks ------

void parse_file(struct compile_unit *c_unit, char *path)
{
    //append pre str
    char *needle = strrchr(path, '/');
    if(needle)
    {
        int len = needle - path;
        if(c_unit->pre_str)
        {
            int len_p = strlen(c_unit->pre_str);
            c_unit->pre_str = realloc(c_unit->pre_str, strlen(c_unit->pre_str + len + 1));
            memcpy(c_unit->pre_str + len_p, path, len);
            c_unit->pre_str[len_p + len] = 0;
        }
        else
        {
            c_unit->pre_str = malloc(len + 1);
            memcpy(c_unit->pre_str, path, len);
            c_unit->pre_str[len] = 0;
        }
    }
    //---- 
    
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

    printf("parsing file path[%s]\n", path);

    XMLDoc_parse_file_SAX(path, &sax, c_unit);
}

void fill_str(char *str)
{
    for(int i = 0; i < STR_S; i++)
        str[i] = 0;
}

// ----- COMPILE_UNIT -----

void compile_unit_init(struct compile_unit *c_unit)
{
    c_unit->option = 0;

    c_unit->compiler = 0;

    c_unit->size = 4;

    c_unit->pre_str = 0;

    c_unit->active_node = malloc(sizeof(struct xml_node));
    xml_node_init(c_unit->active_node, 0);
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

    if(c_unit->compiler)
        free(c_unit->compiler);

    if(c_unit->pre_str)
        free(c_unit->pre_str);

    xml_node_delete(c_unit->active_node);
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
    res = malloc(c_unit->size);
    char *start = res;

    for(int i = 0; i < c_unit->size; i++)
        res[i] = 0;

    strcpy(res, c_unit->compiler);
    res += strlen(c_unit->compiler);

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

    return start;
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
