/*
Autor: Giovani G Marciniak
GRR20182981

Arquivo: tempo.c
Modificado em 13/12/2020
Descrição:
Implementação relativa a Tarefa 4 do primeiro trabalho prático de Sistemas Distribuídos.
Os processos agora compartilharão as informações novas do vetor state[N].
*/

#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

// Definição dos EVENTOS
#define TEST 1
#define FAULT 2
#define RECOVERY 3

// Outras definições
#define TEST_INTERVAL 30.0
#define MAX_TIME 600.0

#define UNKNOWN -1
#define ONLINE 0
#define OFFLINE 1

// Vamos definir o descritor do processo
typedef struct
{
	int id; 			// identificador de facility(recurso, no nosso caso processo) do SMPL
	int *state;			// vetor state[N] que indica estado dos processos
	// outras variáveis locais dos processos são declaradas aqui
} TipoProceso;

TipoProceso *processo;

int main(int argc, char const *argv[])
{
	static int N, 		// número de processos
			token,		//indica o processo que está sendo executado
			event, r, i;

	static char fa_name[5];		//nome da facility

	if (argc != 2) {
		puts("Uso correto: ./tempo <número de processos>");
		exit(1);
	}

	N = atoi(argv[1]); //atoi converte para número

	smpl(0, "Tarefa 4");
	reset();
	stream(1);			// temos 1 fluxo de simulação exclusiva, 1 processo por fez, 1 thread

	//inicialização dos processos
	processo = (TipoProceso*) malloc(sizeof(TipoProceso)*N);

	for (i=0;i<N;i++) {
		memset(fa_name,'\0',5);
		sprintf(fa_name, "%d", i);
		processo[i].id = facility(fa_name, 1);
		processo[i].state = (int*) malloc(sizeof(int)*N);
		processo[i].state[i] = ONLINE; 	// define o proprio status do processo no vetor como Online (zero)
		for (int j = 1; j < N; j++) {
			processo[i].state[(i+j)%N] = UNKNOWN; // define os demais processos com o valor -1 (unknown)
		}
	}

	// Vamos fazer o escalonamento incial de eventos
	// nossos processos vão executar testes
	// o intervalo de testes vai ser de 30 unidades de tempo
	// a simulação começa no tempo 0(zero) e vamos escalonar o primeiro teste de todos os processos para o tempo 30
	printf("****************************************************\n");
	printf("QUANTIDADE DE PROCESSOS: %d processos\n", N);
	printf("INTERVALO DE TESTES: %4.1f unidades de tempo\n", TEST_INTERVAL);
	printf("TEMPO MÁXIMO DE EXECUÇÃO: %4.1f unidades de tempo\n", MAX_TIME);
	printf("****************************************************\n");

	for (i=0;i<N;i++) {
		schedule(TEST, TEST_INTERVAL, i);
	}

	// Cronograma dos eventos (escalonamento)
	schedule(FAULT, 31.0, 1); 			//o processo 1 falha no tempo 31
	schedule(RECOVERY, 61.0, 1); 		//o processo volta ao tempo 61

	// Loop principal
	while (time() < MAX_TIME) {
		cause(&event, &token); //token retorna id do processo que tá executando agora

		switch(event) {
			case TEST:
				if ((status(processo[token].id)) != 0) break;	//se o processo está falho, não testará
				
				// O nodo não falho testa o próximo:
				for (i = 1; i < N; i++)
				{
					int i_next = (token+i)%N;
					if (status(processo[i_next].id) != 0) {
						printf("O processo %d testou o processo %d FALHO no tempo %4.1f\n", token, i_next, time());
						// caso o estado do nodo testado esteja diferente do vetor state, corrige
						if (processo[token].state[i_next] != OFFLINE)
							processo[token].state[i_next] = OFFLINE;
					}
					else {
						printf("O processo %d testou o processo %d CORRETO no tempo %4.1f\n", token, (token+i)%N, time());
						// caso o estado do nodo testado esteja diferente do vetor state, corrige
						if (processo[token].state[i_next] != ONLINE)
							processo[token].state[i_next] = ONLINE;

						// vai copiar o vetor state[N] dos que ele não testou
						int j = (i_next+1)%N;
						while (j != token) {
							processo[token].state[j] = processo[i_next].state[j];
							j = (j+1)%N;

						}

						//imprime vetor state
						printf("# VETOR STATE DO PROCESSO %d #\n(processo,status) = [", token);
						for (i=0;i<N;i++) {
							printf(" (%d,%d)",i,processo[token].state[i]);
						}
						printf(" ]\n");

						break;
					}
				}

				//if (i == N) {
				//	printf("ATENÇÃO: Todos os processos estão falhos, exceto o processo %d\n", token);
					//exit(1);  //???
				//}

				printf("----------------------------------------------------\n");
				schedule(TEST, TEST_INTERVAL, token);
				break;

			case FAULT:
				r = request(processo[token].id, token, 0);
				if (r != 0) {
					puts("Não foi possível falhar o processo.");
					exit(1);
				}
				printf("O processo %d falhou no tempo %4.1f\n", token, time());
				break;

			case RECOVERY:
				release(processo[token].id, token);
				printf("O processo %d recuperou no tempo %4.1f\n", token, time());
				schedule(TEST, TEST_INTERVAL, token);
				break;
		}
	}
	return(0);
}

/* tarefa 1: fazer cada um dos processos testar o seguinte no anel; testar com a função status do SMPL e imprimir (printf)
o resultado de cada teste.
Por exemplo: "O processo 1 testou o processo 2 correto"

tarefa 2: cada processo correto executa testes até achar outro processo correto. Imprimir os testes e resultados.

tarefa 3: cada processo mantém localmente um vetor State[N]
inicializa o State[N] com -1 para as N entraas indicando "unknown"
atualiza as entradas correspondentes do State[] ao testar

tarefa 4: quando um processo correto testa outro processo correto obtém as informações de diagnóstico
do processo testado sobre todos os processos do sistema exceto aqueles que testou nesta rodada*/





