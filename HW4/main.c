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

#define	ET_NONE	0	// No file type
#define	ET_EXEC	2	// Executable file

#define SHT_SYMTAB 	2

#define STB_LOCAL	0
#define STB_GLOBAL 	1

pid_t run_target(const char* programname, char *const argv[]) {
	pid_t pid;
	
	pid = fork();
	
    if (pid > 0) {
		return pid;
		
    } 
	else if (pid == 0) {
		/* Allow tracing of this process */
		if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
			perror("ptrace");
			exit(1);
		}
		/* Replace this process's image with the given program */
		execv(programname, argv);
		
	} 
	else {
		// fork error
		perror("fork");
        exit(1);
    }
}

void run_debugger(pid_t child_pid, unsigned long addr, bool is_dynamic) {
	int count = 1;
	int wait_status;
    struct user_regs_struct regs;
	unsigned long rsp;
	unsigned long data;
	unsigned long data_trap;
	unsigned long return_address;
	unsigned long return_data;
	unsigned long return_data_trap;
	unsigned long old_addr = addr;
	
	
	wait(&wait_status);
	if (is_dynamic) {
		addr = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)addr, NULL);
		addr -= 6;
	}
	
	data = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)addr, NULL);
	data_trap = (data & 0xFFFFFFFFFFFFFF00) | 0xCC;
	ptrace(PTRACE_POKETEXT, child_pid, (void*)addr, (void*)data_trap);

	ptrace(PTRACE_CONT, child_pid, NULL, NULL);
    wait(&wait_status);
    while (WIFSTOPPED(wait_status)) {
		ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
		rsp = regs.rsp;
		return_address = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)regs.rsp, NULL);
		return_data = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)return_address, NULL);
		
		return_data_trap = (return_data & 0xFFFFFFFFFFFFFF00) | 0xCC;		
		
		ptrace(PTRACE_POKETEXT, child_pid, (void*)return_address, (void*)return_data_trap);

		ptrace(PTRACE_POKETEXT, child_pid, (void*)addr, (void*)data);
		regs.rip -= 1;
    	ptrace(PTRACE_SETREGS, child_pid, NULL, &regs);
		printf("PRF:: run #%d first parameter is %d\n", count, ((signed int)regs.rdi));

		ptrace(PTRACE_CONT, child_pid, NULL, NULL);
		wait(&wait_status);
		 if(!WIFSTOPPED(wait_status)){
            break;
        }

		ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
		while (regs.rsp != rsp + 8) {
			ptrace(PTRACE_POKETEXT, child_pid, (void*)return_address, (void*)return_data);
        	regs.rip -= 1;
        	ptrace(PTRACE_SETREGS, child_pid, NULL, &regs);
        	ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL);
        	wait(&wait_status);

        	if(!WIFSTOPPED(wait_status)){
               return;
           	}

           	ptrace(PTRACE_POKETEXT, child_pid, (void*)return_address, (void*)return_data_trap);

           	ptrace(PTRACE_CONT, child_pid, NULL, NULL);
           	wait(&wait_status);
           	if(!WIFSTOPPED(wait_status)){
           	    return;
           	}
           	ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
		}
		regs.rip -= 1;
		ptrace(PTRACE_POKETEXT, child_pid, (void*)return_address, (void*)return_data);
    	ptrace(PTRACE_SETREGS, child_pid, NULL, &regs);
		printf("PRF:: run #%d returned with %d\n", count, ((signed int)regs.rax));
		count++;

		if(is_dynamic && count == 2){
            addr = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)old_addr, NULL);
			data = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)addr, NULL);
			data_trap = (data & 0xFFFFFFFFFFFFFF00) | 0xCC;
        }
		ptrace(PTRACE_POKETEXT, child_pid, (void*)addr, (void*)data_trap);
        ptrace(PTRACE_CONT, child_pid, NULL, NULL);
        wait(&wait_status);
	}
}

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

