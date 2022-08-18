/*
MIT License

Copyright (c) 2016, Hao Wei.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _GRAPH_H
#define _GRAPH_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <queue>
#include <algorithm>
#include <utility>
#include <cmath>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdint>
#include <chrono>

#include "Util.h"
#include "UnitHeap.h"

namespace Gorder
{

using namespace std;

class Vertex{
public:
	int outstart;
	int outdegree;
	int instart;
	int indegree;

	Vertex(){
		outdegree=indegree=0;
		outstart=instart=-1;
	}
};

class Graph{
	public:
		int vsize;
		long long edgenum;
		string name;
		
		vector<Vertex> graph;
		vector<int> outedge;
		vector<int> inedge;
	
		string getFilename();
		void setFilename(string name);

		Graph();
		~Graph();
		void clear();

		void readGraph(vector<pair<int, int>>& edges, int vsize);
		void readGraph(const string& fullname);
		void writeGraph(ostream&);
		void PrintReOrderedGraph(const vector<int>& order);
		void GraphAnalysis();
		void RemoveDuplicate(const string& fullname);
		
		void strTrimRight(string& str);
		static vector<string> split(const string &s, char delim);
		static vector<string>& split(const string &s, char delim, vector<string> &elems);

		void GapCount();
		double GapCost(vector<int>& order);
		void Transform(vector<int>& order);
		void GorderGreedy(vector<int>& order, int window);

		void RCMOrder(vector<int>& order);
		unsigned long long LocalityScore(const int w);
};

}

#endif

