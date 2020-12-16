/*
Autor: Giovani G Marciniak
GRR20182981

Arquivo: tempo.c
Modificado em 15/12/2020
Descrição:
Primeiro trabalho prático de Sistemas Distribuídos.
Implementação do algoritmo VRING no ambiente de simulação SMPL.
*/

#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

// Definição dos EVENTOS
#define TEST 1
#define FAULT 2
#define RECOVERY 3

// Outras definições
#define TEST_INTERVAL 30.0 			// define o intervalo de testes
#define MAX_TIME 600.0				// define o tempo máximo de execução da simulação

#define UNKNOWN -1

// Vamos definir o descritor do processo
typedef struct
{
	int id; 					// identificador de facility(recurso, no nosso caso processo) do SMPL
	int *state;					// vetor state[N] que indica estado dos processos
} TipoProceso;

TipoProceso *processo;

int main(int argc, char const *argv[])
{
	static int N, 				// número de processos
			token,				//indica o processo que está sendo executado
			event, r, i;

	static char fa_name[5];		//nome da facility

	if (argc != 2) {
		puts("Uso correto: ./tempo <número de processos>");
		exit(1);
	}

	N = atoi(argv[1]); 			//atoi converte string para número

	smpl(0, "Tarefa 4");
	reset();
	stream(1);					//temos 1 fluxo de simulação exclusiva, 1 processo por vez, 1 thread

	//inicialização dos processos
	processo = (TipoProceso*) malloc(sizeof(TipoProceso)*N);

	for (i=0;i<N;i++) {
		memset(fa_name,'\0',5);
		sprintf(fa_name, "%d", i);
		processo[i].id = facility(fa_name, 1);
		processo[i].state = (int*) malloc(sizeof(int)*N);
		processo[i].state[i] = 0; 	// define o proprio status do processo no vetor como Online (zero)
		for (int j = 1; j < N; j++) {
			processo[i].state[(i+j)%N] = UNKNOWN; // define os demais processos com o valor -1 (unknown)
		}
	}

	// Vamos fazer o escalonamento incial de eventos
	// nossos processos vão executar testes
	// o intervalo de testes vai ser de 30 unidades de tempo
	// a simulação começa no tempo 0(zero) e vamos escalonar o primeiro teste de todos os processos para o tempo 30
	printf("****************************************************\n");
	printf("ALUNO: Giovani G. Marciniak\n");
	printf("PROFESOR: Elias P. Duarte Jr.\n");
	printf("DISCIPLINA: Sistemas Distribuídos\n\n");
	printf("*********** SIMULAÇÃO DO ALGORITMO VRING ***********\n\n");	
	printf("QUANTIDADE DE PROCESSOS: %d processos\n", N);
	printf("INTERVALO DE TESTES: %4.1f unidades de tempo\n", TEST_INTERVAL);
	printf("TEMPO MÁXIMO DE EXECUÇÃO: %4.1f unidades de tempo\n", MAX_TIME);
	printf("****************************************************\n\n");
	printf("Início da execução...\n");

	for (i=0;i<N;i++) {
		schedule(TEST, TEST_INTERVAL, i);
	}

	// Cronograma dos eventos (escalonamento)
	schedule(FAULT, 31.0, 1); 			//o processo 1 falha no tempo 31
	schedule(FAULT, 91.0, 2);
	schedule(FAULT, 151.0,3);
	//schedule(RECOVERY, 151.0, 1); 		//o processo volta ao tempo 61

	int count_rodada = 1;

	int evento_atual = -1; //guarda o indice do processo que esta com um evento em andamento
	int evento_testes, evento_latencia;  //contadores de teste e latencia do evento e o valor do state[n]
	int evento_valor = -1;
	int evento_diagnosticado = 1; //boleano que define se um evento foi diagnosticado ou nao

	// Loop principal
	while (time() < MAX_TIME) {
		cause(&event, &token); //token retorna id do processo que tá executando agora

		if (token == 0) {
			printf("\n\nINÍCIO DA RODADA %d...\n", count_rodada);
			count_rodada++;
			if (!evento_diagnosticado) { 		//se o evento não estiver diagnosticado, ou seja, existir um evento ativo
				evento_diagnosticado = 1; 		// considera que será diagnosticado
				for (i = 1; i < N; i++) {
					if (processo[(evento_atual+i)%N].state[evento_atual] != evento_valor) { // define os demais processos com o valor -1 (unknown)
						evento_diagnosticado = 0;
						evento_latencia++;
						break;
					}
				}

				if (evento_diagnosticado) {
					printf("\nEVENTO DIAGNOSTICADO!!!\n");
					printf("PROCESSO: %d\nEVENTO: ", evento_atual);
					if (evento_valor%2) printf("Falhou\n");
					else printf("Recuperou\n");
					printf("LATÊNCIA: %d rodadas\nTESTES EXECUTADOS: %d testes\n\n", evento_latencia, evento_testes);
					evento_valor = -1;
				}

			}
		}

		switch(event) {
			case TEST:
				if ((status(processo[token].id)) != 0) break;	//se o processo está falho, não testará
				
				// O processo não falho, então testará outro
				for (i = 1; i < N; i++)
				{
					int i_next = (token+i)%N; 				//i_next corresponde ao proximo index que será testado

					if (status(processo[i_next].id) != 0) {
						printf("O processo %d testou o processo %d FALHO no tempo %4.1f\n", token, i_next, time());
						// caso o estado do processo testado esteja diferente do vetor state, corrige
						if (!processo[token].state[i_next]%2)  //se o valor no vetor state for par, incrementa o valor
							processo[token].state[i_next]++;

						if (i == (N-1)) {
							printf("\nATENÇÃO: Todos os processos estão falhos, exceto o processo %d\nFim da execução.\n", token);
							exit(1);
						}

						//verifica se está em processo de diagnostico
						if (!evento_diagnosticado) {
							if (evento_valor < 0) evento_valor = processo[token].state[i_next]; //se for o primeiro a encontrar o erro
							evento_testes++;
						}
					}
					else {
						printf("O processo %d testou o processo %d CORRETO no tempo %4.1f\n", token, (token+i)%N, time());
						// caso o estado do processo testado esteja diferente do vetor state, corrige
						if (processo[token].state[i_next]%2)  //se o valor no vetor state for ímpar, incrementa o valor
							processo[token].state[i_next]++;

						if (!evento_diagnosticado) {
							if (evento_valor < 0) evento_valor = processo[token].state[i_next]; //se for o primeiro a encontrar o erro
							evento_testes++;
						}

						// vai copiar o vetor state[N] dos que ele não testou
						int j = (i_next+1)%N;
						while (j != token) {
							if (processo[token].state[j] < processo[i_next].state[j])
							processo[token].state[j] = processo[i_next].state[j];
							j = (j+1)%N;
						}

						//imprime vetor state
						printf("> Vetor state do processo %d\n(processo,status) = [", token);
						for (i=0; i<N; i++) {
							printf(" (%d,%d)",i,processo[token].state[i]);
						}
						printf(" ]\n");

						break;
					}
				}

				printf("----------------------------------------------------\n");
				schedule(TEST, TEST_INTERVAL, token);
				break;

			case FAULT:
				r = request(processo[token].id, token, 0);
				if (r != 0) {
					puts("Não foi possível falhar o processo.");
					exit(1);
				}

				//Limpa a memória do vetor do processo que falhou/crashou
				processo[token].state[token] = 0; 	// define o proprio status do processo no vetor como Online (zero)
				for (int j = 1; j < N; j++) {
					processo[token].state[(token+j)%N] = UNKNOWN; // define os demais processos com o valor -1 (unknown)
				}

				printf("\nO processo %d falhou no tempo %4.1f\n", token, time());
				
				// Reseta os valores de diagnóstico
				evento_atual = token;
				evento_diagnosticado = 0;
				evento_testes = 0;
				evento_latencia = 0;

				break;

			case RECOVERY:
				release(processo[token].id, token);
				printf("\nO processo %d recuperou no tempo %4.1f\n", token, time());
				schedule(TEST, TEST_INTERVAL, token);

				// Reseta os valores de diagnóstico
				evento_atual = token;
				evento_diagnosticado = 0;
				evento_testes = 0;
				evento_latencia = 0;

				break;
		}
	}
	printf("\nFim da execução.\n");
	return(0);
}