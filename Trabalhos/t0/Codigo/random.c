#include "es.h"
#include "err.h"
#include "random.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

struct rand_t{
	int limite;
};

rand_t* rand_cria(int limite){
	rand_t* self = malloc(sizeof(rand_t));
	self->limite = limite;
	srand(time(NULL));
	return self;
}

void rand_destroi(rand_t* self){
	free(self);
}

err_t rand_le(void *disp,int id,int *pvalor){
	
	err_t err = ERR_OK;
	rand_t *self = disp;
	
	if(self == NULL){
		err = ERR_INSTR_INV;
	}
	
	*pvalor = rand()% self->limite;
	return err;
	
}
