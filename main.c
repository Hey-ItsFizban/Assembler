#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_PROG_SIZE 1024
#define AS_SRCFILE_NAME_SUFFIX ".as"
#define AS_OBJECT_NAME_SUFFIX ".ob"
#define AS_ENTRY_NAME_SUFFIX ".ent"
#define AS_EXTERNAL_NAME_SUFFIX ".ext"

/*Defining constants*/
#define MAX_FILENAME 100
#define MAX_LINELENGTH 80
#define MAX_LABEL_LENGTH 31
#define MAX_LABELS 40
#define MAX_NUM_SIZE 512
#define MEMORY_OFFSET 100 /*memory address of the assembler program. starts at IC = 100*/
/*Defining global variables*/


int IC, DC;
int line_number;
int data_image[MAX_PROG_SIZE];
int instruction_image[MAX_PROG_SIZE];


const char *registers[] = {"nada", "r1", "r2", "r3", "r4", "r5", "r6", "r7", NULL};
const char *directive_types[] = {".data", ".string", ".entry", ".extern", NULL};
const char *instruc_names[] = {"mov", "cmp", "add", "sub", "not", "clr", "lea", "inc", "dec", "jmp",
                               "bne", "red", "prn", "jsr", "rts", "stop", NULL};

/*Defining data types*/
enum label_type {DATA, CODE, EXTERN};
enum instruc_addressing {IMMEDIATE, DIRECT, REGISTER, NONE};

typedef struct operand_nodeDT *operand_pointerDT; /*data type - operand*/
typedef struct operand_nodeDT{
    int operand_address;
    operand_pointerDT next;
}operand_nodeDT;

typedef struct labelDT {
    char label_name[MAX_LABEL_LENGTH + 2]; /*plus 1 for the ":" character, plus 1 for "/0"*/
    enum label_type label_type;
    int is_entry;
    int label_address;
    operand_pointerDT operand_node_pointer;

}labelDT;

typedef struct label_nodeDT *label_pointerDT;
typedef struct label_nodeDT{
    labelDT label;
    label_pointerDT next;
}label_nodeDT;


/*symbol list functions declaration*/

int add_label_to_symbolList(label_pointerDT, char *, enum label_type, int);
int add_operand_node(operand_pointerDT *, int); /*adds a new operand node to end of the list*/
void update_label_addressDATA_label(label_pointerDT p); /*adds the final IC value to the address of each element in the symbol list at the end of the first pass*/
int flag_entry_label(label_pointerDT, char *); /*flags a label if it's an entry label - @second_pass*/
void free_operands_memory(operand_pointerDT); /*free memory allocated for operands*/
void free_symbol_list(label_pointerDT); /*free memory allocated for symbol list*/
labelDT *find_label(label_pointerDT, char *);/*returns label's address according to string entered as label name, if doesn't exist returns 0*/
label_pointerDT find_entry_label(label_pointerDT point); /*returns entry label's address*/
label_pointerDT find_extern_label(label_pointerDT point); /*returns extern label's address*/



/*assisting functions declaration*/

char* my_itoa(int); /*Iterative function to implement itoa() function*/

int is_tabORspace(char a); /*returns 1 if a character is a space or a tab*/     /*OK*/
void strip_extra_spaces(char *); /*removes excess spaces and tabs*/             /*OK*/
int label_flagging(char *); /*returns 1 if a string entered ends with a colon, to declare a label*/     /*OK*/
int is_valid_label(char *);    /*returns 1 if the label entered uses the defined Assembly syntax*/     /*OK*/
int is_directive(char *);   /*checks if a string is an Assembly directive word, returns the compatible index if so*/        /*OK*/
int flag_comma(char *);  /*returns 1 if a comma is in a correct / 0 if incorrect position in string*/
int add_data(char *);   /*adds data directive input to data_image*/
int add_string(char *); /*adds string directive input to data_image*/
int is_instruction(char *); /*returns the index of the compatible Assembly instruction word, returns -1 if not an instruction*/
int add_instruction(int, int, int);    /*adds instruction to instruction_image array and allocates space for compatible oprands*/
int operand_addressing_mode(char *op); /*returns the enumeration of operand addressing mode*/
int is_src_op_valid(int ,int); /*returns 1 if source operand is compatible with the Assembly instruction word given */
int is_dest_op_valid(int, int); /*returns 1 if source and destination operands are compatible with the assembly instruction word given*/
int process_instruction(int, char*); /*if a valid Assembly instruction word- adds it to instruction_image*/
int encode_operator(label_pointerDT, char *, int, int, int); /*parse operator's string, store its code in instruction_image*/
int encode_srcNdest_operands(label_pointerDT , char *, char *); /**/
void encode_base64(int, char *); /*saves the input - encoded in base64 into a string in the function that calls encode_base64*/
int create_object_file(char *, int);
int create_entry_file(char *, label_pointerDT);
int create_extern_file(char *, label_pointerDT);


