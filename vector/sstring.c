/**
 * Vector Lab
 * CS 241 - Spring 2019
 */
 
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    vector* str_vec;
};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
	sstring* result = malloc(sizeof(sstring));
	result->str_vec = vector_create(char_copy_constructor, char_destructor, char_default_constructor);
	
	char curr;
	size_t i;
	for (i = 0; i < strlen(input); i++) {
		curr = input[i];
		vector_push_back(result->str_vec, &curr);
	}	
    return result;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
	size_t size = vector_size(input->str_vec); 
    char* result = malloc((size + 1) * sizeof(char));
	size_t i;
	for (i = 0; i < size; i++) {
		result[i] = *((char*) vector_get(input->str_vec, i));
	}
	result[size] = '\0';
	return result;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
	size_t new_size = vector_size(this->str_vec) + vector_size(addition->str_vec);
	vector_reserve(this->str_vec, new_size);
	size_t i;
	for (i = 0; i < vector_size(addition->str_vec); i++) {
		vector_push_back(this->str_vec, vector_get(addition->str_vec, i));
	}
	return new_size;
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
	vector* result = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
	bool more_del = false;
	size_t i;
	char temp_char;
	sstring* temp = cstr_to_sstring("");
	for (i = 0; i < vector_size(this->str_vec); i++) {
		if ((*(char*)vector_get(this->str_vec, i)) == delimiter && !more_del) {
			vector_push_back(result, sstring_to_cstr(temp));
			sstring_destroy(temp);
			more_del = true;
			temp = cstr_to_sstring("");
		} else if ((*(char*)vector_get(this->str_vec, i)) == delimiter && more_del) {
			more_del = true;
			vector_push_back(result, "");
		} else {
			more_del = false;
			temp_char = *(char*)vector_get(this->str_vec, i);
			sstring_append(temp, cstr_to_sstring(&temp_char));
		}
	}
    return result;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    return -1;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    return NULL;
}

void sstring_destroy(sstring *this) {
    // your code goes here
	vector_destroy(this->str_vec);
	this->str_vec = NULL;
	free(this);
	this = NULL;
	return;
}
