/*
Autor: Giovani G Marciniak (copiando do professor)
GRR20182981

Arquivo: tempo.c
Modificado em 26/11/2020
Descrição (uma frase):
Nosso primeiro programa de simulação
Vamos simular N nodos, cada um conta o "tempo" independentemente
Um exemplo simples e significativo para captar o "espirito" da simulação */

#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

// Vamos definir os EVENTOS
#define TEST 1
#define FAULT 2
#define RECOVERY 3

// Vamos definir o descritor do processo
typedef struct
{
	int id; 			// identificador de facility(recurso, no nosso caso processo) do SMPL
	// outras variáveis locais dos processos são declaradas aqui...
} TipoProceso;

TipoProceso *processo;

int main(int argc, char const *argv[])
{
	static int N, 		// número de processos
			token,		//indica o processo que está sendo executado
			event, r, i;

	static char fa_name[5];		//nome da facility

	if (argc != 2) {
		puts("Uso correto: tempo <número de processos>");
		exit(1);
	}

	N = atoi(argv[1]); //atoi converte para número

	smpl(0, "Um Exemplo de simulação");
	reset();
	stream(1);			// temos 1 fluxo de simulação exclusiva, 1 processo por fez, 1 thread

	//inicializar processos
	processo = (TipoProceso*) malloc(sizeof(TipoProceso)*N);

	for (i=0;i<N;i++) {
		memset(fa_name,'\0',5);
		sprintf(fa_name, "%d", i);
		processo[i].id = facility(fa_name, 1);	
	}

	// vamos fazer o escalonamento incial de eventos
	// nossos processos vão executar testes
	// o intervalo de testes vai ser de 30 unidades de tempo
	// a simulação começa no tempo 0(zero) e vamos escalonar o primeiro teste de todos os processos para o tempo 30

	for(i=0;i<N;i++){
		schedule(TEST, 30.0, i);
	}
	schedule(FAULT, 31.0, 1); //o processo 1 falha no tempo 31
	schedule(RECOVERY, 61.0, 1); //o processo volta ao tempo 61

	//loop principal
	while(time() < 150.0) {
		cause(&event, &token); //token retorna id do processo que tá executando agora
		switch(event) {
			case TEST:
				if (status(processo[token].id != 0)) break;		//se o processo está falho, não testa
				printf("O processo %d vai testar no tempo %4.1f\n",token, time());
				schedule(TEST, 30.0, token);
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
				schedule(TEST, 30.0, token);
				break;
		}
	}

}

/* tarefa 1: fazer cada um dos processos testar o seguinte no ane; testar com a função status do SMPL e imprimir (printf)
o resultado de cada teste.
Por exemplo: "O processo 1 testou o processo 2 correto"

tarefa 2: cada processo correto executa testes até achar outro processo correto. Imprimir os testes e resultados.

tarefa 3: cada processo mantém localmente um vetor State[N]
inicializa o State[N] com -1 para as N entraas indicando "unknown"
atualiza as entradas correspondentes do State[] ao testar

tarefa 4: quando um processo correto testa outro processo correto obtém as informações de diagnóstico
do processo testado sobre todos os processos do sistema exceto aqueles que testou nesta rodada





