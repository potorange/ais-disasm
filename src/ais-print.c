#include "ais-print.h"
#include "hashtab.h"

extern hashtab_t *s_table;

#define TIC6X_VMA_FMT "l"
#define tic6x_sprintf_vma(s,x) sprintf (s, "%08" TIC6X_VMA_FMT "x", x)

char *
tic6x_get_symbol(ais_vma addr)
{
	return (char *)ht_search(s_table, &addr, sizeof(addr));
}

void
tic6x_print_label(bfd_vma addr, char *buf)
{
       char *symbol = tic6x_get_symbol(addr);
       buf[0] = '\0';
       if (symbol != NULL) {
               sprintf(buf, "%s:", symbol);
       }
}

void
tic6x_print_address(bfd_vma addr, struct disassemble_info *info)
{
  char *sym = tic6x_get_symbol(addr);
  if (sym == NULL) {
	  char buf[30];
	  tic6x_sprintf_vma (buf, addr);
	  (*info->fprintf_func) (info->stream, "0x%s", buf);
  } else {
	  (*info->fprintf_func) (info->stream, "%s", sym);
  }
}

int
tic6x_section_print_byte(bfd_vma addr, struct disassemble_info *info)
{
	bfd_byte byte;
	buffer_read_memory (addr, (bfd_byte *)&byte, 1, info);
	(*info->fprintf_func) (info->stream, ".byte 0x%02x", byte);
	return 1;
}

int
tic6x_section_print_short(bfd_vma addr, struct disassemble_info *info)
{
	unsigned short hword;
	if (addr & 1)
		return tic6x_section_print_byte(addr, info);
	buffer_read_memory (addr, (bfd_byte *)&hword, 2, info);
	(*info->fprintf_func) (info->stream, ".short 0x%04x", hword);
	return 2;
}

int
tic6x_section_print_word(bfd_vma addr, struct disassemble_info *info)
{
	unsigned int word;
	switch (addr & 3) {
	case 1:
	case 3:
		return tic6x_section_print_byte(addr, info);
	case 2:
		return tic6x_section_print_short(addr, info);
	}
	buffer_read_memory (addr, (bfd_byte *)&word, 4, info);
	char *sym = tic6x_get_symbol(word);
	if (sym == NULL) {
		(*info->fprintf_func) (info->stream, ".word 0x%08x", word);
	} else {
		(*info->fprintf_func) (info->stream, ".word %s", sym);
	}
	return 4;
}

int
tic6x_section_print_string(bfd_vma addr, struct disassemble_info *info)
{
    int count = 0;
    bfd_byte c;
    info->fprintf_func(info->stream, ".string '");
    buffer_read_memory (addr++, &c, 1, info);
    count++;
    while (c != 0) {
        if (c == '\n')
            info->fprintf_func(info->stream, "\\n");
        else if (c == '\r')
            info->fprintf_func(info->stream, "\\r");
        else if (c == '\t')
            info->fprintf_func(info->stream, "\\t");
        else
            info->fprintf_func(info->stream, "%c", c);
        buffer_read_memory (addr++, &c, 1, info);
        count++;
    }
    info->fprintf_func(info->stream, "',0");
    return count;
}

#define MAX_STR 64
#define PRINT_MODE_STRING 0
#define PRINT_MODE_DATA 0

int
tic6x_section_print_mixed(bfd_vma addr, struct disassemble_info *info)
{
    bfd_byte str[MAX_STR + 1];
    int printable = 1;
    int offset = 0 ;
    int d = 0;
    int i;

    if (addr == 0xc008c9feu)
	d = 1;

    buffer_read_memory (addr, str, MAX_STR, info);
    str[MAX_STR] = 0;

    for (offset = 0 ; offset < 3 ; offset++) {
	printable = 1;
        for (i = offset ; i < offset + strlen(str + offset) ; i++) {
            if ((str[i] < 32 || str[i] > 126) && str[i] != '\n' && str[i] != '\t' && str[i] != '\r') {
                printable = 0;
                break;
	    }
        }
    	if (printable && strlen(str + offset) > 1) {
		switch (offset) {
		case 0:
			return tic6x_section_print_string(addr, info);
		case 1:
		case 3:
			return tic6x_section_print_byte(addr, info);
		case 2:
			return tic6x_section_print_short(addr, info);
		}
	}
    }

    return tic6x_section_print_word(addr, info);
}
