/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2019 Panda Team
*/

#include <vector>

namespace clustering
{
	template <typename T>
	class Cluster {
	public:
		std::vector<std::vector<T>> data;
		std::vector<T> centroid;

		Cluster(const std::vector<std::vector<T>> &d)
		{
			data = d;
			calculateCentroid();
		}

		void calculateCentroid() {
			centroid = std::vector<T>(data[0].size());
			T sum;
			for (size_t i = 0; i < data[0].size(); i++)
			{
				sum = 0;
				for (size_t j = 0; j < data.size(); j++)
				{
					sum += data[j][i];
				}
				centroid[i] = (T) sum / data.size();
			}
		}
	};
	
	template <typename T, typename Distance>
	class HierarchicalClustering {
	private:

		std::vector<std::vector<double>> calculateDistances();

	protected:

	public:
		std::vector<Cluster<T>> clusters;
		std::vector<std::vector<T>> sourceData;
		int clustersNum;

		HierarchicalClustering(
			const std::vector<std::vector<T>> &data,
			const int &k)
		{
			sourceData = data;
			clustersNum = k;
		}

		void initialize();

		void hierarchical_clustering();

	}; // class HierarchicalClustering


} // namespace clustering

#include "hierarchClustering.cpp"