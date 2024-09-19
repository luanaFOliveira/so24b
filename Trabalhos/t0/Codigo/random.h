#ifndef RANDOM_H
#define RANDOM_H
#include "err.h"
typedef struct rand_t rand_t;

rand_t* rand_cria(int limite);
void rand_destroi();
err_t rand_le(void* disp,int id,int *pvalor);

#endif