char* my_itoa(int n) {
    char* ret = NULL;
    int num_chars = 0;

    /*Determine if integer is negative*/
    int negative = 0;
    if (n < 0) {
        n = -n;
        negative = 1;
        num_chars++;
    }

    /* count how many chars */
    int temp = n;
    do {
        num_chars++;
        temp /= 10;
    } while ( temp );

    /*1 for \0 */
    ret = (char*)malloc(sizeof(char)*(num_chars + 1));
    ret[num_chars] = '\0';

    /*Add the negative sign if needed*/
    if (negative) ret[0] = '-';

    /* Copy digits to string in reverse order*/
    int i = num_chars - 1;
    do {
        ret[i--] = n % 10 + '0';
        n /= 10;
    } while (n);

    return ret;
}


int is_tabORspace(char a)
{
    if ((a == ' ') || (a == '\t') || (a == '\t\t') || (a == '\t\t\t'))
        return 1;

    return 0;
}


void strip_extra_spaces(char* current_line) {
    int i, x;
    for(i=x=0; current_line[i]; ++i)
        if(!is_tabORspace(current_line[i]) || (i > 0 && !is_tabORspace(current_line[i-1])))
            current_line[x++] = current_line[i];
    current_line[x] = '\0';
}

int label_flagging(char *label_test)
{
    int i, len = strlen(label_test);
    for (i = 0; i < len; i++)
    {
        if(*label_test == ':')
            return 1;
        label_test++;
    }

    return 0;
}

int is_valid_label(char *label_chk)
{
    if (!isalpha(*label_chk))
    {
        printf("Error in line %d, label name is illegal.\n", line_number);
        return 0;
    }
    return 1;
}

int is_directive(char *directive_chk)
{
    int i = 0;
    while(directive_types[i])
    {
        if (!strcmp(directive_chk, directive_types[i]))
            return i; /*returns the index of directive types if token is a directive*/
        i++;
    }
    return -1;
}

int flag_comma(char *ch){
    if(*ch == ','){
        printf("Error in line %d: comma is mispositioned.\n", line_number);
        return 0;
    }
    ch++;
    while(*ch){
        if(*ch == ','){
            if(*(ch+1) == '\0'){
                printf("Error in line %d: comma is mispositioned.\n", line_number);
                return 0;
            }
            if(*(ch+1) == ','){
                printf("Error in line %d: Multiple consecutive commas.\n", line_number);
                return 0;
            }
        }
        ch++;
    }
    return 1;
}

int add_data(char *ch){
    char *token;
    char *end_char = NULL;
    char comma = ",";
    int tempDC = DC;
    int temp_num;
    if(!flag_comma(ch))
    {
        printf("Error in line %d: data directive parameter is incorrect.\n", line_number);
        DC = tempDC;
        return 1;
    }
    token = strtok(ch, comma);
    while(token)
    {
        temp_num = (int) strtol(token, &end_char, 10);
        if(*end_char != '\0'){
            printf("Error in line %d: couldn't parse natural number in parameter %s.\n", line_number, token);
            DC = tempDC;
            return 1;
        }
        if(temp_num >= MAX_NUM_SIZE || temp_num < -MAX_NUM_SIZE){
            printf("Error in line %d: natural number parameter %s doesn't fit in memory.\n", line_number, token);
            DC = tempDC;
            return 1;
        }
        data_image[DC] = temp_num;
        DC++;
        token = strtok(NULL, comma);
    }
    return 0;
}

int add_string(char *ch)
{
    int i=0;

    while(ch[i] != '\0'){
        data_image[DC] = ch[i];
        i++;DC++;
    }
    data_image[DC] = '\0';
    DC++;
    return 0;
}

int is_instruction(char *str)
{
    int i = 0;
    while (instruc_names[i])
    {
        if (strcmp(instruc_names[i], str))
            return i;
        i++;
    }
    return -1;
}

