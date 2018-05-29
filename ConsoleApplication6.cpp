// ConsoleApplication6.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include "Definitions.h"

using namespace std;

#include "database.h"
#include <fstream>
#define NOMINMAX
#include <windows.h>
//#include "opencl_driver.h"

float comparar_descritor(const float desc1, const float desc2) {
	return fabs(desc1 - desc2);
}


float DTW_do_bottom_up(const vector<float>& d1, const vector<float>& d2)
{
	float matriz_dtw[129][129];

	matriz_dtw[1][1] = comparar_descritor(d1[0], d2[0]);

	//casos base
	for (int j = 2; j <= d2.size(); j++) //primeira linha
		matriz_dtw[1][j] = matriz_dtw[1][j - 1] + comparar_descritor(d1[0], d2[j - 1]);

	for (int i = 2; i <= d1.size(); i++) //primeira coluna
		matriz_dtw[i][1] = matriz_dtw[i - 1][1] + comparar_descritor(d1[i - 1], d2[0]);

	//o resto
	for (int i = 2; i <= d1.size(); i++)
		for (int j = 2; j <= d2.size(); j++)
			matriz_dtw[i][j] = comparar_descritor(d1[i - 1], d2[j - 1]) + min({ matriz_dtw[i - 1][j], matriz_dtw[i - 1][j - 1], matriz_dtw[i][j - 1] });

	return matriz_dtw[d1.size()][d2.size()];
}
float formas_tmp[imagens_por_vez*problem_dimension];

const int size_of_descriptors = 88, VEC_SIZE_MULTI = 128;


/*void execute_pass_dsw(vector<vector<float>>::iterator first, vector<vector<float>>::iterator last, const int thread, int real_begin) {

	vector<vector<float>> descriptors;
	descriptors.assign(first, last);
	const int size = descriptors.size();
	for (int a = 0; a < size; a++) {
		float min;
		for (int i = a + 1; i < size; i++) {
			min = DTW_do_bottom_up(descriptors[a], descriptors[i]);
			for (int b = 0; b < VEC_SIZE_MULTI; b++) {
				rotate(descriptors[i].begin(), descriptors[i].begin() + 1, descriptors[i].end());
				float temp = DTW_do_bottom_up(descriptors[a], descriptors[i]);
				min = temp < min ? temp : min;
			}
		}
		if(thread == 13)
			cout << "VECTOR " << real_begin++ << " DONE! MIN: " << min << endl;
	}

}*/

void execute_pass_dsw(vector<float> to_compare, vector<vector<float>>::iterator first, vector<vector<float>>::iterator last, const int thread, int real_begin) {

	vector<vector<float>> descriptors;
	descriptors.assign(first, last);
	const int size = descriptors.size();
	float min;
	for (int i = 1; i < size; i++) {
		min = DTW_do_bottom_up(to_compare, descriptors[i]);
		for (int b = 0; b < VEC_SIZE_MULTI; b++) {
			rotate(descriptors[i].begin(), descriptors[i].begin() + 1, descriptors[i].end());
			float temp = DTW_do_bottom_up(to_compare, descriptors[i]);
			min = temp < min ? temp : min;
		}
	}
	if (thread == 16)
		cout << "THREAD 16 DONE! MIN: " << min << " IMAGE: " << real_begin << endl;
	

}

vector<float> do_singlethread_dsw(const int size) {
	vector<float> times;
	for (int i = 0; i < 2; i++) {
		vector<vector<float>> triangle_descriptors = getAllTriangleDescriptors("mpeg.txt");

		LARGE_INTEGER t1;
		QueryPerformanceCounter(&t1);
		for (int a = 0; a < size; a++) {
			execute_pass_dsw(triangle_descriptors[a], triangle_descriptors.begin(), triangle_descriptors.begin() + size, 13, 0);
		}

		LARGE_INTEGER t2;
		QueryPerformanceCounter(&t2);
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		times.push_back((t2.QuadPart - t1.QuadPart) / (float)(freq.QuadPart));
	}

	return times;
}

