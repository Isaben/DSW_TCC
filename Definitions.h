#pragma once

const int problem_dimension = 127;

struct output
{
	int indice_forma1;
	int indice_forma2;
	int deslocamento;
	float resposta;
};

const int comparacoes_por_vez = 1024; //a gpu pode processar mais de uma imagem por vez
const int imagens_por_vez = comparacoes_por_vez * 2;