int add_instruction(int instruction_type, int src, int dest)
{
    int instruction = 0;
    int operand_offset = 0;

    instruction |= (instruction_type << 6); /*places instr at the 3 final bits*/
    if (dest >= 0)
    {
        instruction |= (dest << 2);
        operand_offset += 1;
        if (src >= 0)
        {
            instruction |= (src << 4);
            if (src != REGISTER)
                operand_offset += 1;
            else if (dest != REGISTER)
                operand_offset += 1;
        }
    }
    instruction_image[IC] = instruction;
    IC++;
    IC += operand_offset;
    return 0;
}

int is_register(char *ch)
{
    int i = 1;
    while (registers[i])
    {
        if (ch == registers[i])
            return i;
        i++;
    }
    return 0;
}
int operand_addressing_mode(char *op)
{
    char *temp_op;
    char minus[] = "-";
    char strudel[] = "@";

    /*check if number*/
    if (isdigit(op))
        return IMMEDIATE;
    else if(op[0] == minus)
    {
        temp_op = strtok(op, minus);
        if(isdigit(temp_op))
            return IMMEDIATE;
    }
    /*check if register*/
    else if (op[0] == strudel)
    {
        temp_op = strtok(op, strudel);
        if (is_register(temp_op))
            return REGISTER;
    }

    return DIRECT;
}

int is_src_op_valid(int op, int instruction_type)
{
    if (instruction_type == 6 && (op != DIRECT))
        return 0;
    return 1;
}

int is_dest_op_valid(int op, int instruction_type)
{
    if ((instruction_type != 1 && instruction_type != 12) && op == IMMEDIATE)
        return 0;
    return 1;
}

int process_instruction(int instruction_type, char *input)
{
    enum instruc_addressing op1 = NONE;
    enum instruc_addressing op2 = NONE;
    char *token1, *token2, *token3;
    char comma[] = ",";
    if (instruction_type > 13)
    {
        if (input)
            printf("Error in line %d: excess parameters.\n", line_number);
        else
            return add_instruction(instruction_type, -1, -1);
    }
    else if (input)
    {
        if (!flag_comma(input))
            return 1;
        token1 = strtok(input, comma);
        token2 = strtok(NULL, comma);
        token3 = strtok(NULL, comma);
        if (token1 && !token3)
        {
            op1 = operand_addressing_mode(token1);
            if (instruction_type > 3 && instruction_type != 6)
            {
                if (token2)
                    printf("Error in line %d: excess operands.\n", line_number);
                else
                    if (is_dest_op_valid(op1, instruction_type))
                        return add_instruction(instruction_type, -1, op1);
                    else
                        printf("Error in line %d: destination operand isn't compatible to instruction entered.\n", line_number);
            }
            else
            {
                if (token2)
                {
                    if (is_src_op_valid(op1, instruction_type))
                    {
                        op2 = operand_addressing_mode(token2);
                        if (is_dest_op_valid(op2, instruction_type))
                            return add_instruction(instruction_type, op1, op2);
                        else
                            printf("Error in line %d: destination operand isn't compatible to instruction entered.\n", line_number);
                    }
                }
                else
                    printf("Error in line %d: missing destination operand.\n", line_number);
            }
        }
        else
            printf("Error in line %d: amount of operands input is incorrect.\n", line_number);
    }
    else
        printf("Error in line %d: missing operands.\n", line_number);

    return 1;
}


int encode_operator(label_pointerDT pointer, char *op, int addressing_mode, int is_source_op, int two_registers)
{
    labelDT *label;
    int num;
    int decimal = 10;

    char *end_char;

    switch (addressing_mode)
    {
        /*immediate addressing mode*/
        case 1:
            num = (int) strtol(op + 1, &end_char, decimal);
            if (*end_char != '\0')
            {
                printf("Error in line %d: failure in parsing natural number parameter.\n", line_number);
                IC++;
                return 1;
            }
            if (num >= 128 || num < -128)
            {
                printf("Error in line %d: immediate operand %s does not fit in memory.\n", line_number, op);
                IC++;
                return 1;
            }
            instruction_image[IC] = num >> 3; /*shift right 3 times to land on op-code*/
            IC++;
            break;
        /*direct addressing mode*/
        case 3:
            if (!is_valid_label(op))
            {
                printf("Error in line %d: label name \"%s\"is not a valid Assembly syntax.\n", line_number, op);
                IC++;
                return 1;
            }
            label = find_label(pointer, op);
            if (!label)
            {
                printf("Error in line %d: label \"%s\" missing.\n", line_number, op);
                IC++;
                return 1;
            }
            if (label -> label_type == EXTERN)
            {
                instruction_image[IC] = 1;
                if (add_operand_node(&label -> operand_node_pointer, IC))
                {
                    printf("Error in line %d: failure in storing operator address on an external label.\n", line_number);
                    IC++;
                    return 1;
                }
            }
            else
                instruction_image[IC] = (label -> label_address << 2 | 2); /*relocatable*/
            IC++;
            break;
        case 5:
            if (two_registers)
                instruction_image[IC - 1] |= atoi(op + 1) << 2;
            else
            {
                if (is_source_op)
                    instruction_image[IC] = atoi(op + 1) << 7; /*src register at bits 7-11*/
                else
                    instruction_image[IC] = atoi(op + 1) << 2;
                IC++;
            }
            break;
    }

    return 0;
}