#include <thread>
#include <cmath>

vector<float> do_multithread_dsw_variable(const int desc_size, const int num_tests) {
	
	vector<vector<float>> triangle_descriptors = getAllTriangleDescriptors("mpeg.txt");
	vector<float> times;
	for (int test = 0; test < num_tests; test++) {
		int NUM_OF_THREADS = 16;
		LARGE_INTEGER t1;
		QueryPerformanceCounter(&t1);
		for (int i = 0; i < desc_size; i++) {
			vector<std::thread> tt;
			vector<vector<float>>::iterator it_in = triangle_descriptors.begin() + i;
			int size_desc_thread = floor((desc_size - i) / (float)NUM_OF_THREADS);

			for (int a = 0; a < (NUM_OF_THREADS - 1) && size_desc_thread > 4; a++) {
				tt.push_back(std::thread(execute_pass_dsw, triangle_descriptors[i], it_in, it_in + size_desc_thread, a, i));
				it_in += size_desc_thread;
			}

			execute_pass_dsw(triangle_descriptors[i], it_in, triangle_descriptors.begin() + desc_size, 16, i);

			for (auto &e : tt) {
				e.join();
			}

			NUM_OF_THREADS = size_desc_thread < 20 ? --NUM_OF_THREADS : NUM_OF_THREADS;
		}

		LARGE_INTEGER t2;
		QueryPerformanceCounter(&t2);
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		float time = (t2.QuadPart - t1.QuadPart) / (float)(freq.QuadPart);
		times.push_back(time);
		cout << "MULTI-THREAD" << desc_size << "VARIABLE VALORES TEMPO: " << time << endl;
	}

	return times;
}

vector<float> do_multithread_dsw_constant(const int desc_size, const int num_tests) {

	const int NUM_OF_THREADS = 16;
	vector<vector<float>> triangle_descriptors = getAllTriangleDescriptors("mpeg.txt");
	vector<float> times;
	for (int test = 0; test < num_tests; test++) {
		LARGE_INTEGER t1;
		QueryPerformanceCounter(&t1);
		for (int i = 0; i < desc_size; i++) {
			vector<std::thread> tt;
			vector<vector<float>>::iterator it_in = triangle_descriptors.begin() + i;
			int size_desc_thread = floor((desc_size - i) / (float)NUM_OF_THREADS);

			for (int a = 0; a < (NUM_OF_THREADS - 1) && size_desc_thread > 4; a++) {
				tt.push_back(std::thread(execute_pass_dsw, triangle_descriptors[i], it_in, it_in + size_desc_thread, a, i));
				it_in += size_desc_thread;
			}

			execute_pass_dsw(triangle_descriptors[i], it_in, triangle_descriptors.begin() + desc_size, 16, i);

			for (auto &e : tt) {
				e.join();
			}

		}

		LARGE_INTEGER t2;
		QueryPerformanceCounter(&t2);
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		float time = (t2.QuadPart - t1.QuadPart) / (float)(freq.QuadPart);
		times.push_back(time);
		cout << "MULTI-THREAD" << desc_size << "VARIABLE VALORES TEMPO: " << time << endl;
	}

	return times;
}