unsigned long find_dyn_symbol(char* symbol_name, char* exe_file_name) {
    Elf64_Ehdr elf_header;
	Elf64_Shdr strings_section;
	Elf64_Rela* rela_table = NULL;
    Elf64_Sym* dyn_symbols = NULL;
    int index = -1;
    int num_of_rela = 0;
    int num_of_symbols = 0;
    char* dyn_names = NULL;
	char* strings_table = NULL;
	char* section_name = NULL;
	unsigned long addr = 0;
	FILE* exe_file = fopen(exe_file_name, "rb");
    if (exe_file == NULL || ftell(exe_file) == -1) {
		goto end;
	}

    fread(&elf_header, sizeof(elf_header), 1, exe_file);    
    fseek(exe_file, elf_header.e_shoff + elf_header.e_shentsize * elf_header.e_shstrndx, SEEK_SET);
    fread(&strings_section, elf_header.e_shentsize, 1, exe_file);

    strings_table = (char*)malloc(strings_section.sh_size);
	if (strings_table == NULL) {
		goto end;
	}
    fseek(exe_file, strings_section.sh_offset, SEEK_SET);
    fread(strings_table, strings_section.sh_size, 1, exe_file);

    for (int i = 0; i < elf_header.e_shnum; i++) {
        fseek(exe_file, elf_header.e_shoff + i * sizeof(strings_section), SEEK_SET);
        fread(&strings_section, sizeof(strings_section), 1, exe_file);
        section_name = (char*)(strings_table + strings_section.sh_name);

        if (!strcmp(section_name, ".dynsym")) {
            dyn_symbols = (Elf64_Sym*)malloc(strings_section.sh_size);
			if (dyn_symbols == NULL) {
				goto end;
			}
            fseek(exe_file, strings_section.sh_offset, SEEK_SET);
            fread(dyn_symbols, strings_section.sh_size, 1, exe_file);
            num_of_symbols = strings_section.sh_size / strings_section.sh_entsize;
        }

        else if (!strcmp(section_name, ".rela.plt")) {
            rela_table = malloc(strings_section.sh_size);
			if (rela_table == NULL) {
				goto end;
			}
            fseek(exe_file, strings_section.sh_offset, SEEK_SET);
            fread(rela_table, strings_section.sh_size , 1, exe_file);
            num_of_rela = strings_section.sh_size / strings_section.sh_entsize;
        }

        else if (!strcmp(section_name, ".dynstr")) {
            dyn_names = (char*)malloc(strings_section.sh_size);
			if (dyn_names == NULL) {
				goto end;
			}
            fseek(exe_file, strings_section.sh_offset, SEEK_SET);
            fread(dyn_names, strings_section.sh_size, 1, exe_file);
        }
    }

    for (int i = 0; i < num_of_rela; i++) {
        Elf64_Rela curr_rela = rela_table[i];
        char* curr = dyn_names + dyn_symbols[ELF64_R_SYM(curr_rela.r_info)].st_name;
        if (!strcmp(curr, symbol_name)){
            index = i;
        }
    }

	if (index != -1) {
		addr = rela_table[index].r_offset;
	}
end:
    if (exe_file != NULL) {
		fclose(exe_file);
	}
	if (dyn_names != NULL) {
		free(dyn_names);
	}
	if (dyn_symbols != NULL) {
		free(dyn_symbols);
	}
	if (rela_table != NULL) {
		free(rela_table);
	}
	if (strings_table != NULL) {
   		free(strings_table);
	}
    return addr;
}


int main(int argc, char *const argv[]) {
	int err = 0;
	unsigned long addr = find_symbol(argv[1], argv[2], &err);
    pid_t child_pid;
	
	if (err == -2) {
		printf("PRF:: %s is not a global symbol!\n", argv[1]);
        return 0;
    }
	else if (err == -1) {
		printf("PRF:: %s not found! :(\n", argv[1]);
        return 0;
    }
	else if (err == -3) {
		printf("PRF:: %s not an executable!\n", argv[2]);
        return 0;
    }
	else if (err == -4) {
        addr = find_dyn_symbol(argv[1], argv[2]);
    }
	child_pid = run_target(argv[2], argv + 2);
    run_debugger(child_pid, addr, err == -4);

	return 0;
}