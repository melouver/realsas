#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define REG0 ((IR >> 24) & 0x07)
#define REG1 ((IR >> 20) & 0x0F)
#define REG2 ((IR >> 16) & 0x0F)
#define IMMEDIATE (IR & 0xFFFF)
#define ADDRESS (IR & 0xFFFFFF)
#define PORT (IR & 0xFF)
#define OPCODE ((IR >> 27) & 0x1F)

typedef struct _PROG_STATE_WORD {
    unsigned short overflow_flg: 1;
    unsigned short compare_flg: 1;
    unsigned short reserve: 14;
}PROG_STATE_WORD;

unsigned char* CS;
unsigned char* DS;
unsigned char* SS;
unsigned char* ES;
unsigned char* ss_stack_pointer;
unsigned char* es_stack_pointer;
unsigned char* MEM;

unsigned int* PC;
short GR[8];
PROG_STATE_WORD PSW;

unsigned int IR;

void PrintMemory(unsigned long mem_size) {
    unsigned char* tmp = MEM;
    int cur = 0;
    for (;tmp != MEM + mem_size; tmp++) {
        printf("  %d", *tmp);
        cur++;
        if (cur%4 == 0) {
            printf("\n");
        }
    }
}

int HLT();//ok
int JMP();//ok
int CJMP();//ok
int OJMP();//ok
int CALL();//?
int RET();//?
int PUSH();//
int POP();
int LOADB();//ok
int LOADW();
int STOREB();
int STOREW();
int LOADI();
int NOP();
int IN();
int OUT();
int ADD();
int ADDI();
int SUB();
int SUBI();
int MUL();
int DIV();
int AND();
int OR();
int NOR();
int NOTB();
int SAL();
int SAR();
int EQU();
int LT();
int LTE();
int NOTC();

int main(int argc, char* argv[]) {

    unsigned int instruction;
    unsigned long mem_size;
    int (*ops[])() = {HLT, JMP, CJMP, OJMP, CALL, RET, PUSH, POP, LOADB, LOADW, STOREB, STOREW,
                      LOADI, NOP, IN, OUT, ADD, ADDI, SUB, SUBI, MUL, DIV, AND, OR, NOR, NOTB,
                      SAL, SAR, EQU, LT, LTE, NOTC};

    FILE* code_IR_file;
    int ret = 1, n;

    if (argc < 2) {
        printf("ERROR: no enough command line arguments!\n");
        exit(-1);
    }


    mem_size = 0x1000;

    if ((MEM = (unsigned char*)malloc(sizeof(unsigned char) * mem_size)) == NULL) {
        printf("ERROR: failed to allocate memory!\n");
        exit(-1);
    }
    /*
     * write code segment
     */
    PC = (unsigned int*)MEM;


    if ((code_IR_file = fopen(argv[1], "r")) == NULL) {
        printf("Failed to open file: %s", argv[2]);
        exit(-1);
    }
    CS = (unsigned char*)MEM;
    int all_line = 0;
    fscanf(code_IR_file, "%i", &instruction);
    while (!feof(code_IR_file)) {
        memcpy(PC, &instruction, sizeof(instruction));
        all_line++;
        PC++;
        fscanf(code_IR_file, "%i", &instruction);
    }
    all_line -= ceil(instruction/4) + 1;

    unsigned char* tmp_ptr = (unsigned char*)PC;
    tmp_ptr -= (int)(ceil(instruction/4)+1)*4;
    PC = (unsigned int*)tmp_ptr;
    DS = tmp_ptr;
    /* write data segment*/

    rewind(code_IR_file);
    fscanf(code_IR_file, "%i", &instruction);
    while (all_line > 1) {
        fscanf(code_IR_file, "%i", &instruction);
        all_line--;
    }

    /*if ((data_seg_file = fopen(argv[3], "r")) == NULL) {
        printf("Failed to open file: %s", argv[3]);
        exit(-1);
    }*/

    fscanf(code_IR_file, "%i", &instruction);
    while (!feof(code_IR_file)) {
        unsigned  char *byte_ptr = (unsigned char*)PC;
        unsigned char tmp;
        for (int i = 3; i >= 0; --i) {
            tmp = instruction >> (i*8);
            memcpy(byte_ptr++, &tmp, sizeof(unsigned char));
        }
        PC++;
        fscanf(code_IR_file, "%i", &instruction);
    }



    /* SS and ES have same capcity*/
    ss_stack_pointer = SS = (unsigned char*)PC;

    es_stack_pointer = ES = (SS + (MEM + mem_size - SS)/2);
    /*
     * reset PC to the beginning of code
     */
    PC = (unsigned int*) MEM;
    while (ret) {
        IR = *PC;
        PC++;
        ret = (*ops[OPCODE])();
    }
    free(MEM);
    fclose(code_IR_file);
    return 0;
}

int HLT() {
    return 0;
}

int JMP() {
    PC = (unsigned int*)(CS + ADDRESS);
    return 1;
}

int CJMP() {
    if (PSW.compare_flg)
        PC = (unsigned int*)(MEM + ADDRESS);
    return 1;
}

int OJMP() {
    if (PSW.overflow_flg)
        PC = (unsigned int*)(MEM + ADDRESS);
    return 1;
}

