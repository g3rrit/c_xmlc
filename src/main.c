#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yxml.h"

#define BUFSIZE 8192
#define STR_S 256

struct string_node
{
    char *val;
    struct string_node *next;
};

void string_node_append(struct string_node **entry, char *data);

void string_node_print(struct string_node *entry);

// ----- COMPILE_UNIT -----
struct compile_unit
{
    struct string_node *source;    
    struct string_node *include;
    struct string_node *link;
    char *output;

    int size;
};

void compile_unit_init(struct compile_unit *c_unit);

void compile_unit_delete(struct compile_unit *c_unit);

void compile_unit_print(struct compile_unit *c_unit);

char *get_compile_str(struct compile_unit *c_unit);

// ----- COMPILE_UNIT -----

void parse_file(struct compile_unit *c_unit, char *path);

void fill_str(char *str);

void print_yxml_error(yxml_ret_t r);

void handle_yxml_output(struct compile_unit *c_unit, char *attr, char *elem, char *value);

int main(int argc, char *argv[])
{
    printf("Starting XMLC\n");

    struct compile_unit c_unit;
    compile_unit_init(&c_unit);

    parse_file(&c_unit, "build.xml");

    compile_unit_print(&c_unit);

    //invoke gcc
    char *compile_str = get_compile_str(&c_unit);
    printf("compile_str: %s\n", compile_str);
    system(compile_str);
    if(compile_str)
        free(compile_str);

    compile_unit_delete(&c_unit);

    return 0;
}


void parse_file(struct compile_unit *c_unit, char *path)
{
    //char *xmldata = 0;

    void *buffer = 0;

    char *value = 0;

    FILE *file = fopen(path, "rb");
    if(!file) 
    {
        printf("error reading file: %s\n", path); 
        goto cleanup;
    }

    /*
    fseek(file, 0L, SEEK_END);
    long lsize = ftell(file);
    rewind(fp);

    xmldata = malloc(lsize + 1);

    if(fread(xmldata, lsize, 1, file) != 1)
    {
        printf("error copying file to memory\n");
        goto cleanup;
    }
    fclose(file);
    file = 0;
    */

    printf("parsing file: %s\n", path);

    //parse xml
    yxml_t xml; 
    buffer = malloc(BUFSIZE);
    yxml_init(&xml, buffer, BUFSIZE);

    value = malloc(STR_S);
    int valpos = 0;

    char curr_c = fgetc(file);
    while(!feof(file))
    {
        yxml_ret_t r = yxml_parse(&xml, curr_c);
        if(r < 0)
        {
            printf("error parsing file:\n");
            printf("----> file: %s - line: %i - byte: %i - byte offset %i\n", path, xml.line, xml.byte, xml.total);
            print_yxml_error(r);
            goto cleanup;
        }

        switch(r)
        {
            case YXML_ATTRSTART:
                fill_str(value);
                valpos = 0;
                break;
        
            case YXML_ATTRVAL:
                value[valpos] = *xml.data;
                valpos++;
                break;

            case YXML_ATTREND:
                handle_yxml_output(c_unit, xml.attr, xml.elem, value);
                break;
        }

        curr_c = fgetc(file);
    }
    //parse xml
    
cleanup:
    //if(xmldata)
    //    free(xmldata);
    if(file)
        fclose(file);
    if(buffer)
        free(buffer);
    if(value)
        free(value);
}

void fill_str(char *str)
{
    for(int i = 0; i < STR_S; i++)
        str[i] = 0;
}

void print_yxml_error(yxml_ret_t r)
{
    printf("--------> ");
    switch(r)
    {
    case YXML_EREF:
        printf("invalid character er entity reference. e.g. &whatever");
        break;
    case YXML_ECLOSE:
        printf("close tag does not match open tag");
        break;
    case YXML_ESTACK:
        printf("stack overflow\n");
        break;
    case YXML_ESYN:
        printf("miscollaneous syntax error");
        break;
    }    
    printf("\n");
}

void handle_yxml_output(struct compile_unit *c_unit, char *attr, char *elem, char *value)
{
    printf("found element: %s - attr: %s - value: %s\n", elem, attr, value);

    if(strcmp(elem, "include") == 0)
    {
        parse_file(c_unit, value);
    }
    else if(strcmp(elem, "source") == 0)
    {
        string_node_append(&c_unit->source, value);
        c_unit->size += strlen(value) + 1;
    }
    else if(strcmp(elem, "flag") == 0)
    {
        if(strcmp(attr, "link") == 0)
        {
            string_node_append(&c_unit->link, value);
            c_unit->size += strlen(value) + 1 + 2;
        }
        else if(strcmp(attr, "include") == 0)
        {
            string_node_append(&c_unit->include, value);
            c_unit->size += strlen(value) + 1 + 2;
        }
        else if(strcmp(attr, "output") == 0)
        {
            c_unit->size -= strlen(c_unit->output);
            c_unit->size += strlen(value);

            if(c_unit->output)
                free(c_unit->output);

            c_unit->output = malloc(strlen(value) + 1);
            strcpy(c_unit->output, value);
            c_unit->output[strlen(value)] = 0;
        }
    }
}

// ----- COMPILE_UNIT -----

void compile_unit_init(struct compile_unit *c_unit)
{
    c_unit->source = 0;

    c_unit->include = 0;

    c_unit->link = 0;

    c_unit->output = malloc(6);
    strcpy(c_unit->output, "a.out");
    c_unit->output[5] = 0;
    c_unit->size = strlen(c_unit->output) + 4;
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
    if(c_unit->source)
        free_string_node(c_unit->source);
    if(c_unit->include)
        free_string_node(c_unit->include);
    if(c_unit->link)
        free_string_node(c_unit->link);

    free(c_unit->output);
}

void compile_unit_print(struct compile_unit *c_unit)
{
    printf("printing compile_unit:\n");
    printf("size: %i\n", c_unit->size);

    printf("source files:\n");
    string_node_print(c_unit->source);
    printf("include dirs:\n");
    string_node_print(c_unit->include);
    printf("link with:\n");
    string_node_print(c_unit->link);

    printf("output: %s\n", c_unit->output);
}

char *get_compile_str(struct compile_unit *c_unit)
{
    char *res = 0;
    res = malloc(c_unit->size + 4);

    for(int i = 0; i < c_unit->size + 4; i++)
        res[i] = 0;

    strcpy(res, "gcc -o ");
    res += 7;
    strcpy(res, c_unit->output);
    res += strlen(c_unit->output);
    *res = ' ';
    res++;

    struct string_node *node;
    node = c_unit->include;
    while(1)
    {
        strcpy(res, "-I");
        res += 2;
        strcpy(res, node->val);
        res += strlen(node->val);
        *res = ' ';
        res++;

        if(node->next)
            node = node->next;
        else
            break;
    }
    
    node = c_unit->link;
    while(1)
    {
        strcpy(res, "-l");
        res += 2;
        strcpy(res, node->val);
        res += strlen(node->val);
        *res = ' ';
        res++;

        if(node->next)
            node = node->next;
        else
            break;
    }
    node = c_unit->source;
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

    res -= c_unit->size + 4;
    return res ;
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
    while(1)
    {
        printf("--val: %s\n", entry->val);
        if(entry->next)
            entry = entry->next;
        else
            break;
    }
}
