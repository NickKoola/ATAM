#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <syscall.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "elf64.h"

#define	ET_NONE	0	//No file type 
#define	ET_REL	1	//Relocatable file 
#define	ET_EXEC	2	//Executable file 
#define	ET_DYN	3	//Shared object file 
#define	ET_CORE	4	//Core file 

#define SHT_SYMTAB 2
#define SHT_STRTAB 3

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2


#define DEBUG

/* symbol_name		- The symbol (maybe function) we need to search for.
 * exe_file_name	- The file where we search the symbol in.
 * error_val		- If  1: A global symbol was found, and defined in the given executable.
 * 			- If -1: Symbol not found.
 *			- If -2: Only a local symbol was found.
 * 			- If -3: File is not an executable.
 * 			- If -4: The symbol was found, it is global, but it is not defined in the executable.
 * return value		- The address which the symbol_name will be loaded to, if the symbol was found and is global.
 */
unsigned long find_symbol(char* symbol_name, char* exe_file_name, int* error_val) {
	unsigned long return_val = 0;
	Elf64_Ehdr elf_header;
	Elf64_Sym symbol;
	Elf64_Shdr section_header;
	Elf64_Shdr strtab_header;
	Elf64_Shdr symtab_header;
	int symbol_size = 0;
	unsigned long offset = 0;
	char* buff = NULL;
	bool found_local = false;
	if (!symbol_name || !exe_file_name || !error_val) {
		goto end;
	}
	symbol_size = strlen(symbol_name) + 1;
	buff = malloc((symbol_size)*sizeof(char));
	if (buff == NULL) {
		goto end;
	}
	
	FILE* exe_file = fopen(exe_file_name, "rb");
	if (exe_file == NULL || ftell(exe_file) == -1) {
		*error_val = -3;
		goto end;
	}
	// read the elf header
	fread(&elf_header, sizeof(Elf64_Ehdr), 1, exe_file);
	
	// check if it is an executable
	if (elf_header.e_type != ET_EXEC) {
		*error_val = -3;
		goto end;
	}

	// read the section header table
	fseek(exe_file, elf_header.e_shoff, SEEK_SET);
	for (int i = 0; i < elf_header.e_shnum; i++) {
		fread(&section_header, sizeof(Elf64_Shdr), 1, exe_file);
		if (section_header.sh_type == SHT_SYMTAB) {
			symtab_header = section_header;
		}
	}
	Elf64_Off off = elf_header.e_shoff + symtab_header.sh_link*sizeof(Elf64_Shdr);
	fseek(exe_file, off, SEEK_SET);
	
	fread(&strtab_header, sizeof(Elf64_Shdr), 1, exe_file);
	
	fseek(exe_file, symtab_header.sh_offset, SEEK_SET);
	
	for (int i = 0; i < (symtab_header.sh_size / symtab_header.sh_entsize); i++) {		
		fread(&symbol, sizeof(Elf64_Sym), 1, exe_file);
		if (symbol.st_name == ET_NONE) {
			continue;
		}
		offset = ftell(exe_file);
		
		fseek(exe_file, strtab_header.sh_offset + symbol.st_name, SEEK_SET);
		
		fread(buff, symbol_size, 1, exe_file);

		fseek(exe_file, offset, SEEK_SET);
		if (buff[symbol_size-1] == '\0' && strcmp(buff, symbol_name)== 0)  {
			switch (ELF64_ST_BIND(symbol.st_info))
			{
			case STB_LOCAL:
				found_local = true;
				break;
			
			case STB_GLOBAL:
				if (symbol.st_shndx == SHN_UNDEF) {
					*error_val = -4;
					goto end;
				}
				else {
					*error_val = 1;
					return_val = symbol.st_value;
					goto end;
				}
				break;
			default:
				// should not be reached
				*error_val = -5;
				goto end;
				break;
			}
		}
	}
	if (found_local) {
		*error_val = -2;
		goto end;
	}
	*error_val = -1;
	// check if it is an executable
	
end:
	if (exe_file != NULL) {
		fclose(exe_file);
	}
	if (buff != NULL) {
		free(buff);
	}
	return return_val;
}

int main(int argc, char *const argv[]) {
	int err = 0;
	unsigned long addr = find_symbol(argv[1], argv[2], &err);

	if (addr > 0)
		printf("%s will be loaded to 0x%lx\n", argv[1], addr);
	else if (err == -2)
		printf("%s is not a global symbol! :(\n", argv[1]);
	else if (err == -1)
		printf("%s not found!\n", argv[1]);
	else if (err == -3)
		printf("%s not an executable! :(\n", argv[2]);
	else if (err == -4)
		printf("%s is a global symbol, but will come from a shared library\n", argv[1]);
	return 0;
}