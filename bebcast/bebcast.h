#include <stdio.h>
#include <stdlib.h>

enum tipodemensagem {MSG, ACK};

typedef struct
{
	int origem; 					// identificador da origem da mensagem
	enum tipodemensagem tipo;		// define o tipo da mensagem, ack ou normal
	int remetente;					// identifica o remetente da mensagem, note que pode ser diferente ou não da origem
} TipoMensagem;