int encode_srcNdest_operands(label_pointerDT point, char *op1, char *op2)
{
    int src_mask = 3584; /*1110 0000 0000*/
    int dest_mask = 28; /*0001 1100*/
    int current_instruction = instruction_image[IC];
    int error_flag = 0;

    IC++;

    /*
     *      11-10-9       8-7-6-5       4-3-2           1-0
     * src addressing     op code    dest addressing    A,R,E
     * */
    error_flag |= encode_operator(point, op1, (current_instruction & src_mask), 1, 0);
    if ((current_instruction & src_mask) == 5 && ((current_instruction & dest_mask) >> 4) == 5)
        error_flag |= encode_operator(point, op2, (current_instruction & dest_mask) >> 3, 0, 1);
    else
        error_flag |= encode_operator(point, op2, (current_instruction & dest_mask) >> 3, 0, 0);

    return error_flag;
}

int encode_dest_operand(label_pointerDT point, char *op)
{
    int dest_mask = 28; /*0001 1100*/
    int current_instruction = instruction_image[IC];

    IC++;

    return encode_operator(point, op, (current_instruction & dest_mask) >> 3, 0, 0);
}

void encode_base64(int input, char *output)
{
    char base64[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
                  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y' ,'z',
                  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

    int first_six_mask = 63; /*0011 1111 first 6 bits*/
    int first_six_bin = input & first_six_mask;
    int last_six_bin = input >> 6;

    output[0] = base64[last_six_bin];
    output[1] = base64[first_six_bin];
}


int create_object_file(char *file_name, int memory_offset)
{
    char *line_string;
    int i;
    FILE *file_pointer;
    file_pointer = fopen(file_name, "w");
    if (!file_pointer)
    {
        printf("Error: creation of \"%s\" file failed.\n", file_name);
        return 1;
    }
    fprintf(file_pointer, "%d\t%d\n", IC, DC);

    for (i = 0; i < IC; i++)
    {
        encode_base64(instruction_image[i], line_string);
        fprintf(file_pointer, "%s\n", line_string);
    }
    for (i = 0; i < DC; i++)
    {
        encode_base64(data_image[i], line_string);
    }
    fclose(file_pointer);

    return 0;
}

int create_entry_file(char *file_name, label_pointerDT point)
{
    FILE *file_pointer;
    char line[50];
    char *address_string;
    point = find_entry_label(point);
    if (point)
    {
        file_pointer = fopen(file_name, "w");
        if (!file_pointer)
        {
            printf("Error: creation of \"%s\" file failed.\n", file_name);
            return 1;
        }
        do
        {
            address_string = NULL;
            address_string = my_itoa(point -> label.label_address);

            strcpy(line, point -> label.label_name);
            strcat(line, "\t\t");
            strcat(line, address_string);
            fprintf(file_pointer, "%s\n", line);
            point = find_entry_label(point -> next);
        }while (point);
        fclose(file_pointer);
    }

    return 0;
}

int create_extern_file(char *file_name, label_pointerDT point)
{
    FILE *file_pointer;
    char line[50];
    char *address_string;
    operand_pointerDT operand_pointer;
    point = find_extern_label(point);

    if (point)
    {
        operand_pointer = point -> label.operand_node_pointer;
        if (operand_pointer)
        {
            file_pointer = fopen(file_name, "w");
            if (!file_pointer)
            {
                printf("Error: creation of \"%s\" file failed.\n", file_name);
                return 1;
            }
            do
            {
                operand_pointer = point -> label.operand_node_pointer;
                while (operand_pointer)
                {
                    address_string = my_itoa(operand_pointer -> operand_address + MEMORY_OFFSET);

                    strcpy(line, point -> label.label_name);
                    strcat(line, "\t\t");
                    strcat(line, address_string);
                    fprintf(file_pointer, "%s\n", line);
                    operand_pointer = operand_pointer -> next;
                }
                point = find_extern_label(point -> next);
            }while (point);
            fclose(file_pointer);
        }
    }

    return 0;
}


int add_label_to_symbolList(label_pointerDT label_pointer, char *name, enum label_type label_type1, int count_val)
{
    label_pointerDT pointer1, pointer2;
    label_pointerDT temp;

    temp = (label_pointerDT) malloc(sizeof(label_nodeDT));
    if (!temp)
    {
        printf("Error: memory allocation failed.\n");
        return 1;
    }
    pointer1 = label_pointer;
    strcpy(pointer1-> label.label_name, name);
    temp -> label.is_entry = 0;
    temp -> label.label_type = label_type1;
    temp -> label.label_address = count_val;
    temp -> label.operand_node_pointer = NULL;
    temp -> next = NULL;
    if (!pointer1)
        label_pointer = temp;
    else
    {
        while (pointer1)
        {
            if (!strcmp(pointer1 -> label.label_name, temp -> label.label_name))
            {
                printf("Error in line %d: label %s already exists in symbol list.\n", line_number, temp -> label.label_name);
                free (temp);
                return 1;
            }
            pointer2 = pointer1;
            pointer1 = pointer1 -> next;
        }
        pointer2 -> next = temp;
    }
    return 0;
}

int add_operand_node(operand_pointerDT *operand_pointer, int address)
{
    operand_pointerDT pointer1, pointer2;
    operand_pointerDT temp;
    temp = (operand_pointerDT) malloc(sizeof(operand_nodeDT));
    if (!temp)
    {
        printf("Error: Memory allocation failed.\n");
        return 1;
    }

    temp -> operand_address = address;
    temp -> next = NULL;

    pointer1 = *operand_pointer;
    if (!pointer1)
        *operand_pointer = temp;
    else
    {
        while (pointer1)
        {
            pointer2 = pointer1;
            pointer1 = pointer1 -> next;
        }
        pointer2 -> next = temp;
    }

    return 0;
}

void update_label_addressDATA_label(label_pointerDT p)
{
    while (p)
    {
        if (p -> label.label_type == DATA)
            p -> label.label_address += IC + MEMORY_OFFSET;
        p = p -> next;
    }
}

int flag_entry_label(label_pointerDT point, char *name)
{
    while (point)
    {
        if (!strcmp(point -> label.label_name, name))
        {
            if (point -> label.label_type != EXTERN)
            {
                point -> label.is_entry = 1;
                return 1;
            }
        }
        point = point -> next;
    }

    return 0;
}

void free_operands_memory(operand_pointerDT point)
{
    operand_pointerDT temp;
    while (point)
    {
        temp = point -> next;
        free(point);
        point = temp;
    }
}
void free_symbol_list(label_pointerDT point)
{
    label_pointerDT temp;
    while (point)
    {
        temp = point -> next;
        free_operands_memory(point -> label.operand_node_pointer);
        free(point);
        point = temp;
    }
}

labelDT *find_label(label_pointerDT point, char *name)
{
    while (point)
    {
        if (!strcmp(point ->label.label_name, name))
            return &(point -> label);
        point = point -> next;
    }

    return 0;
}

label_pointerDT find_entry_label(label_pointerDT point)
{
    while (point)
    {
        if (point -> label.is_entry)
            return point;
        point = point -> next;
    }

    return 0;
}

label_pointerDT find_extern_label(label_pointerDT point)
{
    while (point)
    {
        if (point -> label.label_type == EXTERN)
            return point;
        point = point -> next;
    }

    return 0;
}



/*first and second pass functions*/

int first_pass(FILE *, label_pointerDT);
int second_pass(FILE *, label_pointerDT);


int first_pass(FILE *filePtr, label_pointerDT label_pointer)
{
    char current_line[MAX_LINELENGTH + 5];
    char temp_buffer[MAX_LINELENGTH + 5];
    char temp_label[MAX_LABEL_LENGTH + 2];    /*plus 1 for the ":" character, plus 1 for "/0"*/
    char *line_token1;
    char *line_token2;
    char *line_token3;
    const char space[] = " ";
    const char quote_mark[] = "\"";
    const char newline[] = "\n";

    int label_flag = 0; /*0 no label. 1 has label.*/
    int label_length = 0;
    line_number = 0;
    int type_index = 0;
    int error_flag = 0;
    int line_error_flags;
    int L = 0; /*number of words in machine code*/

    while (fgets(current_line, MAX_LINELENGTH, filePtr))
    {
        line_number++;
        line_error_flags = 0;
        if (current_line[0] == ';') /*Comment line will be ignored by the assembler*/
            continue;

        strcpy(temp_buffer, current_line);
        strip_extra_spaces(current_line);
        line_token1 = strtok(current_line, space);
        label_flag = label_flagging(line_token1);
        if (!line_token1)
            continue; /*no token -> go to next line*/
        if (label_flag)
        {
            label_length = is_valid_label(line_token1);
            if (!label_length)
            {
                error_flag = 1;
                continue;
            }
            strcpy(temp_label, line_token1);
            temp_label[label_length] = '\0';
            line_token1 = strtok(NULL, space);
        }
        type_index = is_directive(line_token1);
        if (type_index >= 0) /*if true: label attached to a directive -> value will be DC*/
        {
            line_token1 = strtok(NULL, space);
            if (!line_token1)
            {
                error_flag = 1;
                printf("Error in line %d, expected additional parameters.\n", line_number);
                continue;
            }
            if (type_index == 2) /*entry*/
                continue;
            else if (type_index == 3) /*extern*/
            {
                if(add_label_to_symbolList(label_pointer, line_token1, EXTERN, 0)) /*extern label at 1st pass is inserted as value 0*/
                {
                    error_flag = 1;
                    printf("Error in line %d: failed to add label %s to symbol list.\n", line_number, line_token1);
                    continue;
                }
            }
            else
            {
                if (label_flag)
                {
                    if (add_label_to_symbolList(label_pointer, temp_label, DATA, DC))
                    {
                        error_flag = 1;
                        printf("Error in line %d: failed to add label %s to symbol list.\n", line_number, temp_label);
                        continue;
                    }
                }
                line_token2 = strtok(NULL, space);
                line_token3 = strtok(NULL, space);
                if (type_index == 0)
                {
                    line_error_flags |= add_data(line_token1);
                    if (line_token2)
                    {
                        error_flag = 1;
                        printf("Error in line %d: excess parameters typed in.\n", line_number);
                    }
                }
                else if (type_index == 1)
                {
                    line_token1 = strtok(temp_label, quote_mark);
                    line_token2 = strtok(NULL, quote_mark);
                    line_token3 = strtok(NULL, quote_mark);
                    if (line_token1 && line_token2 && line_token3 && !strtok(line_token3, newline))
                        line_error_flags |= add_string(line_token2);
                    else
                    {
                        error_flag = 1;
                        printf("Error in line %d: incorrect parameter.\n", line_number);
                    }
                }
                if (line_error_flags)
                {
                    printf("Error in processing line %d: storage failed.\n", line_number);
                    error_flag = 1;
                }
            }
        }
        else
        {
            if (label_flag)
            {
                if (add_label_to_symbolList(label_pointer, temp_label, CODE, IC + (MEMORY_OFFSET)))
                {
                    printf("Error in line %d: failed to add label \"%s\" to symbol list", line_number, line_token1);
                    line_error_flags = 1;
                }
            }
            type_index = is_instruction(line_token1);
            if (type_index < 0)
            {
                printf("Error in line %d: \"%s\".\tExpected an instruction or a directive word.\n", line_number, line_token1);
                line_error_flags = 1;
            }
            else
            {
                line_token1 = strtok(NULL, space);
                if (strtok(NULL, space))
                {
                    printf("Error in line %d: excess parameters.\n", line_number);
                    error_flag = 1;
                    continue;
                }
                if (process_instruction(type_index, line_token1))
                    line_error_flags = 1;
            }
        }
        error_flag |= line_error_flags;
    }
    update_label_addressDATA_label(label_pointer);

    return 0;
}

int second_pass(FILE *filePtr, label_pointerDT label_pointer)
{
    char *token, *op1, *op2;
    char current_line[MAX_LINELENGTH + 5];

    int type_index = 0;
    int line_error_flag;
    int error_flag = 0;

    const char space[] = " ";
    const char comma[] = ",";
    const char semicolon = ';';
    const char newline[] = "\n";

    line_number = 0;
    IC = 0;

    while (fgets(current_line, MAX_LINELENGTH, filePtr))
    {
        line_number++;
        line_error_flag = 0;
        if(current_line[0] == semicolon)
            continue;
        strip_extra_spaces(current_line);
        token = strtok(current_line, space);
        if (!token)
            continue;
        if (label_flagging(token))
            token = strtok(NULL, space);

        type_index = is_directive(token);
        if (type_index >= 0 && type_index != 2)
            continue;
        else if (type_index == 2)
        {
            token = strtok(NULL, space);
            if (flag_entry_label(label_pointer, token))
            {
                printf("Error in line %d: entry label %s does not exist in symbol list / label is an external label.\n", line_number, token);
                line_error_flag = 1;
            }
        }
        else
        {
            token = strtok(NULL, space);
            op1 = strtok(token, comma);
            if (!op1)
            {
                IC++;
                continue;
            }
            op2 = strtok(NULL, comma);
            if (!op2)
            {
                if(encode_srcNdest_operands(label_pointer, op1, op2))
                {
                    printf("Error at processing line %d: encode failure.\n", line_number);
                    line_error_flag = 1;
                }
            }
            else
            {
                if (encode_dest_operand(label_pointer, op1))
                {
                    printf("Error at processing line %d: encode failure.\n", line_number);
                    line_error_flag = 1;
                }
            }
        }
        error_flag |= line_error_flag;
    }

    return error_flag;
}

/*main program*/
/*
 * The main function receives command line arguments and runs all assembler functions
 * Handles input files, runs through the first and second passes
 * If input file uses the correct assembly syntax the main function creates the necessary assembly files:
 * .as      .ob     .ent    .ext
 * */

int main(int argc, char *argv[]){

    int i; /*index*/
    FILE *filePtr;
    label_pointerDT label_pointer;
    char as_filename[MAX_FILENAME];

    for (i = 1; i < argc; i++)
    {
        strcpy(as_filename, argv[i]);
        strcat(as_filename, AS_SRCFILE_NAME_SUFFIX);
        filePtr = fopen(as_filename, "r");
        if (!filePtr)
        {
            printf("\nFile %s does not exist.\n", as_filename);
            continue;
        }
        printf("\nStarting assembly for file %s.\nRunning first pass", as_filename);
        label_pointer = NULL;
        IC = 0;
        DC = 0;
        /*run first pass*/
        if (first_pass(filePtr, label_pointer))
        {
            printf("First pass failed, Assembler process aborted for file %s.\n", as_filename);
            fclose(filePtr);
            free_symbol_list(label_pointer);
            continue;
        }

        /*run second pass*/
        fclose(filePtr);
        filePtr = fopen(as_filename, "r");

        if (second_pass(filePtr, label_pointer))
        {
            printf("Second pass failed, Assembler process aborted for file %s.\n", as_filename);
            fclose(filePtr);
            free_symbol_list(label_pointer);
            continue;
        }

        fclose(filePtr);
        strcpy(as_filename, argv[i]);
        strcpy(as_filename, AS_OBJECT_NAME_SUFFIX);

        if (create_object_file(as_filename, MEMORY_OFFSET))
        {
            printf("Creating assembly object file failed, Assembler process aborted for file %s.\n", as_filename);
            remove(as_filename);
            free_symbol_list(label_pointer);
            continue;
        }

        fclose(filePtr);
        strcpy(as_filename, argv[i]);
        strcpy(as_filename, AS_ENTRY_NAME_SUFFIX);
        if (create_entry_file(as_filename, label_pointer))
        {
            printf("Creating assembly entry file failed, Assembler process aborted for file %s.\n", as_filename);
            remove(as_filename);
            free_symbol_list(label_pointer);
            continue;
        }
        strcpy(as_filename, argv[i]);
        strcpy(as_filename, AS_EXTERNAL_NAME_SUFFIX);
        if (create_extern_file(as_filename, label_pointer))
        {
            printf("Creating assembly external file failed, Assembler process aborted for file %s.\n", as_filename);
            remove(as_filename);
            free_symbol_list(label_pointer);
            continue;
        }
        printf("Assembler process for source file %s.as completed.\n", as_filename);
        free_symbol_list(label_pointer);
    }

    return 0;
}
