/*
Autor: Giovani G Marciniak
GRR20182981

Arquivo: vcube.c
Modificado em 01/02/2021
Descrição:
Segundo trabalho prático da disciplina Sistemas Distribuídos.
Implementação do algoritmo VCUBE V2 no ambiente de simulação SMPL.
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
#define MAX_TIME 274.0				// define o tempo máximo de execução da simulação
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

// faz a potenciação de um inteiro
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

	smpl(0, "VCube Ver2");
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
	printf("*********** SIMULAÇÃO DO ALGORITMO VCUBE V2 ***********\n\n");	
	printf("QUANTIDADE DE PROCESSOS: %d processos\n", N);
	printf("INTERVALO DE TESTES: %4.1f unidades de tempo\n", TEST_INTERVAL);
	printf("TEMPO MÁXIMO DE EXECUÇÃO: %4.1f unidades de tempo\n", MAX_TIME);
	printf("****************************************************\n\n");
	printf("Início da execução...\n");

	for (i=0;i<N;i++) {
		schedule(TEST, TEST_INTERVAL+i, i);
	}

	// Cronograma dos eventos (escalonamento)
	//schedule(FAULT, 130.0, 5); 			//o processo 5 falha no tempo 130.00
	//schedule(RECOVERY, 392.0, 5); 		//o processo volta ao tempo 392.00

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
	while (N > pow_int(2, max_s)) max_s++;

	// Loop principal
	while (time() < MAX_TIME) {
		cause(&event, &token); //token retorna id do processo que tá executando agora

		if (token == 0) {
			printf("\n\nINÍCIO DA RODADA %d...\n", count_rodada);
			printf("----------------------------------------------------\n");
			count_rodada++;

			if (!evento_diagnosticado) evento_latencia++; 

			if (!evento_diagnosticado) { 		//se o evento não estiver diagnosticado, ou seja, existir um evento ativo
				evento_diagnosticado = 1; 		// considera que será diagnosticado
				
				for (i = 0; i < N; i++) {
					// se não for falho ou o atual
					if ( (status(processo[i].id) == 0) && (i != evento_atual) ) { 
						if (processo[i].state[evento_atual] != evento_valor) {
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
				printf("Processo: %d  s: %d\n",token, processo[token].s);

				// Procura quem é o testador de cada processo:
				for (i = 0; i < N; i++)
				{
					//printf("i por fora: %d\n", i);
					for (int j = 1; j <= tamanho_cluster; j++)
					{
						i_next = call_cisj(i, processo[token].s, j);
						if (i_next != token) {
							if (!(processo[token].state[i_next]%2) || (processo[token].state[i_next] == UNKNOWN)) break;
							// se o processo for par no vetor state, o processo atual não é o responsavel por testar
						}

						// Portanto, se i_next == token, vai realizar o teste em i
						else
						{
							if (status(processo[i].id) != 0)
							{
								printf("Testado: %d    Status: FALHO      Tempo: %4.1f\n", i, time());
								// caso o estado do processo testado esteja diferente do vetor state, corrige
								if (!processo[token].state[i]%2)
									processo[token].state[i]++;
							
								// vai cair nesse caso se for um processo recuperado
								if (processo[token].state[i_next] == UNKNOWN)
									processo[token].state[i_next] = 1;

								//verifica se está em processo de diagnostico
								if (!evento_diagnosticado) {
									evento_testes++;
									//se for o primeiro a encontrar o erro
									if ((i == evento_atual)&&(evento_valor < 0))
										evento_valor = processo[token].state[i]; 
								}

								break;
							}

							else
							{
								printf("Testado: %d    Status: CORRETO    Tempo: %4.1f\n", i, time());
								// caso o estado do processo testado esteja diferente do vetor state, corrige
								if (processo[token].state[i]%2)
									processo[token].state[i]++;

								if (!evento_diagnosticado) {
									evento_testes++;
									//se for o primeiro a encontrar o erro
									if ((i == evento_atual)&&(evento_valor < 0))
										evento_valor = processo[token].state[i]; 
								}

								// vai copiar o vetor state[N] dos que ele não testou (que tem valor maior)
								for (j = 0; j < N; j++)
								{
									if(processo[token].state[j] < processo[i].state[j]) {
										processo[token].state[j] = processo[i].state[j];
										//printf("-- O processo %d obteve informação do processo %d através do processo %d\n", token, j, i);
									}
								}
								break;
							}
						}
					}
				}

				//imprime vetor state
				printf("state[%d] = ", token);
						for (i=0; i<N; i++) {
							printf("%d ",processo[token].state[i]);
						}
				printf("\n\n");
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

				printf("O processo %d falhou no tempo %4.1f\n\n", token, time());
				
				// Reseta os valores de diagnóstico
				evento_atual = token;
				evento_diagnosticado = 0;
				evento_testes = 0;
				evento_latencia = 0;

				break;

			case RECOVERY:
				release(processo[token].id, token);
				printf("O processo %d recuperou no tempo %4.1f\n\n", token, time());
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
