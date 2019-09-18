/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2019 Panda Team
*/
#include <vector>
#include <any>

#include <iostream>
#include <fstream>

#include <chrono>

#include "../../modules/utils/ThreadPool.cpp"
#include "../../modules/utils/Semaphore.h"

#include "../../assets/json.hpp"
#include "../../modules/mapping.hpp"
#include "../../modules/utils/poor_mans_quantum.hpp"


using json = nlohmann::json;

///////////////////////////////////////////////////////


namespace ooc_functions
{
template <typename T>
std::vector<std::vector<T>>
transpose(std::vector<std::vector<T>> &a)
{

  size_t rows = a.size();
  size_t cols = a[0].size();

  std::vector<std::vector<T>> array(cols, std::vector<T>(rows));
  for (size_t i = 0; i < cols; ++i)
  {
    for (size_t j = 0; j < rows; ++j)
    {
      array[i][j] = a[j][i];
    }
  }
  return array;
}

template <typename T>
std::vector<std::vector<T>>
transposeInPlace(std::vector<std::vector<T>> matrix)
{
  size_t rows = matrix.size();
  size_t cols = matrix[0].size();
  size_t n = std::min(rows, cols);

  for (size_t i = 0; i < n; i++)
  {
    for (size_t j = i; j < n; j++)
    {
      T temp = matrix[j][i];
      matrix[j][i] = matrix[i][j];
      matrix[i][j] = temp;
    }
  }

  if (cols > rows)
  {

    //std::vector<std::vector<T> rest(rows);
    std::vector<std::vector<T>> rest(rows, std::vector<T>());
    std::vector<T> rest2(cols - rows - 1);

    for (size_t i = 0; i < rows; ++i)
    {
      //rest[i] = matrix[i].splice(rows, cols)

      rest[i].insert(rest[i].begin(), std::make_move_iterator(matrix[i].begin() + rows), std::make_move_iterator(matrix[i].begin() + cols));
      matrix[i].erase(matrix[i].begin() + rows, matrix[i].begin() + cols);
    }

    for (size_t i = 0; i < cols - rows; ++i)
    {
      matrix.push_back(rest2);
    }

    for (size_t i = 0; i < rest[0].size(); ++i)
    {
      for (size_t j = 0; j < rest.size(); j++)
      {
        matrix[i + rows][j] = rest[j][i];
      }
    }
  }

  else if (cols < rows)
  {

    //std::vector<T> rests(rows-cols);
    std::vector<std::vector<T>> rest(rows - cols, std::vector<T>());

    for (size_t i = 0; i < cols; ++i)
    {
      // matrix[i].concat(rest)
      matrix[i].reserve(matrix[i].size()+rows-cols);
    }

    for (size_t i = 0; i < rows - cols; ++i)
    {
      //rest[i] = matrix[i + cols].splice(0, cols)

      rest[i].insert(rest[i].begin(), std::make_move_iterator(matrix[i + cols].begin() + 0), std::make_move_iterator(matrix[i + cols].begin() + cols));
      matrix[i + cols].erase(matrix[i + cols].begin() + 0, matrix[i + cols].begin() + cols);
    }

    for (size_t i = 0; i < rest[0].size(); ++i)
    {
      for (size_t j = 0; j < rest.size(); j++)
      {
        matrix[i][j + cols] = rest[j][i];
      }
    }

    for (size_t i = 0; i < rows - cols; ++i)
    {
      matrix.pop_back();
    }
  }
  return matrix;
}

// --------------------------------------------------------------
// sorting functions

template <typename T>
void quicksort(std::vector<T> &a, size_t lo, size_t hi);

template <typename T>
void insertionsort(std::vector<T> &a, size_t lo, size_t hi);

template <typename T>
void dualPivotSort(std::vector<T> &a, size_t lo = 0, size_t hi = 0)
{
  if (hi == 0)
  {
    hi = a.size();
  }
  size_t quicksort_sizeThreshold = 32;
  (hi - lo < quicksort_sizeThreshold ? insertionsort(a, lo, hi) : quicksort(a, lo, hi));
}

template <typename T>
void insertionsort(std::vector<T> &a, size_t lo, size_t hi)
{

  T t, x;
  size_t i, j;
  for (i = lo + 1; i < hi; ++i)
  {
    for (j = i, t = a[i], x = t; j > lo && a[j - 1] > x; --j)
    {

      a[j] = a[j - 1];
    }
    a[j] = t;
  }
}

template <typename T>
void quicksort(std::vector<T> &a, size_t lo, size_t hi)
{
  // Compute the two pivots by looking at 5 elements.
  size_t sixth = (hi - lo) / 6 | 0,
         i1 = lo + sixth,
         i5 = hi - 1 - sixth,
         i3 = (lo + hi - 1) >> 1, // The midpoint.
      i2 = i3 - sixth,
         i4 = i3 + sixth;

  T e1 = a[i1], x1 = e1,
    e2 = a[i2], x2 = e2,
    e3 = a[i3], x3 = e3,
    e4 = a[i4], x4 = e4,
    e5 = a[i5], x5 = e5;

  T t;

  if (x1 > x2)
    t = e1, e1 = e2, e2 = t, t = x1, x1 = x2, x2 = t;
  if (x4 > x5)
    t = e4, e4 = e5, e5 = t, t = x4, x4 = x5, x5 = t;
  if (x1 > x3)
    t = e1, e1 = e3, e3 = t, t = x1, x1 = x3, x3 = t;
  if (x2 > x3)
    t = e2, e2 = e3, e3 = t, t = x2, x2 = x3, x3 = t;
  if (x1 > x4)
    t = e1, e1 = e4, e4 = t, t = x1, x1 = x4, x4 = t;
  if (x3 > x4)
    t = e3, e3 = e4, e4 = t, t = x3, x3 = x4, x4 = t;
  if (x2 > x5)
    t = e2, e2 = e5, e5 = t, t = x2, x2 = x5, x5 = t;
  if (x2 > x3)
    t = e2, e2 = e3, e3 = t, t = x2, x2 = x3, x3 = t;
  if (x4 > x5)
    t = e4, e4 = e5, e5 = t, t = x4, x4 = x5, x5 = t;

  T pivot1 = e2, pivotValue1 = x2,
    pivot2 = e4, pivotValue2 = x4;

  a[i1] = e1;
  a[i2] = a[lo];
  a[i3] = e3;
  a[i4] = a[hi - 1];
  a[i5] = e5;

  size_t less = lo + 1, // First element in the middle partition.
      great = hi - 2;   // Last element in the middle partition.

  bool pivotsEqual = pivotValue1 <= pivotValue2 && pivotValue1 >= pivotValue2;
  if (pivotsEqual)
  {

    for (size_t k = less; k <= great; ++k)
    {
      T ek = a[k], xk = ek;
      if (xk < pivotValue1)
      {
        if (k != less)
        {
          a[k] = a[less];
          a[less] = ek;
        }
        ++less;
      }
      else if (xk > pivotValue1)
      {

        /* eslint no-constant-condition: 0 */
        while (true)
        {
          T greatValue = a[great];
          if (greatValue > pivotValue1)
          {
            great--;
            continue;
          }
          else if (greatValue < pivotValue1)
          {
            a[k] = a[less];
            a[less++] = a[great];
            a[great--] = ek;
            break;
          }
          else
          {
            a[k] = a[great];
            a[great--] = ek;
            break;
          }
        }
      }
    }
  }
  else
  {
    // (function () { // isolate scope
    {
      for (size_t k = less; k <= great; k++)
      {
        T ek = a[k], xk = ek;
        if (xk < pivotValue1)
        {
          if (k != less)
          {
            a[k] = a[less];
            a[less] = ek;
          }
          ++less;
        }
        else
        {
          if (xk > pivotValue2)
          {
            while (true)
            {
              T greatValue = a[great];
              if (greatValue > pivotValue2)
              {
                great--;
                if (great < k)
                  break;

                continue;
              }
              else
              {

                if (greatValue < pivotValue1)
                {
                  a[k] = a[less];
                  a[less++] = a[great];
                  a[great--] = ek;
                }
                else
                {
                  a[k] = a[great];
                  a[great--] = ek;
                }
                break;
              }
            }
          }
        }
      }
    }
    // })(); // isolate scope
  }

  a[lo] = a[less - 1];
  a[less - 1] = pivot1;
  a[hi - 1] = a[great + 1];
  a[great + 1] = pivot2;

  dualPivotSort(a, lo, less - 1);
  dualPivotSort(a, great + 2, hi);

  if (pivotsEqual)
  {
    return;
  }

  if (less < i1 && great > i5)
  {

    // (function () { // isolate scope
    {
      T lessValue, greatValue;
      while ((lessValue = a[less]) <= pivotValue1 && lessValue >= pivotValue1)
        ++less;
      while ((greatValue = a[great]) <= pivotValue2 && greatValue >= pivotValue2)
        --great;

      for (size_t k = less; k <= great; k++)
      {
        T ek = a[k], xk = ek;
        if (xk <= pivotValue1 && xk >= pivotValue1)
        {
          if (k != less)
          {
            a[k] = a[less];
            a[less] = ek;
          }
          less++;
        }
        else
        {
          if (xk <= pivotValue2 && xk >= pivotValue2)
          {
            /* eslint no-constant-condition: 0 */
            while (true)
            {
              greatValue = a[great];
              if (greatValue <= pivotValue2 && greatValue >= pivotValue2)
              {
                great--;
                if (great < k)
                  break;
                continue;
              }
              else
              {
                if (greatValue < pivotValue1)
                {
                  a[k] = a[less];
                  a[less++] = a[great];
                  a[great--] = ek;
                }
                else
                {
                  a[k] = a[great];
                  a[great--] = ek;
                }
                break;
              }
            }
          }
        }
      }
    }
    //})(); // isolate scope
  }

  dualPivotSort(a, less, great + 1);
}

// linspace (erzeugt einen linearen Datenvektor)
template <typename T>
std::vector<T>
linspace(T a, T b, int n)
{
  std::vector<T> array;
  if (n > 1)
  {
    T step = (b - a) / T(n - 1);
    int count = 0;
    while (count < n)
    {
      array.push_back(a + count * step);
      ++count;
    }
  }
  else
  {
    array.push_back(b);
  }
  return array;
}

template <typename T>
T Lerp(T v0, T v1, T t)
{
  return (1 - t) * v0 + t * v1;
}

template <typename T>
T quickQuantil(std::vector<T> data, T probs)
{

  if (!(data.size() > 0))
    return 0;

  if (1 == data.size())
    return data[0];

  T poi = Lerp(T(-0.5), data.size() - T(0.5), probs);

  int left = std::max(int(std::floor(poi)), int(0));
  int right = std::min(int(std::ceil(poi)), int(data.size() - 1));

  if (probs <= T(0.5))
    std::nth_element(data.begin(), data.begin() + left, data.end());
  else
    std::nth_element(data.begin(), data.begin() + right, data.end());

  T datLeft = data[left];
  T datRight = data[right];

  T quantile = Lerp(datLeft, datRight, poi - T(left));

  return quantile;
}

// akima interpolation
/*
Ref. : Hiroshi Akima, Journal of the ACM, Vol. 17, No. 4, October 1970,
      pages 589-602.
*/
template <typename T>
std::vector<T>
akimaInterp1(std::vector<T> const &x, std::vector<T> const &y, std::vector<T> const &xi, bool save_Mode = true)
{
  // check inputs

  //calculate u vector
  auto uVec = [](std::vector<T> const &x, std::vector<T> const &y) {
    size_t n = x.size();
    std::vector<T> u((n + 3));
    for (size_t i = 1; i < n; ++i)
    {
      u[i + 1] = (y[i] - y[i - 1]) / (x[i] - x[i - 1]); // Shift i to i+2
    }

    auto akima_end = [](const T &u1, const T &u2) {
      return 2.0 * u1 - u2;
    };

    u[1] = akima_end(u[2], u[3]);
    u[0] = akima_end(u[1], u[2]);
    u[n + 1] = akima_end(u[n], u[n - 1]);
    u[n + 2] = akima_end(u[n + 1], u[n]);

    return u;
  };
  std::vector<T> u = uVec(x, y);

  // calculate yp vector
  std::vector<T> yp(x.size());
  for (size_t i = 0; i < x.size(); ++i)
  {
    auto a = std::abs(u[i + 3] - u[i + 2]);
    auto b = std::abs(u[i + 1] - u[i]);
    if ((a + b) != 0)
    {
      yp[i] = (a * u[i + 1] + b * u[i + 2]) / (a + b);
    }
    else
    {
      yp[i] = (u[i + 2] + u[i + 1]) / 2.0;
    }
  }

  // calculte interpolated yi values
  auto kFind = [](const T &xii, const std::vector<T> &x, int start, int end) {

    int klo = start;
    int khi = end;
    // // Find subinterval by bisection
    while (khi - klo > 1)
    {
      int k = (khi + klo) / 2;
      x[k] > xii ? khi = k : klo = k;
    }
    return klo;
  };

  std::vector<T> yi(xi.size());
  for (size_t i = 0; i < xi.size(); ++i)
  {
    // Find the right place in the table by means of a bisection.
    int k = kFind(xi[i], x, int(0), x.size() - 1);

    // Evaluate Akima polynomial
    T b = x[k + 1] - x[k];
    T a = xi[i] - x[k];
    yi[i] = y[k] + yp[k] * a + (3.0 * u[k + 2] - 2.0 * yp[k] - yp[k + 1]) * a * a / b + (yp[k] + yp[k + 1] - 2.0 * u[k + 2]) * a * a * a / (b * b);

    // Differentiate to find the second-order interpolant
    //ypi[i] = yp[k] + (3.0*u[k+2] - 2.0*yp[k] - yp[k+1])*2*a/b + (yp[k] + yp[k+1] - 2.0*u[k+2])*3*a*a/(b*b);

    // Differentiate to find the first-order interpolant
    //yppi[i] = (3.0*u[k+2] - 2.0*yp[k] - yp[k+1])*2/b + (yp[k] + yp[k+1] - 2.0*u[k+2])*6*a/(b*b);
  }
  return yi;
}
}

