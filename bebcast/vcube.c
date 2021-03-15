/*
Autor: Giovani G Marciniak
GRR20182981

Arquivo: vcube.c
Modificado em XX/03/2021 ####################################################
Descrição:
Terceiro trabalho prático da disciplina Sistemas Distribuídos.
Modificação do algoritmo VCUBE simulando um Best-Effort Broadcast em ambiente SMPL.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smpl.h"


// Definição dos EVENTOS
#define TEST 1
#define FAULT 2
#define RECOVERY 3


// Outras definições
#define TEST_INTERVAL 10.0 			// define o intervalo de testes
#define MAX_TIME 249.0				// define o tempo máximo de execução da simulação
#define UNKNOWN -1


// Descritor do tipo processo
typedef struct
{
	int id; 					// identificador de facility(recurso, no nosso caso processo) do SMPL
	int *state;					// vetor state[N] que indica estado dos processos
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


// função que faz a potenciação de um inteiro
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

//a qual cluster de j o processo i pertence?
int cluster(int i, int j)
{
	int k;
	int s = 0;

	while(1) {
		int smax = pow_int(2,s);
		s++;
		for (k = 1; k <= smax; k++) {
			if (call_cisj(j,s,k) == i) {
				return s;
			}
		}
	}
}

int ff_neighbour(int i, int s)
{
	
}

// ### Início Função Main ###
int main(int argc, char const *argv[])
{
	static int N, 				// número de processos
			token,				//indica o processo que está sendo executado
			event, r, i, j, origem;

	static char fa_name[5];		//nome da facility

	if (argc < 5) {
		puts("Uso correto:");
		puts("MODO PADRÃO: ./vcube <processo fonte> <tempo inicio broadcast> <nº total de nodos> -l <nodo falho 1> <tempo falha nodo1> <nodo falho n> <tempo falha nodon>");
		puts("MODO ALEATÓRIO: ./vcube <processo fonte> <tempo inicio broadcast> <nº total de nodos> -a <nº de nodos que falharão> <semente>");
		exit(1);
	}

	if ( (!strcmp(argv[4],"-l")) && (!strcmp(argv[4],"-a")) ){
		puts("Opção não existe. Tente \"-a\" ou \"-l\"");
		exit(1);
	}

	origem = atoi(argv[1]); 			//atoi converte string para número
	float inicioBEBCast = atof(argv[2]);
	N = atoi(argv[3]);

	printf("****************************************************\n");
	printf("ALUNO: Giovani G. Marciniak\n");
	printf("PROFESOR: Elias P. Duarte Jr.\n");
	printf("DISCIPLINA: Sistemas Distribuídos\n\n");
	printf("********** SIMULAÇÃO DO ALGORITMO BEST-EFFORT *********\n\n");
	printf("************* SOBRE O VCUBE V1 MODIFICADO *************\n\n");		
	printf("QUANTIDADE DE PROCESSOS: %d processos\n", N);
	printf("INTERVALO DE TESTES: %4.1f unidades de tempo\n", TEST_INTERVAL);
	printf("TEMPO MÁXIMO DE EXECUÇÃO: %4.1f unidades de tempo\n", MAX_TIME);
	printf("PROCESSO ORIGEM DO BROADCAST: %d\n", origem);
	printf("****************************************************\n");

	smpl(0, "VCube Ver1 + bebcast");
	reset();
	stream(1);					//temos 1 fluxo de simulação exclusiva, 1 processo por vez, 1 thread

	//inicialização dos processos
	processo = (TipoProceso*) malloc(sizeof(TipoProceso)*N);

	for (i=0; i<N; i++) {
		memset(fa_name,'\0',5);
		sprintf(fa_name, "%d", i);
		processo[i].id = facility(fa_name, 1);
		processo[i].state = (int*) malloc(sizeof(int)*N);
		processo[i].state[i] = 0; 	// define o proprio status do processo no vetor como Online (zero)
		for (j=1; j<N; j++) {
			processo[i].state[(i+j)%N] = UNKNOWN; // define os demais processos com o valor -1 (unknown)
		}
	}

	printf("DEFINIÇÕES:\n\n");

	//MODO PADRÃO: ./vcube <processo fonte> <tempo inicio broadcast> <nº total de nodos> -l <nodo falho 1> <tempo falha nodo1> <nodo falho n> <tempo falha nodon>
	// Escalonamento padrão (com lista de nodos)
	if (!strcmp(argv[4],"-l")) {
		printf("MODO: PADRÃO\n");
		for (i=5; i < argc; i++) {
			printf("PROCESSO: %d FALHARÁ NO TEMPO %4.1f\n", atoi(argv[i]), atof(argv[i+1]));
			schedule(FAULT, atof(argv[i+1]), atoi(argv[i]));	// programa uma falha do processo da linha de comando, no tempo especificado
			i++;
		}
	}

	//MODO ALEATÓRIO: ./vcube <processo fonte> <tempo inicio broadcast> <nº total de nodos> -a <nº de nodos que falharão> <semente>
	// Escalonamento aleatório
	if (!strcmp(argv[4],"-a")) {
		printf("MODO: ALEATÓRIO\n");
		printf("SEMENTE ALEATÓRIA: %s\n", argv[6]);

		srand(atoi(argv[6])); // define a semente da linha de comando

		for (i=0; i < atoi(argv[5]); i++) {
			int processoRand = rand()%N;
			float tempoRand = rand()%(int)MAX_TIME;
			printf("PROCESSO: %d FALHARÁ NO TEMPO %4.1f\n", processoRand, tempoRand);
			schedule(FAULT, tempoRand, processoRand);
		}
	}
	printf("****************************************************\n");

	printf("CLUSTER %d\n", cluster(3,8));


/*	for (i=0;i<N;i++) {
		schedule(TEST, TEST_INTERVAL+i, i);
	}

	// Cronograma dos eventos (escalonamento)
	//schedule(FAULT, 130.0, 5); 			//o processo 5 falha no tempo 130.00
	//schedule(RECOVERY, 392.0, 5); 		//o processo volta ao tempo 392.00

	printf("****************************************************\n\n");
	printf("Início da execução...\n");


	int count_rodada = 1;
	int tamanho_cluster = 0;
	int i_next = 0; // será usado para guardar o proximo indice 

	// variáveis relativas ao diagnóstico de eventos 
	int evento_atual = -1; //guarda o indice do processo que esta com um evento em andamento
	int evento_testes, evento_latencia;  //contadores de teste e latencia do evento e o valor do state[n]
	int evento_valor = UNKNOWN;
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
					evento_valor = UNKNOWN;
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

				for (i = 1; i <= tamanho_cluster; i++)
				{
					//i_next corresponde ao proximo index que será testado
					i_next = call_cisj(token, processo[token].s, i);

					// serve para que o número de processos não precise ser na base 2
					if (i_next >= N) break;

					if (status(processo[i_next].id) != 0) {
						printf("Testado: %d    Status: FALHO    Tempo: %4.1f\n", i_next, time());
						// caso o estado do processo testado esteja diferente do vetor state, corrige
						if (!processo[token].state[i_next]%2)  //se o valor no vetor state for par, incrementa o valor
							processo[token].state[i_next]++;

						// vai cair nesse caso se for um processo recuperado
						if (processo[token].state[i_next] == UNKNOWN)
							processo[token].state[i_next] = 1;

						//verifica se está em processo de diagnostico
						if (!evento_diagnosticado) {
							evento_testes++;
							//se for o primeiro a encontrar o erro
							if ((i_next == evento_atual)&&(evento_valor < 0))
								evento_valor = processo[token].state[i_next]; 
						}
					}

					else {
						printf("Testado: %d    Status: CORRETO    Tempo: %4.1f\n", i_next, time());
						// caso o estado do processo testado esteja diferente do vetor state, corrige
						if (processo[token].state[i_next]%2)  //se o valor no vetor state for ímpar, incrementa o valor
							processo[token].state[i_next]++;

						if (!evento_diagnosticado) {
							evento_testes++;
							//se for o primeiro a encontrar o erro
							if ((i_next == evento_atual)&&(evento_valor < 0))
								evento_valor = processo[token].state[i_next]; 
						}

						// vai copiar o vetor state[N] dos que ele não testou **apenas no cluster**
						int i_correto = i_next; // salva o que foi testado correto para copiar

						for (i = i+1; i < tamanho_cluster; i++) {
							i_next = call_cisj(token, processo[token].s, i);

							// essa linha serve para que o número de processos não precise ser na base 2:
							if (i_next > N) break;

							if(processo[token].state[i_next] < processo[i_correto].state[i_next]) {
								processo[token].state[i_next] = processo[i_correto].state[i_next];
								//printf("-- O processo %d obteve informação do processo %d através do processo %d\n", token, i_next, i_correto);
							}

						}
						break;
					}

				}

				//imprime vetor state
				printf("state[%d] = ", token);
				for (j=0; j<N; j++) {
					printf("%d ",processo[token].state[j]);
				}
				printf("\n");
				printf("\n");
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
				for (j=1; j<N; j++) {
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
	}*/
	printf("\nFim da execução.\n");
	return(0);
}
