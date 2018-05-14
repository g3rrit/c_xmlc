#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yxml.h"

#define BUFSIZE 8192
#define STR_S 256

// ----- ATTRIBUTE -----

#define MAX_ATTRIBUTES 10

struct attribute
{
    char *elem;
    char *attr[MAX_ATTRIBUTES];
    char *value[MAX_ATTRIBUTES];
    int att_size;

    char *data;
};

void attribute_init(struct attribute *this);

void attribute_elem(struct attribute *this, char *elem);

void attribute_add(struct attribute *this, char *attr, char *value);

void attribute_data(struct attribute *this, char *data);

void attribute_delete(struct attribute *this);


// ----- ATTRIBUTE -----

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
    struct string_node *option;

    char *compiler;

    char *target;

    int size;
};

void compile_unit_init(struct compile_unit *c_unit, char *target);

void compile_unit_delete(struct compile_unit *c_unit);

void compile_unit_print(struct compile_unit *c_unit);

char *get_compile_str(struct compile_unit *c_unit);

// ----- COMPILE_UNIT -----

void parse_file(struct compile_unit *c_unit, char *path, char *pre_str);

void fill_str(char *str);

void print_yxml_error(yxml_ret_t r);

void handle_yxml_output(struct compile_unit *c_unit, char *attr, char *elem, char *value);

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

void parse_file(struct compile_unit *c_unit, char *path, char *pre_str)
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

    printf("parsing file: %s\n", path);

    //parse xml
    yxml_t xml; 
    buffer = malloc(BUFSIZE);
    yxml_init(&xml, buffer, BUFSIZE);

    value = malloc(STR_S);
    int valpos = 0;

    char curr_c = fgetc(file);

    struct attribute attr;
    attribute_init(&attr);

    char data_buffer[1024];

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
                if(pre_str)
                {
                    memcpy(value + strlen(pre_str) + 1, value, strlen(value));
                    value[strlen(pre_str)] = '/';
                    memcpy(value, pre_str, strlen(pre_str));
                }
                printf("pi is: %s\n", xml.pi);

                /*for(int i = 0; i < 20; i++)
                    printf("nextc: %c\n", fgetc(file));
                    */

                handle_yxml_output(c_unit, xml.attr, xml.elem, value);
                break;
        }

        //-----
        int seek_size = 1;
        for(char ch = fgetc(file); ch == ' '; ch = fgetc(file))
                seek_size++;

        if(ch == '>')
        {
            //handle attribute
            memset(data_buffer, 0, 1024);
            for(int i = 0; i < 1024; i++)
            {
                ch = fgetc(file);
                seek_size++;
                if(ch == '<')
                    break;

                data_buffer[i] = ch;
            }
             

        }

        fseek(file, -seek_size, SEEK_CUR);
        //-----

        curr_c = fgetc(file);
    }
    printf("end of file\n");
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

}


// ----- ATTRIBUTE -----

void attribute_init(struct attribute *this)
{
    this->elem = 0;
    for(int i = 0; i < MAX_ATTRIBUTES; i++)
        this->attr[i] = 0;
    for(int i = 0; i < MAX_ATTRIBUTES; i++)
        this->value[i] = 0;

    this->att_size = 0;

    this->data = 0;
}

void attribute_elem(struct attribute *this, char *elem)
{
    if(this->elem)
        free(this->elem);

    this->elem = malloc(strlen(elem) + 1);
    strcpy(this->elem, elem);
    this->elem[strlen(elem)] = 0;
}

void attribute_add(struct attribute *this, char *attr, char *value)
{
    this->attr[this->att_size] = malloc(strlen(attr) + 1);
    strcpy(this->attr[this->att_size], attr);
    this->attr[this->att_size][strlen(attr)] = 0;

    this->value[this->att_size] = malloc(strlen(value) + 1);
    strcpy(this->value[this->att_size], value);
    this->value[this->att_size][strlen(value)] = 0;

    this->att_size++;
}

void attribute_data(struct attribute *this, char *data)
{
    this->data = malloc(strlen(data));
    strcpy(this->data, data);
    this->data[strlen(data)] = 0;
}

void attribute_delete(struct attribute *this)
{
    for(int i = 0; i < this->attr_size; i++)
        if(this->attr[i])
            free(this->attr[i]);

    for(int i = 0; i < this->attr_size; i++)
        if(this->value[i])
            free(this->value[i]);

    if(this->elem)
        free(this->elem);

    if(this->data)
        free(this->data);
}

// ----- ATTRIBUTE -----


// ----- COMPILE_UNIT -----

void compile_unit_init(struct compile_unit *c_unit, char *target)
{
    c_unit->target = target;

    c_unit->option = 0;

    c_unit->compiler = 0;

    c_unit->size = 4;
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
