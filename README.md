# SPYRE
David Wilson

"Spyre" is a programming language that began development in December, 2015.  This is
the sixth (or so) iteration of Spyre.  The previous iterations can be found at:
+ https://github.com/ForeverDev/spyre
+ https://github.com/ForeverDev/spyre2
+ https://github.com/ForeverDev/spyre3
+ https://github.com/ForeverDev/spyre4
+ https://github.com/ForeverDev/spyre5

Each version of Spyre is increasingly better than the last (as my knowledge about
programming languages has grown).  

Spyre is a statically typed, interpreted programming language.  There are two
data types in Spyre: `int` and `float`.  Both have 64 bit storage.  `int` is
automatically signed (although I may add unsigned support in the future?).

Spyre's compiler is bootstrapping.  That is, the Spyre compiler is written 
in Spyre.  The way this works is I wrote (am in the process of writing) 
a very small Spyre compiler in "Spyre assembly" that converts a source file 
into "Spyre assembly".  Then, using that very small Spyre language, I can 
rewrite a more complex compiler.

Spyre source code compiles into a set of instructions targeted at the Spyre
Virtual Machine.  Here is the set of instructions: (NOTE: Currently, the 
instruction set is pretty messy and out of order, I plan on improving this
in the future when I don't need to add any more instructions)

MNEMONIC	| OPCODE	| OPERANDS  
----------- | --------- | --------  
NOOP		| 0x00		|		    
IPUSH		| 0x01		| INT64	constant
IADD		| 0x02		|
ISUB		| 0x03		|
IMUL		| 0x04		|
IDIV		| 0x05		|
IMOD		| 0x06		|
SHL			| 0x07		|
SHR			| 0x08		|
AND			| 0x09		|
OR			| 0x0A		|
XOR			| 0x0B		|
NOT			| 0x0C		|
NEG			| 0x0D		|
IGT			| 0x0E		|
IGE			| 0x0F		|
ILT			| 0x10		|
ILE			| 0x11		|
ICMP		| 0x12		|
JNZ			| 0x13		| INT32 addr
JZ			| 0x14		| INT32 addr
JMP			| 0x15		| INT32 addr
CALL		| 0x16		| INT32 addr, INT32 nargs
IRET		| 0x17		| 
CCALL		| 0x18		| INT32 ptrToCFunctionName
< TODO add the rest (I'm lazy) >