// end helper functions


//////////////////////////////////////////////////////////////////////


template <typename T>
void matrix_print(const std::vector<std::vector<T>> &mat)
{

    std::cout << "[";
    std::cout << std::endl;
	for (int i = 0; i < mat.size(); i++)
	{
		std::cout << "  [ ";
		if (mat[i].size() > 0)
		{
			for (int j = 0; j < mat[i].size() - 1; j++)
			{
				std::cout << mat[i][j] << ", ";
			}
			std::cout << mat[i][mat[i].size() - 1];
		}
			
		std::cout << " ]" << std::endl;
    }
    std::cout << std::endl;
    std::cout << "]" << std::endl;
}

template <typename T>
void vector_print(const std::vector<T> &vec)
{

    std::cout << "[ ";
    for (int i = 0; i < vec.size() - 1; i++)
    {
        std::cout << vec[i] << ", ";
    }
    std::cout << vec[vec.size() - 1] << " ]" << std::endl;
}

template <typename T>
void vector_print(const std::vector<T> &vec,const size_t width, const size_t height)
{
	if ((width * height) != vec.size()) {
		std::cout << "width * height != vector.size()" << std::endl;
		return;
	}

	for (auto index = 0; index < vec.size(); ++index) {
		std::cout << vec[index] << " ";

		if ((index + 1) % width == 0) {
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;
}

void printDataInfo(const json& data)
{
	for (const auto& [key, value]: data.items()) {
		std::cout << key << " " << value.size() << std::endl;
	}
}


std::vector<std::vector<std::string>> readCsvData(std::string filename, char delimeter)
{
	std::fstream fin;

	fin.open(filename, std::ios::in);
	
	std::vector<std::string> row;
	std::string line, word, w;

	std::vector<std::vector<std::string>> rows;

	// omit headers 
	getline(fin, line);

	int i = 0;
	while (getline(fin, line))
	{
		i++;
		//std::cout << "row " << i << std::endl;
		std::stringstream s(line);

		row.clear();
		while (getline(s, word, delimeter))
		{
			//std::cout << " -> " << word << std::endl;
			
			row.push_back(word);
		}

		rows.push_back(row);
	}

	return rows;
}


std::vector<std::vector<double>> readEnergies(std::string filename)
{
	std::fstream fin;

	fin.open(filename, std::ios::in);
	
	std::vector<double> row;
	std::string line, word, w;

	std::vector<std::vector<double>> rows;

	char delimeter = 9;

	int i = 0;
	while (getline(fin, line))
	{
		std::stringstream s(line);

		row.clear();
		// omit first digit
		getline(s, word, delimeter);

		while (getline(s, word, delimeter))
		{
			// std::cout << " -> " << word << std::endl;
			
			row.push_back(std::stold(word));
		}
		// erase last element
		row.pop_back();

		rows.push_back(row);
	}

	return rows;
}

void saveToCsv(std::string filename, const std::vector<std::vector<std::string>> &mat, const std::vector<std::string> &features)
{
	std::ofstream outputFile;

	// create and open the .csv file
	outputFile.open(filename);

	// write the file headers
	for (auto i = 0; i < features.size(); ++i)
	{
		outputFile << features[i];
		outputFile << ",";
	}
	outputFile << std::endl;

	// last item in the mat is date
	for (auto i = 0; i < mat.size(); ++i)
	{
		//outputFile << dates[i] << ";";
		for (auto j = 0; j < mat[i].size(); j++)
		{
			outputFile << mat[i][j] << ",";
		}
		outputFile << std::endl;
	}

	// close the output file
	outputFile.close();
}


std::mutex mu;

template <typename T, typename Metric, typename Graph, typename Distribution>
double runConfiguration(int i, std::vector<std::vector<T>> data, Metric distance, Graph graph, Distribution distribution, 
	unsigned int iterations, double start_learn_rate, double final_learn_rate, double neighborhoodSize, double neigbour_range_decay, long long random_seed)
{
	
	mu.lock();
	std::cout << "configuration #" << i << " started" << std::endl;
	std::cout << "  Graph: " << typeid(graph).name() << std::endl;
	std::cout << "  Distance: " << typeid(distance).name() << std::endl;
	std::cout << "  Distribution: " << typeid(distribution).name() << std::endl;
	std::cout << std::endl;
	mu.unlock();

	auto t1 = std::chrono::steady_clock::now();

	metric::SOM<std::vector<T>, Metric, Graph, Distribution> DR(distance, graph, distribution, neighborhoodSize, neigbour_range_decay, random_seed);
	

	/* Train */
	DR.train(data, iterations, start_learn_rate, final_learn_rate);

	double total_distances = 0;
	for (size_t i = 0; i < data.size(); i++)
	{
		auto dimR = DR.reduce(data[i]);
		auto bmu = DR.BMU(data[i]);
		total_distances += std::abs(dimR[bmu]);
	}
		
	auto t2 = std::chrono::steady_clock::now();
	mu.lock();
	std::cout << "configuration #" << i << " finished" << std::endl;
	std::cout << "  Graph: " << typeid(graph).name() << std::endl;
	std::cout << "  Distance: " << typeid(distance).name() << std::endl;
	std::cout << "  Distribution: " << typeid(distribution).name() << std::endl;
	std::cout << "Total distances: " << total_distances << 
		" mean distance: " << total_distances / data.size() << 
		" (Time = " << double(std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()) / 1000000 << "s)" << std::endl << std::endl;
	mu.unlock();

	return total_distances;
}

template <typename T>
std::vector<std::vector<T>>
set2conf(std::vector<T> set_0, size_t windowSize, size_t samples, T confidencelevel)
{

  std::random_device rd;  //seed for the random number engine
  std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<> dis(T(0), T(1));

  // propabiliy vector
  std::vector<T> prob_0 = ooc_functions::linspace(T(1) / T(set_0.size()), T(1) - T(1) / T(set_0.size()), set_0.size());
  std::sort(set_0.begin(), set_0.end());
  //ooc_functions::dualPivotSort(set_0);

  // compute probability matrix of set_0
  std::vector<std::vector<T>> m_0(samples, std::vector<T>(set_0.size()));
  //std::vector<std::vector<T>> m_prop_0(samples, std::vector<T>(set_0.size()));

  for (size_t i = 0; i < samples; ++i)
  {
    for (size_t j = 0; j < set_0.size(); ++j)
    {
      m_0[i][j] = T(dis(gen)); // fill with random numbers
    }

    std::sort(m_0[i].begin(), m_0[i].end()); // sort the row
    //ooc_functions::dualPivotSort(m_prop_0[i]);
    m_0[i] = ooc_functions::akimaInterp1(prob_0, set_0, m_0[i]); // interpolate the random numbers
  }

  // m_prop_0.clear();
  // m_prop_0.shrink_to_fit();

  // transpose
  auto m_0t = ooc_functions::transpose(m_0);

  m_0.clear();
  m_0.shrink_to_fit();

  // compute left and right confidence boundaries of set_0
  std::vector<T> set_0_left(set_0.size());
  std::vector<T> set_0_right(set_0.size());
  for (size_t i = 0; i < set_0.size(); ++i)
  {
    set_0_left[i] = ooc_functions::quickQuantil(m_0t[i], (T(1) - confidencelevel) / T(2));
    set_0_right[i] = ooc_functions::quickQuantil(m_0t[i], confidencelevel + (T(1) - confidencelevel) / T(2));
  }

    m_0t.clear();
  m_0t.shrink_to_fit();

  // compute probability matrix of left and right and medians of set_0
  std::vector<std::vector<T>> m_prop_1(samples, std::vector<T>(windowSize));


  for (size_t i = 0; i < samples; ++i)
  {
    for (size_t j = 0; j < windowSize; ++j)
    {
      m_prop_1[i][j] = T(dis(gen)); // fill with random numbers
    }

    std::sort(m_prop_1[i].begin(), m_prop_1[i].end()); // sort the row
  }

  std::vector<std::vector<T>> quants(3, std::vector<T>(windowSize));


  // left
  std::vector<std::vector<T>> m(samples, std::vector<T>(windowSize));
  for (size_t i = 0; i < samples; ++i)
  {
      m[i] = ooc_functions::akimaInterp1(prob_0, set_0_left, m_prop_1[i]); // interpolate the random numbers
  }

  // set_0_left.clear();
  // set_0_left.shrink_to_fit();

  auto mt = ooc_functions::transpose(m);

  for (size_t i = 0; i < windowSize; ++i)
  {
    quants[0][i] = ooc_functions::quickQuantil(mt[i], (T(1.0) - confidencelevel) / T(2.0));
  }

  //right
  for (size_t i = 0; i < samples; ++i)
  {
      m[i] = ooc_functions::akimaInterp1(prob_0, set_0_right, m_prop_1[i]);
  }

  // set_0_right.clear();
  // set_0_right.shrink_to_fit();

  mt = ooc_functions::transpose(m);

  for (size_t i = 0; i < windowSize; ++i)
  {
    quants[2][i] = ooc_functions::quickQuantil(mt[i], confidencelevel + (T(1.0) - confidencelevel) / T(2.0));
  }

  //median
  for (size_t i = 0; i < samples; ++i)
  {
      m[i] = ooc_functions::akimaInterp1(prob_0, set_0, m_prop_1[i]);
  }

  mt = ooc_functions::transpose(m);
  // m.clear();
  // m.shrink_to_fit();

  // m_prop_1.clear();
  // m_prop_1.shrink_to_fit();

  for (size_t i = 0; i < windowSize; ++i)
  {
    quants[1][i] = ooc_functions::quickQuantil(mt[i], T(0.5));
  }

  return quants;
}

template <typename T>
std::vector<std::vector<std::vector<T>>>
set2multiconf(std::vector<T> set_0, std::vector<uint32_t> windowSizes, size_t samples, T confidencelevel)
{
  std::vector<std::vector<std::vector<T>>> multiquants;
  for (size_t i = 0; i < windowSizes.size(); ++i)
  {
    multiquants.push_back(set2conf(set_0, windowSizes[i], samples, confidencelevel));
  }

  return multiquants;
}


int main(int argc, char *argv[])
{
	std::cout << "SOM example have started" << std::endl;
	std::cout << '\n';

	bool hyperparams_tune = true;
	std::string hyperparams_scores_filename;

	if (argc > 1)
	{
		hyperparams_tune = false;
		if (argv[1] == std::string("hyperparams_tune"))
		{
			hyperparams_tune = true;
		}
		else 
		{
			hyperparams_scores_filename = argv[1];
		}
	}

	/* Load data */
	auto speeds = readEnergies("assets/energies_speed_190820.log");
	std::cout << "Num records: " << speeds.size() << std::endl;
	std::cout << "Num values in the record: " << speeds[0].size() << std::endl;


	//unsigned concurentThreadsSupported = std::thread::hardware_concurrency();
	//std::cout << "Num cores: " << concurentThreadsSupported << std::endl;
	//ThreadPool pool(concurentThreadsSupported);
	//Semaphore sem;
	//
	//
	//typedef std::variant<metric::Grid4, metric::Grid4, metric::Grid6, metric::Grid8, metric::Paley, metric::LPS, metric::Margulis> Grid;
	//typedef std::variant<metric::Euclidian<double>, metric::Manhatten<double>, metric::P_norm<double>, metric::Euclidian_thresholded<double>, metric::Cosine<double>, metric::Chebyshev<double>> Distance;
	//typedef std::variant<std::uniform_real_distribution<double>, std::normal_distribution<double>, std::exponential_distribution<double>> Distribution;

	//std::vector<int> graph_types = {0, 1, 2};

	//std::vector<std::any> metric_types;
	//metric_types.push_back(metric::Euclidian<double>());
	//metric_types.push_back(metric::Manhatten<double>());
	//metric_types.push_back(metric::P_norm<double>());
	//metric_types.push_back(metric::Euclidian_thresholded<double>());
	//metric_types.push_back(metric::Cosine<double>());
	//metric_types.push_back(metric::Chebyshev<double>());

	//std::vector<Distribution> distribution_types;
	//distribution_types.push_back(std::uniform_real_distribution<double>(-1, 1));
	//distribution_types.push_back(std::normal_distribution<double>(-1, 1));
	//distribution_types.push_back(std::exponential_distribution<double>(1));

	
	//std::vector<size_t> grid_sizes = {4, 9, 16, 25, 36, 49};
	//std::vector<double> s_learn_rates = {0.2, 0.5, 0.8, 1, 1.2};
	//std::vector<double> f_learn_rates = {0.2, 0.5, 0.7, 0.9};
	//std::vector<double> initial_neighbour_sizes = {0.5, 0.7, 0.9};
	//std::vector<double> neigbour_range_decays = {0.2, 0.3, 0.5};
	//std::vector<long long> random_seeds = {0, 100, 10000};
	//std::vector<unsigned int> iterations_all = {100, 1000, 10000};
	//			
	////size_t grid_size = 25;

	////double s_learn_rate = 0.9;
	////double f_learn_rate = 0.4;
	//			
	////double initial_neighbour_size = 1.2;

	////double neigbour_range_decay = 0;

	////long long random_seed = 0;

	////batch_training

	////unsigned int iterations = 1000;

	//int epochs = 1;
	//
	std::vector<std::string> graph_type_names = {"Grid4", "Grid6", "Grid8", "Paley", "LPS", "Margulis"};
	std::vector<std::string> metric_type_names = {"Euclidian", "Manhatten", "P_norm", "Euclidian_thresholded", "Cosine", "Chebyshev"};
	std::vector<std::string> distribution_type_names = {"uniform_real_distribution", "normal_distribution", "exponential_distribution"};

	////
	//
	int best_graph;
	int best_metric;
	int best_distribution;
				
	size_t best_grid_size;

	double best_s_learn_rate;
	double best_f_learn_rate;
				
	double best_initial_neighbour_size;

	double best_neigbour_range_decay;

	long long best_random_seed;
	
	unsigned int best_iterations;

	//

	////
	if (hyperparams_tune)
	{
	//	std::vector<std::string> metaparams_grid = {"grid_size", "s_learn_rate", "f_learn_rate", "initial_neighbour_size", "neigbour_range_decay",
	//		"random_seed", "iterations", "distribution_type", "metric_type", "graph_type", "score"};
	//	std::vector<std::vector<std::string>> results_grid;

	//	//
	//	const int count = graph_types.size() * metric_types.size() * distribution_types.size() * 
	//		grid_sizes.size() * s_learn_rates.size() * f_learn_rates.size() * initial_neighbour_sizes.size() * neigbour_range_decays.size() * random_seeds.size() * iterations_all.size();

	//	std::vector<double> results(count, INFINITY);
	//	std::cout << "Num configurations: " << count << std::endl;

	//	int i = 0;
	//	for (auto grid_size : grid_sizes)
	//	{
	//		for (auto s_learn_rate : s_learn_rates)
	//		{
	//			for (auto f_learn_rate : f_learn_rates)
	//			{
	//				for (auto initial_neighbour_size : initial_neighbour_sizes)
	//				{
	//					for (auto neigbour_range_decay : neigbour_range_decays)
	//					{
	//						for (auto random_seed : random_seeds)
	//						{
	//							for (auto iterations : iterations_all)
	//							{
	//								for (int distribution_index = 0; distribution_index < distribution_types.size(); distribution_index++)
	//								{
	//									for (int metric_index = 0; metric_index < metric_types.size(); metric_index++)
	//									{
	//										for (auto graph_type : graph_types)
	//										{
	//											auto distribution_type = distribution_types[distribution_index];
	//											auto metric_type = metric_types[metric_index];

	//											pool.execute([i, &sem, &results, &speeds, graph_type, metric_type, metric_index, distribution_type, distribution_index,
	//												iterations, s_learn_rate, f_learn_rate, initial_neighbour_size, neigbour_range_decay, grid_size, random_seed, &results_grid, &metaparams_grid,
	//												&graph_type_names, &metric_type_names, &distribution_type_names]() {

	//												double score;

	//												try {								
	//													if (graph_type == 0)
	//													{
	//														// Grid4
	//														metric::Grid4 graph(grid_size);
	//														score = runConfiguration(i, speeds, metric_type, graph, distribution_type, iterations, s_learn_rate, f_learn_rate, initial_neighbour_size, neigbour_range_decay, random_seed);
	//													}
	//													else if (graph_type == 1)
	//													{
	//														// Grid6
	//														metric::Grid6 graph(grid_size);
	//														score = runConfiguration(i, speeds, metric_type, graph, distribution_type, iterations, s_learn_rate, f_learn_rate, initial_neighbour_size, neigbour_range_decay, random_seed);
	//													}
	//													else if (graph_type == 2)
	//													{
	//														// Grid8
	//														metric::Grid8 graph(grid_size);
	//														score = runConfiguration(i, speeds, metric_type, graph, distribution_type, iterations, s_learn_rate, f_learn_rate, initial_neighbour_size, neigbour_range_decay, random_seed);
	//													}
	//													else if (graph_type == 3)
	//													{
	//														// LPS
	//														metric::LPS graph(grid_size);
	//														score = runConfiguration(i, speeds, metric_type, graph, distribution_type, iterations, s_learn_rate, f_learn_rate, initial_neighbour_size, neigbour_range_decay, random_seed);
	//													}
	//													else if (graph_type == 4)
	//													{
	//														// Margulis
	//														metric::Margulis graph(grid_size);
	//														score = runConfiguration(i, speeds, metric_type, graph, distribution_type, iterations, s_learn_rate, f_learn_rate, initial_neighbour_size, neigbour_range_decay, random_seed);
	//													}
	//														


	//												}
	//												catch (const std::runtime_error& e) {
	//													std::cout << "configuration #" << i << ": runtime error: " << e.what() << std::endl;
	//												}
	//												catch (const std::exception& e) {
	//													std::cout << "configuration #" << i << ": exception: " << e.what() << std::endl;
	//												}
	//												catch (...) {
	//													std::cout << "configuration #" << i << ": unknown error" << std::endl;
	//												}
	//											
	//												mu.lock();
	//												
	//												
	//												auto graph_type_name = graph_type_names[graph_type];
	//												
	//												auto distribution_type_name = distribution_type_names[distribution_index];

	//												auto metric_type_name = metric_type_names[metric_index];
	//											
	//												std::vector<std::string> current_result = {std::to_string(grid_size), std::to_string(s_learn_rate), std::to_string(f_learn_rate), 
	//																							std::to_string(initial_neighbour_size), std::to_string(neigbour_range_decay),
	//																							std::to_string(random_seed), std::to_string(iterations), 
	//																							distribution_type_name, metric_type_name, graph_type_name, 
	//																							std::to_string(score)};

	//												results_grid.push_back(current_result);

	//												if (i % 10000 == 0) {
	//													saveToCsv("SOM_example_2_checkpoint_" + std::to_string(i) + ".csv", results_grid, metaparams_grid);
	//												}

	//												mu.unlock();

	//												results.at(i) = score;

	//												sem.notify();
	//											});

	//											i++;
	//										}
	//									}
	//								}
	//							}
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}
	//
	//	for (auto grid_size : grid_sizes)
	//	{
	//		for (auto s_learn_rate : s_learn_rates)
	//		{
	//			for (auto f_learn_rate : f_learn_rates)
	//			{
	//				for (auto initial_neighbour_size : initial_neighbour_sizes)
	//				{
	//					for (auto neigbour_range_decay : neigbour_range_decays)
	//					{
	//						for (auto random_seed : random_seeds)
	//						{
	//							for (auto iterations : iterations_all)
	//							{
	//								for (auto distribution_type : distribution_types)
	//								{
	//									for (auto metric_type : metric_types)
	//									{
	//										for (auto graph_type : graph_types)
	//										{
	//											sem.wait();
	//										}
	//									}
	//								}
	//							}
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}
	//	pool.close();
	//
	//
	//	double minimal_score = INFINITY;

	//	i = 0;
	//	for (auto grid_size : grid_sizes)
	//	{
	//		for (auto s_learn_rate : s_learn_rates)
	//		{
	//			for (auto f_learn_rate : f_learn_rates)
	//			{
	//				for (auto initial_neighbour_size : initial_neighbour_sizes)
	//				{
	//					for (auto neigbour_range_decay : neigbour_range_decays)
	//					{
	//						for (auto random_seed : random_seeds)
	//						{
	//							for (auto iterations : iterations_all)
	//							{
	//								for (auto distribution_type : distribution_types)
	//								{
	//									for (auto metric_type : metric_types)
	//									{
	//										for (auto graph_type : graph_types)
	//										{
	//											if (results[i] < minimal_score)
	//											{
	//												minimal_score = results[i];
	//												best_grid_size = grid_size;
	//												
	//												best_graph = graph_type;

	//												best_metric = metric_type;
	//												best_distribution = distribution_type;
	//											
	//												best_s_learn_rate = s_learn_rate;
	//												best_f_learn_rate = f_learn_rate;
	//												best_initial_neighbour_size = initial_neighbour_size;
	//												best_neigbour_range_decay = neigbour_range_decay;
	//												best_random_seed = random_seed;
	//												best_iterations = iterations;
	//											}
	//											i++;
	//										}
	//									}
	//								}
	//							}
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}

	//
	//	std::cout << std::endl;
	//	std::cout << std::endl;
	//	std::cout << "The best configuration: " << std::endl;
	//	std::cout << "  Score: " << minimal_score << std::endl;
	//	auto best_graph_type = graph_type_names[best_graph];
	//	std::cout << "  Graph: " << best_graph_type << std::endl;
	//	std::cout << "  Distance: " << typeid(best_metric).name() << std::endl;
	//	std::cout << "  Distribution: " << typeid(best_distribution).name() << std::endl;
	//	std::cout << "  Grid size: " << best_grid_size << std::endl;
	//	std::cout << "  Iterations: " << best_iterations << std::endl;
	//	std::cout << "  Start learn rate: " << best_s_learn_rate << std::endl;
	//	std::cout << "  Final learn rate: " << best_f_learn_rate << std::endl;
	//	std::cout << "  Initial neighbour size: " << best_initial_neighbour_size << std::endl;
	//	std::cout << "  Neigbour range decay: " << best_neigbour_range_decay << std::endl;
	//	std::cout << "  Random seeds: " << best_random_seed << std::endl;
	}
	else
	{
		// load metaparms tune results and shood the best (with the lowets score)

		auto metaparams_grid = readCsvData(hyperparams_scores_filename, ',');
		
		std::vector<double> scores;

		for (auto row : metaparams_grid)
		{
			scores.push_back(std::stod(row[10]));
		}
		
		std::cout << std::endl;
		std::cout << "Num scores: " << scores.size() << std::endl;

		int minElementIndex = std::min_element(scores.begin(), scores.end()) - scores.begin();
		
		std::cout << "The best metaparams index: " << minElementIndex << std::endl;
		
		auto it = std::find (graph_type_names.begin(), graph_type_names.end(), metaparams_grid[minElementIndex][9]); 
		best_graph = std::distance(graph_type_names.begin(), it);
		
		it = std::find (metric_type_names.begin(), metric_type_names.end(), metaparams_grid[minElementIndex][8]); 
		best_metric = std::distance(metric_type_names.begin(), it);
		
		it = std::find (distribution_type_names.begin(), distribution_type_names.end(), metaparams_grid[minElementIndex][7]); 
		best_distribution = std::distance(distribution_type_names.begin(), it);
												
		best_grid_size = std::stod(metaparams_grid[minElementIndex][0]);
		best_s_learn_rate = std::stod(metaparams_grid[minElementIndex][1]);
		best_f_learn_rate = std::stod(metaparams_grid[minElementIndex][2]);
		best_initial_neighbour_size = std::stod(metaparams_grid[minElementIndex][3]);
		best_neigbour_range_decay = std::stod(metaparams_grid[minElementIndex][4]);
		best_random_seed = std::stod(metaparams_grid[minElementIndex][5]);
		best_iterations = std::stod(metaparams_grid[minElementIndex][6]);
	
		std::cout << std::endl;
		std::cout << "The best configuration: " << std::endl;
		std::cout << "  Score: " << scores[minElementIndex] << std::endl;
		std::cout << "  Graph: " << graph_type_names[best_graph] << std::endl;
		std::cout << "  Distance: " << metric_type_names[best_metric] << std::endl;
		std::cout << "  Distribution: " << distribution_type_names[best_distribution] << std::endl;
		std::cout << "  Grid size: " << best_grid_size << std::endl;
		std::cout << "  Iterations: " << best_iterations << std::endl;
		std::cout << "  Start learn rate: " << best_s_learn_rate << std::endl;
		std::cout << "  Final learn rate: " << best_f_learn_rate << std::endl;
		std::cout << "  Initial neighbour size: " << best_initial_neighbour_size << std::endl;
		std::cout << "  Neigbour range decay: " << best_neigbour_range_decay << std::endl;
		std::cout << "  Random seeds: " << best_random_seed << std::endl;
		std::cout << std::endl;
	}

	// create, train SOM over the raw data and reduce the data

	metric::Cosine<double> distance;
	std::normal_distribution<double> distribution(-1, 1);
	metric::Grid6 graph(best_grid_size);

	metric::SOM<std::vector<double>, metric::Cosine<double>, metric::Grid6, std::normal_distribution<double>> som(
		distance, 
		graph, 
		distribution, 
		best_initial_neighbour_size, 
		best_neigbour_range_decay, 
		best_random_seed
	);	
	
	som.train(speeds, best_iterations, best_s_learn_rate, best_f_learn_rate);
	
	// clustering on the reduced data

	auto nodes_data = som.get_weights();
	json nodes_data_json(nodes_data);
	std::ofstream som_output("assets/som" + std::to_string(best_grid_size) + ".json");
	som_output << std::setw(4) << nodes_data_json << std::endl;
	som_output.close();	
	
    metric::Matrix<std::vector<double>, metric::Cosine<double>> distance_matrix(nodes_data);

    auto [assignments, exemplars, counts] = metric::affprop(distance_matrix, (float)0.66);

	
	std::cout << "assignments:" << std::endl;
	vector_print(assignments);
	std::cout << '\n';

	std::cout << "counts:" << std::endl;
	vector_print(counts);
	std::cout << '\n' << std::endl;


	// split and reshape raw data by clusters [cluster -> sensor -> energy -> values]
	
	std::vector<std::vector<std::vector<std::vector<double>>>> clustered_energies(8, std::vector<std::vector<std::vector<double>>>(7, std::vector<std::vector<double>>(counts.size())));
	int num_sensors = 8;
	int num_levels = 7;

	for (auto record : speeds)
	{
		// find cluster id for a record
		auto bmu = som.BMU(record);
		auto cluster_index = assignments[bmu];
		for (int i = 0; i < num_sensors; i++)
		{			
			for (int j = 0; j < num_levels; j++)
			{
				clustered_energies[i][j][cluster_index].push_back(record[i*num_levels + j]);
			}
		}
	}

	// calculate confs based on clustered energies and fill the result json

	std::vector<std::vector<std::vector<std::vector<std::vector<double>>>>> sensors(8, std::vector<std::vector<std::vector<std::vector<double>>>>(7));
	
	std::vector<std::string> conf_names = {"conf_l", "conf_m", "conf_r"};
	std::vector<std::string> sensor_names = {"vorne_li-1", "vorne_li-2", "vorne_li-3", "hinten_re-1", "vorne_re-1", "vorne_re-2", "vorne_re-3", "hinten_re-2"};
	std::vector<uint32_t> windowSizes = {12, 24, 48, 96, 192, 384};
	uint32_t samples = 5;
	double confidencelevel = 1.0;
	
	json reference_data;
	int sensor_index = 0;
	for (auto sensor_data : clustered_energies)
	{		
		std::vector<json> energies_json;
		int level_index = 0;
		for (auto energy_data : sensor_data)
		{			
			std::vector<json> clusters_json;
			for (auto cluster_data : energy_data)
			{
				// metric::PMQ set_0(energy_data);
			
				// returns quants for a single cluster
				std::vector<std::vector<std::vector<double>>> multiquants = set2multiconf(cluster_data, windowSizes, samples, confidencelevel);
					
				json cluster_json;
				for (auto window : multiquants)
				{
					json window_json = {
						{"conf_l", window[0]}, 						
						{"conf_m", window[1]},
						{"conf_r", window[2]}
					};
					cluster_json.push_back(window_json);
				}
				clusters_json.push_back(cluster_json);
			}
			json energy_json = {
				{"name", "level" + std::to_string(level_index)}, 						
				{"border", {}},
				{"position", {}},
				{"quant", clusters_json}
			};
			energies_json.push_back(energy_json);
			level_index++;
		}
		
		json sensor_json = {
			{"id", sensor_names[sensor_index]}, 		
			{"data", energies_json}
		};
		reference_data.push_back(sensor_json);
		sensor_index++;
	}
	std::ofstream outputFile("assets/reference_data.json");
	outputFile << std::setw(4) << reference_data << std::endl;
	outputFile.close();	


    return 0;
}
