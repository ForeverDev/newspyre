#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "assembler.h"

const AssemblerInstruction instructions[0xFF] = {
	{"NOOP",	0x00, {NO_OPERAND}},
	{"IPUSH",	0x01, {INT64}},
	{"IADD",	0x02, {NO_OPERAND}},
	{"ISUB",	0x03, {NO_OPERAND}},
	{"IMUL",	0x04, {NO_OPERAND}},
	{"IDIV",	0x05, {NO_OPERAND}},
	{"IMOD",	0x06, {NO_OPERAND}},
	{"SHL",		0x07, {NO_OPERAND}},
	{"SHR",		0x08, {NO_OPERAND}},
	{"AND",		0x09, {NO_OPERAND}},
	{"OR",		0x0A, {NO_OPERAND}},
	{"XOR",		0x0B, {NO_OPERAND}},
	{"NOT",		0x0C, {NO_OPERAND}},
	{"NEG",		0x0D, {NO_OPERAND}},
	{"IGT",		0x0E, {NO_OPERAND}},
	{"IGE",		0x0F, {NO_OPERAND}},
	{"ILT",		0x10, {NO_OPERAND}},
	{"ILE",		0x11, {NO_OPERAND}},
	{"ICMP",	0x12, {NO_OPERAND}},
	{"JNZ",		0x13, {INT32}},
	{"JZ",		0x14, {INT32}},
	{"JMP",		0x15, {INT32}},
	{"CALL",	0x16, {INT32, INT32}},
	{"IRET",	0x17, {NO_OPERAND}},
	{"CCALL",	0x18, {INT32, INT32}},
	{"FPUSH",	0x19, {FLOAT64}},
	{"FADD",	0x1A, {NO_OPERAND}},
	{"FSUB",	0x1B, {NO_OPERAND}},
	{"FMUL",	0x1C, {NO_OPERAND}},
	{"FDIV",	0x1D, {NO_OPERAND}},
	{"FGT",		0x1E, {NO_OPERAND}},
	{"FGE",		0x1F, {NO_OPERAND}},
	{"FLT",		0x20, {NO_OPERAND}},
	{"FLE",		0x21, {NO_OPERAND}},
	{"FCMP",	0x22, {NO_OPERAND}}
};

void
Assembler_generateBytecodeFile(const char* in_file_name) {
	Assembler A;
	AssemblerFile input;
	input.handle = fopen(in_file_name, "r");
	if (!input.handle) {
		Assembler_die(&A, "Couldn't open source file '%s'", in_file_name);
	}
	fseek(input.handle, 0, SEEK_END);
	input.length = ftell(input.handle);
	fseek(input.handle, 0, SEEK_SET);
	input.contents = (char *)malloc(input.length + 1);
	fread(input.contents, 1, input.length, input.handle);
	input.contents[input.length] = 0;
	fclose(input.handle);

	AssemblerFile output;
	size_t name_len;
	char* out_file_name;
	name_len = strlen(in_file_name);
	out_file_name = malloc(name_len + 1);
	strcpy(out_file_name, in_file_name);
	out_file_name[name_len] = 0;
	out_file_name[name_len - 1] = 'b'; /* .spys -> .spyb */
	output.handle = fopen(out_file_name, "wb");
	if (!output.handle) {
		Assembler_die(&A, "Couldn't open output file for writing");
	}
	output.length = 0;
	output.contents = NULL;

	A.tokens = Lexer_convertToTokens(input.contents);
	while (A.tokens) {
		switch (A.tokens->type) {
			case PUNCT:
				switch (A.tokens->word[0]) {
								
				}
				break;
			case IDENTIFIER:
			{
				const AssemblerInstruction* ins;
				if (!(ins = Assembler_validateInstruction(&A, A.tokens->word))) {
					Assembler_die(&A, "unknown instruction '%s'", A.tokens->word);
				}
				fputc(ins->opcode, output.handle);
				/* go through the operands */
				for (int i = 0; i < 4; i++) {
					if (ins->operands[i] == NO_OPERAND) break;
					A.tokens = A.tokens->next;
					if (!A.tokens) {
						Assembler_die(&A, "expected operand(s)");
					}
					switch (ins->operands[i]) {
						case INT64:
						{
							uint64_t n = strtol(A.tokens->word, NULL, 10);
							fwrite(&n, 1, sizeof(uint64_t), output.handle);
							break;
						}
						case INT32:
						{
							uint32_t n = (uint32_t)strtol(A.tokens->word, NULL, 10);
							fwrite(&n, 1, sizeof(uint32_t), output.handle);
						}
							break;
						case FLOAT64:
						{
							double n = strtod(A.tokens->word, NULL);
							fwrite(&n, 1, sizeof(double), output.handle);
							break;
						}
						case NO_OPERAND:
							break;
					}
				}
				break;
			}
			case NUMBER:
				break;
			case NOTOK:
				break;
		}
		A.tokens = A.tokens->next;
	}

	fclose(output.handle);	
	free(A.tokens);
	free(input.contents);
}

static void
Assembler_die(Assembler* A, const char* format, ...) {
	va_list list;
	printf("\n*** Spyre assembler error ***\n");
	va_start(list, format);
	vprintf(format, list);
	va_end(list);
	printf("\n\n");
	exit(1);
}

/* 0 = not valid, 1 = valid */
static const AssemblerInstruction*
Assembler_validateInstruction(Assembler* A, const char* instruction) {
	unsigned int index = 0;
	while (&instructions[index]) {
		if (!strcmp_lower(instructions[index].name, instruction)) {
			return &instructions[index];	
		};
		index++;
	}
	return 0;
}

/* case insensitive strcmp (for various validations) */
/* 0 = strings are same (to match strcmp) */
static int 
strcmp_lower(const char* a, const char* b) {
	if (strlen(a) != strlen(b)) return 1;
	while (*a) {
		if (tolower(*a++) != tolower(*b++)) return 1; 
	}
	return 0;
}
