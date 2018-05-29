#include "stdafx.h"
#include "opencl_driver.h"
#include "opencl.h"
#include "Definitions.h"
#include <iostream>
#include <algorithm>

using namespace std;

output respostas[comparacoes_por_vez*problem_dimension]; //as respostas serão copiadas pra ca
float dados_formas[imagens_por_vez*problem_dimension];



cl::Buffer d1_gpu;
cl::Buffer out_gpu;

void initGpuAlgorithm()
{
	initOpencl();
	//cudaMalloc((void**)&d1_gpu, sizeof(float)*problem_dimension*imagens_por_vez);
	//cudaMalloc((void**)&out_gpu, sizeof(struct output)*problem_dimension*comparacoes_por_vez);

	d1_gpu = cl::Buffer(__context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(dados_formas));
	out_gpu = cl::Buffer(__context, CL_MEM_READ_WRITE, sizeof(respostas));

	for (int i = 0; i < comparacoes_por_vez; i++)
	{
		for (int a = 0; a < problem_dimension; a++)
		{
			output* pos = &respostas[problem_dimension*i + a];
			pos->indice_forma1 = 2 * i;
			pos->indice_forma2 = 2 * i + 1;
			pos->deslocamento = a;
		}

	}

	int ret = queue.enqueueWriteBuffer(out_gpu, CL_TRUE, 0, sizeof(respostas), respostas);
	if (ret != CL_SUCCESS)
	{
		cout << "Erro> " << getErrorString(ret) << endl;
		exit(0);
	}
	//cudaMemcpy(out_gpu, respostas, sizeof(respostas), cudaMemcpyHostToDevice);
}


void cleanGpu()
{

}
#include "opencl.h"

float respostas_tmp[comparacoes_por_vez];
const int blocos = problem_dimension*comparacoes_por_vez;
const int threads = problem_dimension + 1;

float* calcularScores(float* descritores)
{
	int ret = queue.enqueueWriteBuffer(d1_gpu, CL_TRUE, 0, sizeof(dados_formas), descritores);

	//cudaMemcpy(d1_gpu, descritores, sizeof(float)*imagens_por_vez*problem_dimension, cudaMemcpyHostToDevice);
	if (ret != CL_SUCCESS)
	{
		cout << "erro ao copiar memoria> " << getErrorString(ret) << endl;
		exit(0);
	}

	//hello << <problem_dimension*comparacoes_por_vez, problem_dimension + 1 >> >(d1_gpu, out_gpu);
	cl::Kernel kernel_add = cl::Kernel(__programa, "simple_add");
	kernel_add.setArg(0, d1_gpu);
	kernel_add.setArg(1, out_gpu);


	ret = queue.enqueueNDRangeKernel(kernel_add, cl::NullRange, cl::NDRange(blocos*threads), cl::NDRange(threads));
	if (ret != CL_SUCCESS)
	{
		cout << "erro ao executar kernel> " << getErrorString(ret) << endl;
		exit(0);
	}
	//cudaDeviceSynchronize();
	queue.finish();


	//cudaMemcpy(respostas, out_gpu, sizeof(respostas), cudaMemcpyDeviceToHost);
	queue.enqueueReadBuffer(out_gpu, CL_TRUE, 0, sizeof(respostas), respostas);

	for (int i = 0; i < comparacoes_por_vez; i++)
	{
		respostas_tmp[i] = respostas[problem_dimension*i].resposta;
		for (int a = 1; a < problem_dimension; a++)
		{
			output* pos = &respostas[problem_dimension*i + a];
			respostas_tmp[i] = min(respostas_tmp[i], pos->resposta);
		}
	}

	return respostas_tmp;
}