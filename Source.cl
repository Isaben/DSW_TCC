struct output
{
	int indice_forma1;
	int indice_forma2;
	int deslocamento;
	float resposta;
};


#define problem_dimension 127

void kernel simple_add(global float* in, global struct output* out)
{
	__local float ultimos_2[problem_dimension + 1][3];
	__local float a[problem_dimension];
	__local float b[problem_dimension];

	int block = get_group_id(0);
	global float* _a = in + (out[block].indice_forma1 * problem_dimension);
	global float* _b = in + (out[block].indice_forma2 * problem_dimension);

	int linha_atual = get_local_id(0);

	a[linha_atual] = _a[linha_atual];
	b[linha_atual] = _b[linha_atual];
	const int deslocamento = out[block].deslocamento;

	ultimos_2[linha_atual][0] = 0;
	ultimos_2[linha_atual][1] = 99999;
	ultimos_2[linha_atual][2] = 99999;

	barrier(CLK_LOCAL_MEM_FENCE);
	for (int i = 0; i < (2 * problem_dimension); i++)
	{
		int atual = (i - linha_atual + 1);


		if (atual >= 0 && atual < problem_dimension && linha_atual > 0)
		{
			atual += deslocamento;
			atual += (atual >= problem_dimension) ? (-problem_dimension) : (0);

			float parte1 = fmin(ultimos_2[linha_atual - 1][0], ultimos_2[linha_atual - 1][1]);
			float parte2 = ultimos_2[linha_atual][1];

			float finalmente = fabs(a[linha_atual - 1] - b[atual]) + fmin(parte2, parte1);

			ultimos_2[linha_atual][2] = finalmente;
		}

		barrier(CLK_LOCAL_MEM_FENCE);
		ultimos_2[linha_atual][0] = ultimos_2[linha_atual][1];
		ultimos_2[linha_atual][1] = ultimos_2[linha_atual][2];
		barrier(CLK_LOCAL_MEM_FENCE);

	}

	if (linha_atual == problem_dimension)
		out[block].resposta = ultimos_2[linha_atual][2];
}                                                                               