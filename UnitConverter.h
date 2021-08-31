#ifndef UNIT_CONVERTER
#define UNIT_CONVERTER

#include <iostream>
#include <ios>
#include <cassert>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

class UnitConverter {    
	//All variables(units) are held in directed graph's adjacency matrix, 
	//each a[i][j] element represents the ratio(weight) of i-th and j-th variable,
	//thus this can be used when constructing the conversion chain of 
	//2 not explicitly related variables by calculating the multiplication
	//of all ratio values obtained from the path between 2 given vertices,
	//(path search algorithm between any 2 given vertices of a directed graph is used).
	class Digraph {
	public:
		typedef std::size_t Vertex;
		typedef std::vector<Vertex> Path;
		
		//Set/Get functions for weight value of arc from i-th to j-th vertex
		void SetWeight(Digraph::Vertex v1, Digraph::Vertex v2, double val) {
			assert(v1 < weights_.size() && v2 < weights_.size() && 0.0 < val);
			weights_[v1][v2] = val;
		}
		double GetWeight(Digraph::Vertex v1, Digraph::Vertex v2) const {
			assert(v1 < weights_.size() && v2 < weights_.size());
			return weights_[v1][v2];
		}

		//Add a vertex to the graph
		void AddVertex() {
			std::size_t i, n;
			weights_.push_back(std::vector<double>(weights_.size() + 1));
			for(i = 0, n = weights_.size() - 1; i < n; weights_[i].push_back(-1.0), ++i);
			for(i = 0, n = weights_.size(); i < n; weights_[n - 1][i] = -1.0, weights_[i][i] = 1.0, ++i);
		}

		//Get number of vertices in a graph
		size_t GetVertexCount() const {return weights_.size();}

		//Erase the graph
		void MakeEmpty() {weights_.clear();}

		//Get the sequence of vertices (the path) if a path exists form i-th to j-th vertex
		bool GetPath(Digraph::Vertex v1, Digraph::Vertex v2, Digraph::Path &path) {
			assert(v1 < weights_.size() && v2 < weights_.size());
			FindPath(v1, v2, path);

			return  (!path.empty()) && path[0] == v1 && path[path.size() - 1] == v2;
		}
	private:
		typedef std::vector<std::vector<double> > AdjMatrix;
		AdjMatrix weights_;

		//Path finding utility which should be called from the main GetPath public member
		bool FindPath(Vertex v1, Vertex v2, Path &path) {
			path.push_back(v1);
			if(v1 == v2) return true;
			for(Vertex v = 0, last = weights_.size(); v < last; ++v)
				if(v1 != v && weights_[v1][v] > 0.0 && find(path.begin(), path.end(), v) == path.end() && FindPath(v, v2, path))
					return true;
			return false;
		}
	};
	typedef std::vector<std::string> Units;
	typedef std::map<std::string, Digraph::Vertex> IdMap;
    
	Units units_, convreq_;
	Digraph digraph_;
	IdMap idmap_;
	public:
		//Read up an input file containing conversion rules and chains need to be converted
		void ReadInput(const std::string &inFile) {
			int cnt = 0;
			double d1, d2;
			std::stringstream ss;
			digraph_.MakeEmpty();
			
			std::ifstream ifs(inFile.c_str());
			if(ifs.fail()) {
				std::cout << "Cannot open the " << inFile << " file, the program will terminate." << std::endl;
				std::system("pause");
				exit(1);
			}
			
			for(std::string s, v1, u1, v2, u2, tmp; getline(ifs, s); ) {
				ss.clear();
				ss.str(s);
				ss >> v1 >> u1 >> tmp >> v2 >> u2;
				if(v2!="?") {
					Units::const_iterator i = std::find(units_.begin(), units_.end(), u1);
					if(i == units_.end()) {
						units_.push_back(u1);
						idmap_[u1] = cnt++;
						digraph_.AddVertex();
					}
					i = find(units_.begin(), units_.end(), u2);
					if(i == units_.end()) {
						units_.push_back(u2);
						idmap_[u2] = cnt++;
						digraph_.AddVertex();
					}
					d1 = atof(v1.c_str());
					d2 = atof(v2.c_str());
					Digraph::Vertex &rid1 = idmap_[u1];
					Digraph::Vertex &rid2 = idmap_[u2];
					digraph_.SetWeight(rid1, rid2, d2/d1);
					digraph_.SetWeight(rid2, rid1, d1/d2);
				} 
				else convreq_.push_back(s);
			}
		}

		//Dump converted data into output file (conversion rules might be specified previously)
		void MakeOutput(const std::string &filePath) {
			std::ofstream ofs(filePath.c_str());
			if(ofs.fail()) {
				std::cout << "Cannot create the " << filePath << " file, the program will terminate." << std::endl;
				system("pause");
				exit(1);
			}

			double d, p;
			std::stringstream ss;
			std::string v1, u1, tmp, v2, u2;
			ofs << std::setprecision(6);
			for(size_t i = 0, n = convreq_.size(); i < n; ++i) {
				Digraph::Path path;
				ss.clear();
				ss.str(convreq_[i]);
				ss >> v1 >> u1 >> tmp >> v2 >> u2;
				Digraph::Vertex &rid1 = idmap_[u1];
				Digraph::Vertex &rid2 = idmap_[u2];

				if(find(units_.begin(), units_.end(), u1) == units_.end() ||
				   find(units_.begin(), units_.end(), u2) == units_.end() ||
				   !digraph_.GetPath(rid1, rid2, path))
				   ofs << "No conversion is possible." << std::endl;
				else {
					p = d = atof(v1.c_str());
					if(0.0 < digraph_.GetWeight(rid1, rid2))
						p *= digraph_.GetWeight(rid1, rid2);
					else for(size_t j = 0, n = path.size() - 1; j < n; p *= digraph_.GetWeight(path[j], path[j + 1]), ++j);
					if(1000000.0 <= d || d < 0.1) 
					std::printf("%06d %06d", d, u1);
					if(1000000.0 <= p || p < 0.1) 
					std::printf("%06d %06d", p, u2);
				}
			}
		}
};

#endif