int main()
{
	//vector<float> times_variable_1400 = do_multithread_dsw_variable(1400, 4);
	vector<float> times_variable_700 = do_multithread_dsw_variable(700, 5);
	vector<float> times_variable_350 = do_multithread_dsw_variable(350, 5);

	vector<float> times_constant_1400 = do_multithread_dsw_constant(1400, 5);
	vector<float> times_constant_700 = do_multithread_dsw_constant(700, 5);
	vector<float> times_constant_350 = do_multithread_dsw_constant(350, 5);

	for (auto &i : times_variable_700) {
		cout << "MULTI-THREAD 700 VARIABLE VALORES TEMPO: " << i << endl;
	}
	for (auto &i : times_variable_350) {
		cout << "MULTI-THREAD 350 VARIABLE VALORES TEMPO: " << i << endl;
	}

	for (auto &i : times_constant_1400) {
		cout << "MULTI-THREAD 1400 CONSTANT VALORES TEMPO: " << i << endl;
	}
	for (auto &i : times_constant_700) {
		cout << "MULTI-THREAD 700 CONSTANT VALORES TEMPO: " << i << endl;
	}
	for (auto &i : times_constant_350) {
		cout << "MULTI-THREAD 350 CONSTANT VALORES TEMPO: " << i << endl;
	}
	/*vector<float> times_1400 = do_singlethread_dsw(1400);


	for (auto &i : times_1400) {
		cout << "SINGLE THREAD 1400 VALORES TEMPO: " << i << endl;
	}*/

	/*vector<int> test;
	for (int i = 0; i < 1400; i++) test.push_back(i);
	vector<int>::iterator it = test.begin();

	for (int i = 0; i < 15; i++) {
		vector<int> temp;
		temp.assign(it, it + size_of_descriptors);
		cout << "start: " << temp[0] << " end: " << temp[87] << endl;
		it += size_of_descriptors;
	}
	vector<int> temp;
	temp.assign(it, test.end());
	cout << temp[0] << endl;*/

	//initGpuAlgorithm();
	

	//cout << DTW_do_bottom_up(triangle_descriptors[0], triangle_descriptors[1]) << endl;
	
	/*vector<float> resultados;

	LARGE_INTEGER t1;
	QueryPerformanceCounter(&t1);

	vector<pair<int, int>> comparacoes_a_fazer;
	comparacoes_a_fazer.reserve(triangle_descriptors.size());

	for (int a = 0; a < triangle_descriptors.size(); a++)
	{
		for (int i = a; i < triangle_descriptors.size(); i++)
		{
			comparacoes_a_fazer.push_back(std::pair<int, int>( a, i));
		}
		//cout << "processando imagem " << a << " de " << database.size() << endl;
	}
	
	for (int i = 0; i < comparacoes_a_fazer.size(); )
	{
		int a = 0;

		for (; a < comparacoes_por_vez; a++)
		{
			int i1, i2;
			i1 = comparacoes_a_fazer[i + a].first;
		    i2 = comparacoes_a_fazer[i + a].second;
			vector<float>& f1 = triangle_descriptors[comparacoes_a_fazer[i + a].first];
			memcpy(formas_tmp + (2 * a)*problem_dimension, &f1[0], sizeof(float)*problem_dimension);
			vector<float>& f2 = triangle_descriptors[comparacoes_a_fazer[i + a].second];
			memcpy(formas_tmp + (2 * a + 1)*problem_dimension, &f2[0], sizeof(float)*problem_dimension);
			
			f1.erase(f1.end() - 1);
			f2.erase(f2.end() - 1);

			/*cout << endl;
			for (int i = 0; i < problem_dimension; i++)
				cout << dados_formas[problem_dimension+i] << " ";

			cout << endl;
		}

		float* resp = calcularScores(formas_tmp);

		for (int i = 0; i < comparacoes_por_vez; i++)
			resultados.push_back(resp[i]);

		cout << "processando lote de " << i << " a " << (i + comparacoes_por_vez -1) << " (no total " << comparacoes_a_fazer.size() << ")" << endl;
		i += a;

	}



	LARGE_INTEGER t2;
	QueryPerformanceCounter(&t2);
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	std::cout << "tempo: " << (t2.QuadPart - t1.QuadPart) / (float)(freq.QuadPart) << std::endl;

	fstream resultados_stream("resultados.txt", fstream::out);

	for (float a : resultados)
		resultados_stream << a << endl;

	cleanGpu();
	*/
	system("pause");
	return 0;
}