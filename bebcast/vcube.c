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
#include "token_agrupada.h"


// Definição dos EVENTOS
#define TEST 1
#define FAULT 2
#define RECOVERY 3
#define BEBCAST 4
#define RECIEVE 5


// Outras definições
#define TEST_INTERVAL 10.0 			// define o intervalo de testes
#define MAX_TIME 249.0				// define o tempo máximo de execução da simulação
#define MSG_TRAVEL_TIME 5.0			// define quantas unidades de tempo leva para mensagem enviada chegar
#define UNKNOWN -1


// ### DEFINIÇÕES  ###
enum tipodemensagem {MSG, ACK, HEAD};

typedef struct
{
	unsigned int id;				// id da mensagem 
	int origem; 					// identificador da origem da mensagem
	enum tipodemensagem tipo;		// define o tipo da mensagem, ack ou normal
	int remetente;					// identifica o remetente da mensagem, note que pode ser diferente ou não da origem
	struct TipoMensagem *prox;		// ponteiro para o proximo nodo
} TipoMensagem;
TipoMensagem *mensagem;

typedef struct
{
	int id; 					// identificador de facility(recurso, no nosso caso processo) do SMPL
	int *state;					// vetor state[N] que indica estado dos processos
	int *pendingACK;			// vetor pendingACK guarda se existe um ack pendende de i..N
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

/* ### FUNÇÕES BEBCAST ### */

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

int ff_neighbour(int i, int s, int *state)
{
	int vizinho;

	for (int j = 1; j <= pow_int(2,s-1); j++) {
		vizinho = call_cisj(i,s,j);
		if (!(state[vizinho]%2)) { //se o vizinho não estiver falho
			return vizinho;
		}
	}
	return (-1);
}

//Usada para montar o ID das mensagens
unsigned concatenate(unsigned x, unsigned y)
{
    unsigned pow = 10;
    while(y >= pow)
        pow *= 10;
    return x * pow + y;        
}

TipoMensagem *getMsg(TipoMensagem *lista, unsigned int id) {
	while (lista->prox != NULL) {
		lista = lista->prox;
		if (lista->id == id) {
			return lista;
		}
	}
	printf("Não foi possível encontar a mensagem %d.\n", id);
}

unsigned newMsg(int origem, int remetente, enum tipodemensagem tipo, TipoMensagem *lista)
{
	// gera ID
	unsigned int id = concatenate((int)time()+1.0,remetente);

	// percorre até final da lista
	while (lista->prox != NULL) lista = lista->prox;

	lista->prox = (TipoMensagem*) malloc(sizeof(TipoMensagem));
	lista = lista->prox;

	// Inicializa a mensagem
	lista->origem = origem;
	lista->remetente = remetente;
	lista->tipo = tipo;
	lista->prox = NULL;
	lista->id = id;

	return id;
}

void sendMsg(int destinatario, unsigned int id) 
{
	schedule(RECIEVE, MSG_TRAVEL_TIME, AGRUPAR_TOKEN2(destinatario, id));
}

void recieveMsg(int token, int *destinatario, unsigned int *id)
{
	SEPARAR_TOKEN2(token, *destinatario, *id);
}

void freeMsg(TipoMensagem *lista, unsigned int id) {
	TipoMensagem *anterior = lista;
	lista = lista->prox;

	if (lista == NULL) {
		printf("Falha ao liberar mensagem, lista vazia!\n");
	}

	while (lista->prox != NULL) {
		if (lista->id == id) {
			anterior->prox = lista->prox;
			free(lista);
			return;
		}
		anterior = lista;
		lista = lista->prox;
	}

	if (lista->id == id) {
		free(lista);
		anterior->prox = NULL;
		return;
	}

	printf("Falha ao liberar a mensagem %u\n", id);
}

int vazio(int *vetor, int N)
{
	for (int i = 0; i < N; i++)
		if (vetor[i] >= 0) return 0;

	return 1;
}

// ### Início Função Main ###
int main(int argc, char const *argv[])
{
	static int N, 				// número de processos
			token,				//indica o processo que está sendo executado
			event, r, i, j, origem, vizinho, s, msg_id;

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
		processo[i].pendingACK = (int*) malloc(sizeof(int)*N);

		for (j=0; j<N; j++) {
			processo[i].state[(i+j)%N] = 0; // define todos os processos como corretos
			processo[i].pendingACK[(i+j)%N] = -1;
		}
	}

	printf("DEFINIÇÕES:\n\n");

	//MODO PADRÃO: ./vcube <processo fonte> <tempo inicio broadcast> <nº total de nodos> -l <nodo falho 1> <tempo falha nodo1> <nodo falho n> <tempo falha nodon>
	// Escalonamento padrão (com lista de nodos)
	if (!strcmp(argv[4],"-l")) {
		printf("MODO: PADRÃO\n");
		for (i=5; i < argc; i++) {
			int processoFalho = atoi(argv[i]);
			float tempoFalho = atof(argv[i+1]);
			printf("PROCESSO: %d FALHARÁ NO TEMPO %4.1f\n", processoFalho, tempoFalho);
			schedule(FAULT, tempoFalho, processoFalho);	// programa uma falha do processo da linha de comando, no tempo especificado
			i++; // incremeto para o próximo processo
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

	schedule(BEBCAST, inicioBEBCast, origem);

	// Cronograma dos eventos (escalonamento)
	//schedule(FAULT, 130.0, 5); 			//o processo 5 falha no tempo 130.00
	//schedule(RECOVERY, 392.0, 5); 		//o processo volta ao tempo 392.00

	printf("****************************************************\n\n");
	printf("Início da execução...\n");

	for (i=0;i<N;i++) {
	schedule(TEST, TEST_INTERVAL+i, i);
	}

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

	// Criação da lista que vai guardar as mensagens
	TipoMensagem *msg_list = (TipoMensagem*) malloc(sizeof(TipoMensagem));
	msg_list->id = 0;
	msg_list->origem = msg_list->remetente = -1;
	msg_list->prox = NULL;
	msg_list->tipo = HEAD;
	
	// Loop principal
	while (time() < MAX_TIME) {
		cause(&event, &token); //token retorna id do processo que tá executando agora

		if (token == 0) {
			//printf("\n\nINÍCIO DA RODADA %d...\n", count_rodada);
			//printf("----------------------------------------------------\n");
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
				//processo[token].s = (processo[token].s % max_s) + 1;

				for (s = 1; s <= max_s; s++)
				{
					//printf("Processo: %d  s: %d\n",token, s);
 					tamanho_cluster = pow_int(2, s-1);
					
					for (i = 1; i <= tamanho_cluster; i++)
					{
						//i_next corresponde ao proximo index que será testado
						i_next = call_cisj(token, s, i);

						if (status(processo[i_next].id) != 0) {
							//printf("Testado: %d    Status: FALHO    Tempo: %4.1f\n", i_next, time());
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
							//printf("Testado: %d    Status: CORRETO    Tempo: %4.1f\n", i_next, time());
							// caso o estado do processo testado esteja diferente do vetor state, corrige
							if (processo[token].state[i_next]%2)  //se o valor no vetor state for ímpar, incrementa o valor
								processo[token].state[i_next]++;

							if (!evento_diagnosticado) {
								evento_testes++;
								//se for o primeiro a encontrar o erro
								if ((i_next == evento_atual)&&(evento_valor < 0))
									evento_valor = processo[token].state[i_next]; 
							}

							// vai copiar todas as novidades do vetor state[N]
							for (i = 0; i < N; i++) {

								if(processo[token].state[i] < processo[i_next].state[i]) {
									processo[token].state[i] = processo[i_next].state[i];
									//printf("-- O processo %d obteve informação do processo %d através do processo %d\n", token, i_next, i_correto);
								}

							}
							break;
						}

					}
				}

				//imprime vetor state
				/*printf("state[%d] = ", token);
				for (j=0; j<N; j++) {
					printf("%d ",processo[token].state[j]);
				}
				printf("\n");
				printf("\n");* ######## testar ao inves de imprimir*/
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

			case BEBCAST:
				//se o processo está falho, não faz nada
				if ((status(processo[token].id)) != 0) break;	

				printf("\n\nO processo %d está iniciando um broadcast...\n\n\n", token);

				//mensagem interna
				unsigned int msg_interna_id = newMsg(token, token, MSG, msg_list);

				for (s = 1; s <= max_s; s++)
				{
					vizinho = ff_neighbour(token, s, processo[token].state);

					if (!processo[token].state[vizinho]%2) {
						msg_id = newMsg(token, token, MSG, msg_list);
						sendMsg(vizinho, msg_id);
						printf("%d enviou uma mensagem para %d\n", token, vizinho);

						processo[token].pendingACK[vizinho] = msg_interna_id;	
					}

				}
				break;



			case RECIEVE:
				printf("\n");
				TipoMensagem *msg_antiga;

				recieveMsg(token, &token, &msg_id);
				mensagem = getMsg(msg_list, msg_id);

				if ((status(processo[token].id)) != 0) break; //se estiver falho não faz 

				// se o processo remetente ou origem estiver falho, esvazia o pendingACK
				if ((processo[token].state[mensagem->origem]%2)
						||(processo[token].state[mensagem->remetente]%2)) {
					printf("ENTROU ONDE N DEVIA\n");
					/*for (i = 0; i < N; i++) {
						if (processo[token].pendingACK[i] != NULL) {
							free(processo[token].pendingACK[i]);
							processo[token].pendingACK[i] = NULL;
						}
					}*/
					break;
				}
				
				if (mensagem->tipo == MSG)
				{
					printf("%d recebeu uma mensagem de %d\n", token, mensagem->remetente);
					
					s = cluster(mensagem->remetente, token); //recupera cluster de j que i pertence?
					s--;

					// se for o último processo do s
					if (s == 0) {
						msg_id = newMsg(mensagem->origem, token, ACK, msg_list);
						sendMsg(mensagem->remetente, msg_id);
						printf("%d enviou um mensagem ACK para %d\n", token, mensagem->remetente);
						freeMsg(msg_list, mensagem->id);
						break;
					}

					for (s; s > 0; s--) {
						vizinho = ff_neighbour(token, s, processo[token].state);
 
 						// Guadou ID da mensagem que recebeu
						processo[token].pendingACK[vizinho] = mensagem->id;
						
						msg_id = newMsg(mensagem->origem, token, MSG, msg_list);
						sendMsg(vizinho,msg_id);

						printf("%d enviou uma mensagem para %d\n", token, vizinho);
					}
					break;
				}

				else if (mensagem->tipo == ACK) {
					printf("%d recebeu uma mensagem ACK de %d\n", token, mensagem->remetente);
					
					if (mensagem->origem == token) {
						//free(processo[token].pendingACK[mensagem->remetente]);
						//processo[token].pendingACK[mensagem->remetente] = NULL;
						//free(mensagem);
						// PUXAR/FAZER O NEGOCIO PRA VER SE TERMINOU TODOS OS ACKS AQUI
						//break;
					}

					msg_antiga = getMsg(msg_list, processo[token].pendingACK[mensagem->remetente]);
					processo[token].pendingACK[mensagem->remetente] = -1; //removeu do ack;
					freeMsg(msg_list, mensagem->id); //descarta o ack

					if (vazio(processo[token].pendingACK, N)) {
						if (token == msg_antiga->origem) {
							printf("É a origem!\n");
							freeMsg(msg_list, msg_antiga->id);
							break;
						}

						msg_id = newMsg(msg_antiga->origem, token, ACK, msg_list);
						sendMsg(msg_antiga->remetente, msg_id);
						printf("%d enviou um mensagem ACK para %d\n", token, msg_antiga->remetente);
						freeMsg(msg_list, msg_antiga->id);	
					}
								
					break;
				}

				else { 
					printf("Tipo de Mensagem Inválido\n");
					break;
				}


		}
	}
	printf("\nFim da execução.\n");
	return(0);
}
