#include "database.h"
#include "stdafx.h"

#include <vector>
#include <fstream>
#include <algorithm>
using namespace std;
struct Point
{
	float x;
	float y;
};


float triangleArea(const vector<Point>& p, int i, int t)
{
	int anterior = i - t;
	int proximo = i + t;
	if (anterior < 0)
		anterior += p.size();
	if (proximo >= p.size())
		proximo -= p.size();

	const Point& atual = p[i];
	const Point& next = p[proximo];
	const Point& prev = p[anterior];
	return 0.5*(atual.x*next.y - atual.y*next.x - prev.x*next.y + prev.y*next.x + prev.x*atual.y - prev.y*atual.x);
}

vector<float> triangleAreaSignature(const vector<Point>& p)
{
	int TsMax = p.size() / 2 - 1;
	vector<float> resultados(p.size());

	vector<float> maxTa(TsMax + 1); //Maior área triangular pra cada Ts

	vector<vector<float>> triangular_areas(TsMax + 1); //triangleArea de cada ponto para certo t

	for (int i = 1; i <= TsMax; i++)
		for (int a = 0; a< p.size(); a++)
		{
			float res = triangleArea(p, a, i);
			triangular_areas[i].push_back(res);
			maxTa[i] = max(maxTa[i], res);
		}

	for (int i = 0; i < p.size(); i++)
	{
		float tmp = 0;
		for (int t = 1; t <= TsMax; t++)
		{
			tmp += triangular_areas[t][i] / maxTa[t];
		}
		resultados[i] = (1.0 / TsMax)*tmp;
	}

	return resultados;
}

vector<vector<Point>> readDatabaseFromTxt(const char* filename)
{
	vector<vector<Point>> retorno;
	retorno.reserve(1400);
	fstream arquivo(filename, fstream::in | fstream::binary);

	if (!arquivo.is_open())
		throw std::string("Arquivo nao encontrado.");

	while (1)
	{
		retorno.push_back(vector<Point>(128));

		for (int i = 0; i < 128; i++)
		{
			arquivo >> retorno.back()[i].x;
			arquivo >> retorno.back()[i].y;
		}
		if (arquivo.eof())
		{
			retorno.erase(retorno.end() - 1);
			break;
		}
	}

	return retorno;
}


vector<vector<float>> getAllTriangleDescriptors(const char* filename)
{
	auto database = readDatabaseFromTxt("mpeg.txt");

	vector<vector<float>> triangle_descriptors(database.size());
	for (int i = 0; i < database.size(); i++)
	{
		triangle_descriptors[i] = std::move(triangleAreaSignature(database[i]));
	}

	return triangle_descriptors;

}
