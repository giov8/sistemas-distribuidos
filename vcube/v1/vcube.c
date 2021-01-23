/*
Autor: Giovani G Marciniak
GRR20182981

Arquivo: vcube.c
Modificado em 21/01/2021
Descrição:
Segundo trabalho prático da disciplina Sistemas Distribuídos.
Implementação do algoritmo VCUBE V1 no ambiente de simulação SMPL.
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
#define MAX_TIME 500.0				// define o tempo máximo de execução da simulação
#define UNKNOWN -1

// Vamos definir o descritor do processo
typedef struct
{
	int id; 					// identificador de facility(recurso, no nosso caso processo) do SMPL
	int *state;					// vetor state[N] que indica estado dos processos
	int s;						// representa o cluster atual que vai ser testado pelo processo
} TipoProceso;

TipoProceso *processo;

// função que vai chamar o programa (compilado) cisj.c e devolver o int do processo que será testado.
int call_cisj (int i, int s, int j)
{
	FILE *fp;
	char path[1035];
  	char str[64];
  	int value = 0;

	sprintf(str, "./cisj %d %d %d", i, s, j);
	//printf("%s", str);
	fp = popen(str, "r");
	if (fp == NULL) {
    	printf("Falha na função C(i, s, j)\n" );
    exit(1);
  }

 	// Converte a saída para inteiro
	while (fgets(path, sizeof(path), fp) != NULL) {
		value = atoi(path);
  	}

  //Fecha o pipeline
  pclose(fp);
  return value;
}

int pow_int(int base, int exp)
{
    int resultado = 1;
    while (exp) {
        if (exp % 2)
           resultado *= base;
        exp /= 2;
        base *= base;
    }
    return resultado;
}


int main(int argc, char const *argv[])
{
	static int N, 				// número de processos
			token,				//indica o processo que está sendo executado
			event, r, i;

	static char fa_name[5];		//nome da facility

	if (argc != 2) {
		puts("Uso correto: ./vcube <número de processos>");
		exit(1);
	}

	N = atoi(argv[1]); 			//atoi converte string para número

	smpl(0, "VCube Ver1");
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
		processo[i].s = 0; 			// define o cluster inicial
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
	printf("*********** SIMULAÇÃO DO ALGORITMO VCUBE V1 ***********\n\n");	
	printf("QUANTIDADE DE PROCESSOS: %d processos\n", N);
	printf("INTERVALO DE TESTES: %4.1f unidades de tempo\n", TEST_INTERVAL);
	printf("TEMPO MÁXIMO DE EXECUÇÃO: %4.1f unidades de tempo\n", MAX_TIME);
	printf("****************************************************\n\n");
	printf("Início da execução...\n");

	for (i=0;i<N;i++) {
		schedule(TEST, TEST_INTERVAL, i);
	}

	// Cronograma dos eventos (escalonamento)
	//schedule(FAULT, 1.0, 1); 			//o processo 1 falha no tempo 31
	//schedule(RECOVERY, 1.0, 1); 		//o processo volta ao tempo 61
	//schedule(FAULT, 151.0, 2);
	//schedule(FAULT, 300.0,3);

	int count_rodada = 1;
	int tamanho_cluster = 0;
	int i_next = 0; // será usado para guardar o proximo indice 

	// variáveis relativas ao diagnóstico de eventos 
	int evento_atual = -1; //guarda o indice do processo que esta com um evento em andamento
	int evento_testes, evento_latencia;  //contadores de teste e latencia do evento e o valor do state[n]
	int evento_valor = -1;
	int evento_diagnosticado = 1; //boleano que define se um evento foi diagnosticado ou nao

	// Cálculo do número máximo de clusters
	static int max_s = 1;

	while (N > pow_int(2, max_s)) {
		//printf("pow int: %d\n", pow_int(2, max_s+1));
		max_s++;
	}

	// Loop principal
	while (time() < MAX_TIME) {
		cause(&event, &token); //token retorna id do processo que tá executando agora

		if (token == 0) {
			printf("\n\nINÍCIO DA RODADA %d...\n", count_rodada);
			count_rodada++;
			if (!evento_diagnosticado) { 		//se o evento não estiver diagnosticado, ou seja, existir um evento ativo
				evento_diagnosticado = 1; 		// considera que será diagnosticado
				
				for (i = 0; i < N; i++) {
					if ( (status(processo[i].id) == 0) && (i != evento_atual) ) { // se não for falho ou o atual
						if (processo[i].state[evento_atual] != evento_valor) { // define os demais processos com o valor -1 (unknown)
							evento_diagnosticado = 0;
							evento_latencia++;
							break;
						}
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
				
				// O processo não está falho, então testará outro

				processo[token].s = (processo[token].s % max_s) + 1;
 				tamanho_cluster = pow_int(2, processo[token].s);
				printf("**Processo: %d  s: %d**\n",token, processo[token].s);

				for (i = 1; i <= tamanho_cluster; i++)
				{

					//i_next corresponde ao proximo index que será testado
					//do {
						i_next = call_cisj(token, processo[token].s, i);
					//} while (i_next < N)

					// essa linha serve para que o número de processos não precise ser na base 2:
					//if (i_next > N)
					//	i_next = 


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
						printf("O processo %d testou o processo %d CORRETO no tempo %4.1f\n", token, i_next, time());
						// caso o estado do processo testado esteja diferente do vetor state, corrige
						if (processo[token].state[i_next]%2)  //se o valor no vetor state for ímpar, incrementa o valor
							processo[token].state[i_next]++;

						if (!evento_diagnosticado) {
							if (evento_valor < 0) evento_valor = processo[token].state[i_next]; //se for o primeiro a encontrar o erro
							evento_testes++;
						}

						// vai copiar o vetor state[N] dos que ele não testou **apenas no cluster**
						int i_correto = i_next; // salva o que foi testado correto para copiar

						for (i = i+1; i < tamanho_cluster; i++) {
							i_next = call_cisj(token, processo[token].s, i);

							// essa linha serve para que o número de processos não precise ser na base 2:
							if (i_next > N) break;

							if(processo[token].state[i_next] < processo[i_correto].state[i_next]) {
								processo[token].state[i_next] = processo[i_correto].state[i_next];
								printf("Processo %d obteve informação do processo %d através do processo %d\n", token, i_next, i_correto);
							}

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
				processo[token].s = 0;
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