int CALL() {

    memcpy(es_stack_pointer, GR, sizeof(short)*8);
    es_stack_pointer += sizeof(short)*8;
    memcpy(es_stack_pointer, &PSW, sizeof(PSW));
    es_stack_pointer += sizeof(PSW);
    memcpy(es_stack_pointer, &PC, sizeof(unsigned int*));
    es_stack_pointer += sizeof(unsigned int*);
    PC = (unsigned int*)(CS + ADDRESS);

    return 1;
}

int RET() {

    es_stack_pointer -= sizeof(unsigned int*);
    memcpy(&PC, es_stack_pointer, sizeof(unsigned int*));
    es_stack_pointer -= sizeof(PSW);
    memcpy(&PSW, es_stack_pointer, sizeof(PSW));
    es_stack_pointer -= sizeof(short)*8;
    memcpy(GR, es_stack_pointer, sizeof(short)*8);

    return 1;
}

int PUSH() {

    memcpy(ss_stack_pointer, GR+REG0, sizeof(short));
    ss_stack_pointer += sizeof(short );
    return 1;
}

int POP() {

    ss_stack_pointer -= sizeof(short);
    memcpy(GR+REG0, ss_stack_pointer, sizeof(short));
    return 1;
}

int LOADB() {

    GR[REG0] = (short )(* (unsigned char*)(DS + ADDRESS + GR[7]));
    return 1;
}

int LOADW() {

    GR[REG0] = *(short *)(DS + ADDRESS + GR[7]*2);
    return 1;
}

int STOREB() {

    *(unsigned char*)(DS + ADDRESS + GR[7]) = GR[REG0];
    return 1;
}

int STOREW() {

    *(short *)(DS + ADDRESS + GR[7]*2) = GR[REG0];
    return 1;
}

int LOADI() {
    GR[REG0] = (short)IMMEDIATE;
    return 1;
}

int NOP() {

    return 1;
}

int IN() {

    //TODO: make PORT useful!!!!
    char ch;
    ch = getchar();
    GR[REG0] = (short)(ch & 0x00FF);
    return 1;
}


int OUT() {

    //fprintf(solution, "%c", GR[REG0]);
    printf("%c", GR[REG0]);
    return 1;
}

int ADD() {

    GR[REG0] = GR[REG1] + GR[REG2];
    if (GR[REG2] >= 0) {
        PSW.overflow_flg = GR[REG0] < GR[REG1] ? 1 : 0;
    }else if (GR[REG2] < 0) {
        PSW.overflow_flg = GR[REG0] > GR[REG1] ? 1 : 0;
    }
    return 1;
}

int ADDI(){

    short reg1 = GR[REG0];

    GR[REG0] = reg1 + (short)IMMEDIATE;
    if (reg1 >= 0) {
        PSW.overflow_flg = GR[REG0] < (short)IMMEDIATE ?
                           1 : 0;
    }else if (reg1 < 0) {
        PSW.overflow_flg = GR[REG0] > (short )IMMEDIATE ?
                           1 : 0;
    }
    return 1;
}

int SUB() {


    GR[REG0] = GR[REG1] - GR[REG2];

    if (GR[REG2] >= 0) {
        PSW.overflow_flg = GR[REG0] > GR[REG1] ? 1 : 0;
    }else if (GR[REG2] < 0) {
        PSW.overflow_flg = GR[REG0] > GR[REG1] ? 0 : 1;
    }
    return 1;
}

int SUBI() {

    short reg1 = GR[REG0];
    short ime = (short)IMMEDIATE;
    GR[REG0] = reg1 - ime;

    if (ime >= 0) {
        PSW.overflow_flg = GR[REG0] < reg1 ? 0 : 1;
    }else if (ime < 0) {
        PSW.overflow_flg = GR[REG0] > reg1 ? 0 : 1;
    }

    return 1;
}

int MUL() {

    GR[REG0] = GR[REG1] * GR[REG2];
    int res = GR[REG1] * GR[REG2];

    if (res > 32767 || res < -32768) {
        PSW.overflow_flg = 1;
    }else {
        PSW.overflow_flg = 0;
    }

    return 1;
}

int DIV() {

    if (GR[REG2] == 0) {
        printf("divide 0 fatal error!\n");
        exit(-1);
    }

    GR[REG0] = GR[REG1] / GR[REG2];
    return 1;
}

int AND() {
    GR[REG0] = GR[REG1] & GR[REG2];

    return 1;
}


int OR() {

    GR[REG0] = GR[REG1] | GR[REG2];

    return 1;
}

int NOR() {

    GR[REG0] = GR[REG1] ^ GR[REG2];

    return 1;
}

int NOTB() {


    GR[REG0] = ~GR[REG1];

    return 1;
}

int SAL() {
    GR[REG0] = GR[REG1] << GR[REG2];

    return 1;
}

int SAR() {

    GR[REG0] = GR[REG1] >> GR[REG2];

    return 1;
}
int EQU() {

    if (GR[REG0] == GR[REG1]) {
        PSW.compare_flg = 1;
    }else {
        PSW.compare_flg = 0;
    }

    return 1;
}

int LT() {

    if (GR[REG0] < GR[REG1]) {
        PSW.compare_flg = 1;
    }else {
        PSW.compare_flg = 0;
    }

    return 1;
}

int LTE() {

    if (GR[REG0] <= GR[REG1]) {
        PSW.compare_flg = 1;
    }else {
        PSW.compare_flg = 0;
    }
    return 1;
}

int NOTC() {
    if (PSW.compare_flg == 1) {
        PSW.compare_flg = 0;
    }else if (PSW.compare_flg == 0) {
        PSW.compare_flg = 1;
    }

    return 1;
}




































