#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

/*
 * Quatro threads estão envolvidas nesse problema: um agente e três fumantes. Os fumantes
 * ficam em um loop eterno, primeiro esperando por ingredientes, e então fazendo e fumando
 * os cigarros. Os ingredientes são: tabaco, seda e fósforos.
 * 
 * O agente possui um suprimento ilimitado de todos os ingredientes, e cada fumante possui
 * um suprimento ilimitado de cada ingrediente - ou seja, um fumante possui tabaco infinito,
 * um possui seda infinita e o outro possui infinitos fósforos.
 *
 * O agente repetidamente escolhe dois ingredientes diferentes aleatoriamente e os coloca
 * à disposição dos fumantes. Dependendo dos ingredientes agora disponibilizados, o fumante
 * com o ingrediente complementar deve pegar ambos recursos e prosseguir (bolar o tabaco e
 * fumar). Se, por exemplo, o agente disponibilizar seda e fósforos, o fumante com o tabaco
 * deve pegá-los e prosseguir sua "execução".
 *
 * ANÁLISE DO PROBLEMA E SOLUÇÃO:
 *
 * O código em https://w3.cs.jmu.edu/kirkpams/OpenCSF/Books/csf/html/CigSmokers.html quebra
 * a condição de espera circular com o uso de sem_try_wait().
 * Foi essa solução que eu usei aqui.
 *
 * Uma solução possível é usar semáforos inicializados com pshared != 0 e alocá-los em alguma
 * memória compartilhada com mmap. Bem mais (desnecessariamente) complexo, mas é uma possibi-
 * lidade...
 *
 * 
 * TODO: ESSA IMPLEMENTAÇÃO DEIXA AS THREADS VOANDO NO ESPAÇO, SEM TERMINÁ-LAS!
 * MUDAR ISSO!!!
 */

#define NUM_FUMANTES 3  // Talvez seja necessário implementar para N fumantes.

#define TABACO  0
#define SEDA    1
#define FOSFORO 2 

/*
 * Implementação do agente e fumantes
*/


sem_t ingred_sem[3]; 
sem_t agente_sem; 


/*
 * Implementação das funções
*/

void bola_cigarro(int quem) {
	printf("\tFumante %d: bolando aquele cigarrinho gostoso...\n", quem);
}

void fuma_cigarro(int quem) {
	printf("\tFumante %d: fumando o cigarrinho que eu bolei, hmmmm...\n", quem);
}

void* agente(void* arg) {
	int ingredientes, retval;

	while (1) {

		retval = sem_wait(&agente_sem);
		if (retval != 0) {
			printf("sem_wait: erro esperando por agente_sem, saindo...\n");
			exit(-1);
		}
		
		ingredientes = rand() % 3 + 1;  // + 1 POIS ESTOU FAZENDO O ESQUEMA TABACO + SEDA, ETC.

		printf("AGENTE: liberando ingredientes...\n");
		switch(ingredientes) {
	
		case TABACO + SEDA:
			printf("AGENTE: liberando tabaco + seda...\n");
			sem_post(&ingred_sem[TABACO]);
			sem_post(&ingred_sem[SEDA]);
			break;
		case TABACO + FOSFORO:
			printf("AGENTE: liberando tabaco + fosforo...\n");
			sem_post(&ingred_sem[TABACO]);
			sem_post(&ingred_sem[FOSFORO]);
			break;
		case SEDA + FOSFORO:
			printf("AGENTE: liberando seda + fosforo...\n");
			sem_post(&ingred_sem[SEDA]);
			sem_post(&ingred_sem[FOSFORO]);
		 	break;

		}
	}
}

void* fumante_tabaco(void* id) {
	
	while (1) {
	
		sem_wait(&ingred_sem[SEDA]);
		if (sem_trywait(&ingred_sem[FOSFORO]) == 0) {
			bola_cigarro( (int)id );
			fuma_cigarro( (int)id );
			sem_post(&agente_sem);
		}
		else
			sem_post(&ingred_sem[SEDA]);

	}
}

void* fumante_seda(void* id) {
	
	while (1) {
	
		sem_wait(&ingred_sem[FOSFORO]);
		if (sem_trywait(&ingred_sem[TABACO]) == 0) {
			bola_cigarro( (int)id );
			fuma_cigarro( (int)id );
			sem_post(&agente_sem);
		}
		else
			sem_post(&ingred_sem[FOSFORO]);

	}
}

void* fumante_fosforo(void* id) {

	while (1) {
	
		sem_wait(&ingred_sem[TABACO]); 
		if (sem_trywait(&ingred_sem[SEDA]) == 0) {
			bola_cigarro( (int)id );
			fuma_cigarro( (int)id );
			sem_post(&agente_sem);
		}
		else
			sem_post(&ingred_sem[TABACO]);

	}
}


int main(int argc, char* argv[]) {
	int 	   i, retval_sem, retval_thr;
	int	   tabaco = TABACO, seda = SEDA, fosforo = FOSFORO;  // Pra passar valores pra pthread_create
	pthread_t  agente_thr, ftabaco_thr, fseda_thr, ffosforo_thr;

	/* Inicializando semáforos - agente e fumantes */
	retval_sem = sem_init(&agente_sem, 0, 1);
	if (retval_sem != 0) {
		printf("sem_init: erro inicializando agente, saindo...\n");
		exit(-1);
	}
	
	for (i = 0; i < NUM_FUMANTES; i++) {
		retval_sem = sem_init(&ingred_sem[i], 0, 0);
		if (retval_sem != 0) {
			printf("sem_init: erro inicializando ingredientes, saindo...\n");
			exit(-1);
		}
	}

	/* Criando threads para cada função */
	retval_thr = pthread_create(&agente_thr, NULL, agente, NULL);
	if (retval_thr != 0) {
		printf("pthread_create: erro criando agente, saindo...\n");
		exit(-1);
	}

	retval_thr = pthread_create(&ftabaco_thr, NULL, fumante_tabaco, (void*) tabaco);
	if (retval_thr != 0) {
		printf("pthread_create: erro criando fumante_tabaco, saindo...\n");
		exit(-1);
	}

	retval_thr = pthread_create(&fseda_thr, NULL, fumante_seda, (void*) seda);
	if (retval_thr != 0) {
		printf("pthread_create: erro criando fumante_seda, saindo...\n");
		exit(-1);
	}

	retval_thr = pthread_create(&ffosforo_thr, NULL, fumante_fosforo, (void*) fosforo);
	if (retval_thr != 0) {
		printf("pthread_create: erro criando fumante_fosforo, saindo...\n");
		exit(-1);
	}

	// DESTRUINDO TUDO QUE FOI INICIALIZADO
	pthread_join(agente_thr, NULL);
	pthread_join(ftabaco_thr, NULL);
	pthread_join(fseda_thr, NULL);
	pthread_join(ffosforo_thr, NULL);

	sem_destroy(&agente_sem);
	for (i = 0; i < NUM_FUMANTES; i++)
		sem_destroy(&ingred_sem[i]);
	
	return 0;
}

