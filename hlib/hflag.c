#include "core.h"
#include "hstring.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hflag.h"

typedef enum HFlagType {
	HFLAGTYPE_BOOL,
	HFLAGTYPE_INT,
	HFLAGTYPE_FLOAT,
	HFLAGTYPE_STRING
} HFlagType;

typedef union HFlagValue {
	i64 i;
	f64 f;
	str s;
	bool b;
} HFlagValue;


typedef struct HFlag {
	char      short_name;
	char*     long_name;
	char*     description;
	HFlagType type;
	HFlagValue value;
	HFlagValue default_value;
} HFlag;

#define FLAGS_CAP 128
HFlag flags[FLAGS_CAP] = {
	{
		.short_name = 'h',
		.long_name = "help",
		.description = "Print this help message and exit.",
		.type = HFLAGTYPE_BOOL,
		.value = {.b = false},
		.default_value = {.b = false},
	}
};
usize flags_len = 1;

#define EXTRA_ARGS_CAP 128
char*  extra_args[EXTRA_ARGS_CAP];
usize extra_args_len = 0;

char* help_intro = NULL;

void hflag_set_help_intro(char* intro) {
	help_intro = intro;
}

i64* hflag_int(char short_name, char* long_name, char* description, i64 default_value) {
	assert(flags_len < FLAGS_CAP);
	nullpanic(long_name);
	flags[flags_len] = (HFlag){
		.short_name = short_name,
		.long_name = long_name,
		.description = description,
		.type = HFLAGTYPE_INT,
		.default_value = {.i = default_value},
		.value = {.i = default_value},
	};
	flags_len++;
	return &flags[flags_len-1].value.i;
}

str* hflag_str(char short_name, char* long_name, char* description, str default_value) {
	assert(flags_len < FLAGS_CAP);
	nullpanic(long_name);
	flags[flags_len] = (HFlag){
		.short_name = short_name,
		.long_name = long_name,
		.description = description,
		.type = HFLAGTYPE_STRING,
		.default_value = {.s = default_value},
		.value = {.s = default_value},
	};
	flags_len++;
	return &flags[flags_len-1].value.s;
}

f64* hflag_float(char short_name, char* long_name, char* description, f64 default_value) {
	assert(flags_len < FLAGS_CAP);
	nullpanic(long_name);
	flags[flags_len] = (HFlag){
		.short_name = short_name,
		.long_name = long_name,
		.description = description,
		.type = HFLAGTYPE_FLOAT,
		.default_value = {.f = default_value},
		.value = {.f = default_value},
	};
	flags_len++;
	return &flags[flags_len-1].value.f;
}

bool* hflag_bool(char short_name, char* long_name, char* description) {
	assert(flags_len < FLAGS_CAP);
	nullpanic(long_name);
	flags[flags_len] = (HFlag){
		.short_name = short_name,
		.long_name = long_name,
		.description = description,
		.type = HFLAGTYPE_BOOL,
		.default_value = {.b = false},
		.value = {.b = false},
	};
	flags_len++;
	return &flags[flags_len-1].value.b;
}

// internal
HFlagValue hflag_parse_value(HFlagType type, char* value) {
	nullpanic(value);
	switch(type) {
		case HFLAGTYPE_BOOL:
			unreachable(); // Flags do not need a value, they are set to true with just -f, not -f true|false
			return (HFlagValue){0};
		case HFLAGTYPE_INT: {
			char* endptr;
			i64 result = strtoll(value, &endptr, 10);
			if (*endptr != '\0') {
				panicf("Invalid integer value: %s", value);
			}
			return (HFlagValue){.i = result};
		}
		case HFLAGTYPE_FLOAT: {
			char* endptr;
			f64 result = strtod(value, &endptr);
			if (*endptr != '\0') {
				panicf("Invalid float value: %s", value);
			}
			return (HFlagValue){.f = result};
		}
		case HFLAGTYPE_STRING: {
			return (HFlagValue){.s = str_from_cstr(value)};
		}
	}
	unreachable();
	return (HFlagValue){0};
}

void print_help(char* program_name) {
	if (help_intro) {
		fprintf(stderr, "%s", help_intro);
	} else {
		fprintf(stderr, "Usage: %s [options]\n", program_name);
	}
	for (usize i = 0; i < flags_len; i++) {
		HFlag flag = flags[i];
		if(flag.short_name) {
			fprintf(stderr, "    -%c,  ", flag.short_name);
		}
		else {
			fprintf(stderr, "         ");
		}

		fprintf(stderr, "--%-10s  %s ", flag.long_name, flag.description);
		switch (flag.type) {
			case HFLAGTYPE_BOOL:
				fprintf(stderr, "[bool, default: false]\n");
				break;
			case HFLAGTYPE_INT:
				fprintf(stderr, "[int, default: %li]\n", flag.default_value.i);
				break;
			case HFLAGTYPE_STRING:
				fprintf(stderr, "[string, default: %.*s]\n", (int)flag.default_value.s.len, flag.default_value.s.data);
				break;
			case HFLAGTYPE_FLOAT:
				fprintf(stderr, "[float, default: %lf]\n", flag.default_value.f);
				break;
		}
	}
}

// NOTE: If argument parsing fails, this function just fails
void hflag_parse(int* argc_pointer, char*** argv_pointer) {
	char** argv = *argv_pointer;
	int argc = *argc_pointer;

	char* program_name = argv[0];
	argv++; argc--;

	while (argc > 0) {
		char* arg = argv[0];
		nullpanic(arg);

		if (arg[0] == '-' && arg[1] == '-' && arg[2] == '\0') { // "--", stop parsing, interpret literally
			argv++; argc--;
			while(argc > 0) {
				assert(extra_args_len < EXTRA_ARGS_CAP);
				extra_args[extra_args_len] = argv[0];
				extra_args_len++;
				argv++; argc--;
			}
		}
		else if (arg[0] == '-' && arg[2] == '\0') { // short name
			char name = arg[1];
			int flag = -1;

			for(usize i = 0; i < flags_len; i++) {
				if (flags[i].short_name == name) {
					flag = i;
					break;
				}
			}

			if (flag == -1) panicf("Unknown flag '%c'", name);
			if (flags[flag].type == HFLAGTYPE_BOOL) {
				flags[flag].value.b = true;
			}
			else {
				argv++; argc--;
				if (argc == 0) panicf("Flag '%c' requires an argument", name);
				flags[flag].value = hflag_parse_value(flags[flag].type, argv[0]);
			}
		}
		else if (arg[0] == '-' && arg[1] == '-') { // long name
			char* name = &arg[2];

			int flag = -1;
			for(usize i = 0; i < flags_len; i++) {
				if (strcmp(flags[i].long_name, name) == 0) {
					flag = i;
					break;
				}
			}

			if (flag == -1) panicf("Unknown flag '%s'", name);
			if (flags[flag].type == HFLAGTYPE_BOOL) {
				flags[flag].value.b = true;
			}
			else {
				argv++; argc--;
				if (argc == 0) panicf("Flag '%s' requires an argument", name);
				flags[flag].value = hflag_parse_value(flags[flag].type, argv[0]);
			}
		}
		else { // Extra argument, not a flag
			assert(extra_args_len < EXTRA_ARGS_CAP);
			extra_args[extra_args_len] = arg;
			extra_args_len++;
		}
		argv++; argc--;
	}

	*argv_pointer = extra_args;
	*argc_pointer = extra_args_len;

	if (flags[0].value.b == true) { // Help flag activated
		print_help(program_name);
		exit(0);
	}
}
