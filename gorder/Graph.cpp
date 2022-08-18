/*
MIT License

Copyright (c) 2016, Hao Wei.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "Graph.h"

#ifdef __GNUC__
#define likely(cond) __builtin_expect(!!(cond), 1)
#define unlikely(cond) __builtin_expect(!!(cond), 0)
#else
#define likely(cond) (!!(cond))
#define unlikely(cond) (!!(cond))
#endif // GNUC

namespace Gorder {

	string Graph::getFilename() {
		return name;
	}

	void Graph::setFilename(string name) {
		this->name.swap(name);
	}

	Graph::Graph() {
		edgenum = vsize = 0;
	}

	Graph::~Graph() {
	}

	void Graph::clear() {
		vsize = 0;
		edgenum = 0;
		name.clear();
		graph.clear();
		outedge.clear();
		inedge.clear();
	}

	void Graph::readGraph(vector<pair<int, int>>& edges, int vsize) {
		edgenum = edges.size();
		this->vsize = vsize;

		graph.resize(vsize + 1);
		for (long long i = 0; i < edges.size(); i++) {
			graph[edges[i].first].outdegree++;
			graph[edges[i].second].indegree++;
		}
		graph[0].outstart = 0;
		graph[0].instart = 0;
		for (int i = 1; i < vsize; i++) {
			graph[i].outstart = graph[i - 1].outstart + graph[i - 1].outdegree;
			graph[i].instart = graph[i - 1].instart + graph[i - 1].indegree;
		}

		sort(edges.begin(), edges.end(), [](const pair<int, int>& a, const pair<int, int>& b)->bool {
			if (a.first < b.first)
				return true;
			else if (a.first > b.first)
				return false;
			else {

				if (a.second <= b.second)
					return true;
				else
					return false;
			}

			});
		outedge.resize(edgenum);
		for (long long i = 0; i < edges.size(); i++) {
			outedge[i] = edges[i].second;
		}

		vector< pair<int, int> >().swap(edges);
#ifndef Release	
		vector<int> inpos(vsize);
		for (int i = 0; i < vsize; i++) {
			inpos[i] = graph[i].instart;
		}
		inedge.resize(edgenum);
		for (int u = 0; u < vsize; u++) {
			for (int j = graph[u].outstart; j < graph[u].outstart + graph[u].outdegree; j++) {
				inedge[inpos[outedge[j]]] = u;
				inpos[outedge[j]]++;
			}
		}
#endif

		// cout << "vsize: " << vsize << endl;
		// cout << "edgenum: " << edgenum << endl;
		graph[vsize].outstart = edgenum;
		graph[vsize].instart = edgenum;
	}

	void Graph::readGraph(const string& fullname) {
		FILE* fp;
		fopen_s(&fp, fullname.c_str(), "r");
		if (fp == NULL) {
			cout << "Fail to open " << fullname << endl;
			quit();
		}

		char line[40];
		int u, v;
		const char* str = NULL;

		vsize = 0;
		edgenum = 0;
		vector< pair<int, int> > edges;
		edges.reserve(100000000);

		while (feof(fp) != true) {
			if (fgets(line, 40, fp)) {
				u = v = 0;
				str = line;
				while (isdigit(*str))
					u = u * 10 + (*str++ - '0');
				str++;
				while (isdigit(*str))
					v = v * 10 + (*str++ - '0');

				if (u == v)
					continue;
				edgenum++;
				if (u > vsize)
					vsize = u;
				if (v > vsize)
					vsize = v;

				edges.push_back(make_pair(u, v));
			}
		}
		vsize++;

		fclose(fp);
		graph.resize(vsize + 1);
		for (long long i = 0; i < edges.size(); i++) {
			graph[edges[i].first].outdegree++;
			graph[edges[i].second].indegree++;
		}
		graph[0].outstart = 0;
		graph[0].instart = 0;
		for (int i = 1; i < vsize; i++) {
			graph[i].outstart = graph[i - 1].outstart + graph[i - 1].outdegree;
			graph[i].instart = graph[i - 1].instart + graph[i - 1].indegree;
		}

		sort(edges.begin(), edges.end(), [](const pair<int, int>& a, const pair<int, int>& b)->bool {
			if (a.first < b.first)
				return true;
			else if (a.first > b.first)
				return false;
			else {

				if (a.second <= b.second)
					return true;
				else
					return false;
			}

			});
		outedge.resize(edgenum);
		for (long long i = 0; i < edges.size(); i++) {
			outedge[i] = edges[i].second;
		}

		vector< pair<int, int> >().swap(edges);
#ifndef Release	
		vector<int> inpos(vsize);
		for (int i = 0; i < vsize; i++) {
			inpos[i] = graph[i].instart;
		}
		inedge.resize(edgenum);
		for (int u = 0; u < vsize; u++) {
			for (int j = graph[u].outstart; j < graph[u].outstart + graph[u].outdegree; j++) {
				inedge[inpos[outedge[j]]] = u;
				inpos[outedge[j]]++;
			}
		}
#endif

		cout << "vsize: " << vsize << endl;
		cout << "edgenum: " << edgenum << endl;
		graph[vsize].outstart = edgenum;
		graph[vsize].instart = edgenum;
	}

	void Graph::Transform(vector<int>& order) {
		RCMOrder(order);
		if (order.size() != vsize) {
			cout << "order.size()!=vsize" << endl;
			quit();
		}
		if (graph.size() != (vsize + 1)) {
			cout << "graph.size()!=(vsize+1)" << endl;
			quit();
		}

		vector<int>().swap(inedge);
		vector< pair<int, int> > edges;
		edges.reserve(edgenum);
		for (int i = 0; i < vsize; i++) {
			for (int j = graph[i].outstart, limit = graph[i + 1].outstart; j < limit; j++)
				edges.push_back(make_pair(order[i], order[outedge[j]]));
		}
		if (edges.size() != edgenum) {
			cout << "edges.size()!=edgenum" << endl;
			quit();
		}

		for (int i = 0; i < vsize; i++) {
			graph[i].outdegree = graph[i].indegree = 0;
		}
		for (int i = 0; i < edges.size(); i++) {
			graph[edges[i].first].outdegree++;
			graph[edges[i].second].indegree++;
		}

		graph[0].outstart = 0;
		graph[0].instart = 0;
		for (int i = 1; i < vsize; i++) {
			graph[i].outstart = graph[i - 1].outstart + graph[i - 1].outdegree;
			graph[i].instart = graph[i - 1].instart + graph[i - 1].indegree;
		}
		graph[vsize].outstart = edgenum;
		graph[vsize].instart = edgenum;

		sort(edges.begin(), edges.end(), [](const pair<int, int>& a, const pair<int, int>& b)->bool {
			if (a.first < b.first)
				return true;
			else if (a.first > b.first)
				return false;
			else {

				if (a.second <= b.second)
					return true;
				else
					return false;
			}
			});

		outedge.resize(edgenum);
		for (long long i = 0; i < edges.size(); i++) {
			outedge[i] = edges[i].second;
		}
		vector< pair<int, int> >().swap(edges);
		vector<int> inpos(vsize);
		for (int i = 0; i < vsize; i++) {
			inpos[i] = graph[i].instart;
		}
		inedge.resize(edgenum);
		for (int u = 0; u < vsize; u++) {
			for (int j = graph[u].outstart; j < graph[u].outstart + graph[u].outdegree; j++) {
				inedge[inpos[outedge[j]]] = u;
				inpos[outedge[j]]++;
			}
		}
	}


	void Graph::writeGraph(ostream& out) {
		for (int u = 0; u < vsize; u++) {
			for (int j = graph[u].outstart; j < graph[u].outdegree + graph[u].outstart; j++) {
				int v = outedge[j];
				out << u << '\t' << v << endl;
			}
		}
	}


	void Graph::PrintReOrderedGraph(const vector<int>& order) {
		ofstream out((name + "_Gorder.txt").c_str());

		vector<int>().swap(inedge);

		vector< vector<int> > ReOrderedGraph(vsize);
		int u, v;
		for (int i = 0; i < vsize; i++) {
			u = order[i];
			ReOrderedGraph[u].reserve(graph[i + 1].outstart - graph[i].outstart);
			for (int j = graph[i].outstart; j < graph[i].outstart + graph[i].outdegree; j++) {
				v = order[outedge[j]];
				ReOrderedGraph[u].push_back(v);
			}
			sort(ReOrderedGraph[u].begin(), ReOrderedGraph[u].end());
		}
		/*
			for(int u=0; u<vsize; u++){
				sort(ReOrderedGraph[u].begin(), ReOrderedGraph[u].end());
			}
		*/
		for (int u = 0; u < vsize; u++) {
			for (int j = 0; j < ReOrderedGraph[u].size(); j++) {
				out << u << '\t' << ReOrderedGraph[u][j] << endl;
			}
		}
		out.close();
	}


	void Graph::GraphAnalysis() {
		vector<int> tmp(vsize);
		for (int i = 0; i < vsize; i++) {
			tmp[i] = graph[i].outdegree;
		}
		sort(tmp.begin(), tmp.end());

		cout << "outdegree:" << endl;
		vector<int>::iterator tmpit1, tmpit2;
		tmpit1 = tmp.begin();
		for (int i = 1; i < vsize; i *= 10) {
			tmpit2 = tmpit1;
			tmpit1 = upper_bound(tmp.begin(), tmp.end(), i);
			cout << i << ": " << tmpit1 - tmpit2 << endl;
		}


		for (int i = 0; i < vsize; i++) {
			tmp[i] = graph[i].indegree;
		}
		sort(tmp.begin(), tmp.end());

		cout << "indegree:" << endl;
		tmpit1 = tmp.begin();
		for (int i = 1; i < vsize; i *= 10) {
			tmpit2 = tmpit1;
			tmpit1 = upper_bound(tmp.begin(), tmp.end(), i);
			cout << i << ": " << tmpit1 - tmpit2 << endl;
		}
	}


	void Graph::RemoveDuplicate(const string& fullname) {
		FILE* fp;
		fopen_s(&fp, fullname.c_str(), "r");
		if (fp == NULL) {
			cout << "Fail to open " << fullname << endl;
			quit();
		}

		char line[40];
		int u, v;
		const char* str = NULL;

		set< pair<int, int> > edges;

		while (feof(fp) != true) {
			if (fgets(line, 40, fp)) {
				u = v = 0;
				str = line;
				while (isdigit(*str))
					u = u * 10 + (*str++ - '0');
				str++;
				while (isdigit(*str))
					v = v * 10 + (*str++ - '0');

				if (u == v)
					continue;

				edges.insert(make_pair(u, v));
			}
		}

		fclose(fp);

		cout << "after remove, the size is " << edges.size() << endl;

		ofstream fout;
		fout.open("NoDuplicate.txt");
		for (set< pair<int, int> >::iterator it = edges.begin(); it != edges.end(); it++) {
			fout << it->first << '\t' << it->second << endl;
		}
		fout.close();
	}


	vector<string>& Graph::split(const string& s, char delim, vector<string>& elems) {
		int begin, end;

		begin = 0;
		end = s.find(delim);
		while (end != string::npos) {
			elems.push_back(s.substr(begin, end - begin));
			begin = end + 1;
			end = s.find(delim, begin);
		}
		if (begin != s.size()) {
			elems.push_back(s.substr(begin));
		}

		return elems;
	}

	vector<string> Graph::split(const string& s, char delim) {
		vector<string> elems;
		return split(s, delim, elems);
	}

	void Graph::strTrimRight(string& str) {
		string whitespaces(" \t\r\n");
		int index = str.find_last_not_of(whitespaces);
		if (index != string::npos)
			str.erase(index + 1);
		else
			str.clear();
	}

	void Graph::GapCount() {
		int* gap = new int[vsize];
		memset(gap, 0, sizeof(int) * vsize);

		for (int i = 0; i < vsize; i++) {
			for (int j = graph[i].outstart + 1; j < graph[i].outdegree + graph[i].outstart; j++) {
				gap[outedge[j] - outedge[j - 1]]++;
			}
			gap[outedge[graph[i].outstart]]++;
		}

		double entropy = 0;
		for (int i = 0; i < vsize; i++) {
			if (gap[i] == 0)
				continue;
			else {
				entropy += (double)gap[i] / edgenum * log2((double)gap[i] / edgenum);
			}
		}
		cout << "shannon: " << entropy << endl;
		delete[] gap;
	}

	double Graph::GapCost(vector<int>& order) {
		double gaplog = 0;
		double gaplog2 = 0;
		vector<int> edgelist;
		edgelist.reserve(100000);
		for (int i = 0; i < vsize; i++) {
			for (int j = graph[i].outstart + 1; j < graph[i].outdegree + graph[i].outstart; j++) {
				if (outedge[j] - outedge[j - 1])
					gaplog += log(double(outedge[j] - outedge[j - 1])) / log(double(2));
			}
			edgelist.clear();
			for (int j = graph[i].outstart; j < graph[i].outstart + graph[i].outdegree; j++) {
				edgelist.push_back(order[outedge[j]]);
			}
			sort(edgelist.begin(), edgelist.end());
			for (int j = 1; j < edgelist.size(); j++) {
				if (edgelist[j] - edgelist[j - 1])
					gaplog2 += log(double(edgelist[j] - edgelist[j - 1])) / log(double(2));
			}
		}
		cout << "original average gap cost: " << gaplog / edgenum << endl;
		cout << "new average gap cost: " << gaplog2 / edgenum << endl;

		return gaplog2 / edgenum;
	}


	void Graph::GorderGreedy(vector<int>& retorder, int window) {
		UnitHeap unitheap(vsize);
		vector<bool> popvexist(vsize, false);
		vector<int> order;
		int count = 0;
		vector<int> zero;
		zero.reserve(10000);
		order.reserve(vsize);
		const int hugevertex = sqrt((double)vsize);

		for (int i = 0; i < vsize; i++) {
			unitheap.LinkedList[i].key = graph[i].indegree;
			unitheap.update[i] = -graph[i].indegree;
		}
		unitheap.ReConstruct();

		int tmpindex, tmpweight;

		tmpweight = -1;
		for (int i = 0; i < vsize; i++) {
			if (graph[i].indegree > tmpweight) {
				tmpweight = graph[i].indegree;
				tmpindex = i;
			}
			else if (graph[i].indegree + graph[i].outdegree == 0) {
				unitheap.update[i] = INT_MAX / 2;
				zero.push_back(i);
				unitheap.DeleteElement(i);
			}
		}

		order.push_back(tmpindex);
		unitheap.update[tmpindex] = INT_MAX / 2;
		unitheap.DeleteElement(tmpindex);
		for (int i = graph[tmpindex].instart, limit1 = graph[tmpindex + 1].instart; i < limit1; i++) {
			int u = inedge[i];
			if (graph[u].outdegree <= hugevertex) {
				if (unitheap.update[u] == 0) {
					unitheap.IncrementKey(u);
				}
				else {
#ifndef Release
					if (unitheap.update[u] == INT_MAX)
						unitheap.update[u] = INT_MAX / 2;
#endif
					unitheap.update[u]++;
				}

				if (graph[u].outdegree > 1)
					for (int j = graph[u].outstart, limit2 = graph[u + 1].outstart; j < limit2; j++) {
						int w = outedge[j];
						if (unitheap.update[w] == 0) {
							unitheap.IncrementKey(w);
						}
						else {
#ifndef Release
							if (unitheap.update[w] == INT_MAX)
								unitheap.update[w] = INT_MAX / 2;
#endif
							unitheap.update[w]++;
						}

					}
			}
		}
		if (graph[tmpindex].outdegree <= hugevertex) {
			for (int i = graph[tmpindex].outstart, limit1 = graph[tmpindex + 1].outstart; i < limit1; i++) {
				int w = outedge[i];
				if (unitheap.update[w] == 0) {
					unitheap.IncrementKey(w);
				}
				else {
#ifndef Release
					if (unitheap.update[w] == INT_MAX)
						unitheap.update[w] = INT_MAX / 2;
#endif
					unitheap.update[w]++;
				}

			}
		}

#ifndef Release
		clock_t time1, time2, time3, time4;
		clock_t sum1 = 0, sum2 = 0, sum3 = 0;
#endif

		while (count < vsize - 1 - zero.size()) {
#ifndef Release
			if (count % 1000000 == 0) {
				// cout << count << endl;
				// cout << "sum1: " << sum1 << endl;
				// cout << "sum2: " << sum2 << endl;
				// cout << "sum3: " << sum3 << endl;
				// cout << endl;
				sum1 = sum2 = sum3 = 0;
			}

			time1 = clock();
#endif

			int v = unitheap.ExtractMax();
			count++;
#ifndef Release
			time2 = clock();
#endif
			order.push_back(v);
			unitheap.update[v] = INT_MAX / 2;

			int popv;
			if (count - window >= 0)
				popv = order[count - window];
			else
				popv = -1;

			if (popv >= 0) {
				if (graph[popv].outdegree <= hugevertex) {
					for (int i = graph[popv].outstart, limit1 = graph[popv + 1].outstart; i < limit1; i++) {
						int w = outedge[i];
						unitheap.update[w]--;
#ifndef Release
						if (unitheap.update[w] == 0)
							unitheap.update[w] = INT_MAX / 2;
#endif
					}
				}

				for (int i = graph[popv].instart, limit1 = graph[popv + 1].instart; i < limit1; i++) {
					int u = inedge[i];
					if (graph[u].outdegree <= hugevertex) {
						unitheap.update[u]--;
#ifndef Release
						if (unitheap.update[u] == 0)
							unitheap.update[u] = INT_MAX / 2;
#endif
						if (graph[u].outdegree > 1)
							if (binary_search(outedge.data() + graph[u].outstart, outedge.data() + graph[u + 1].outstart, v) == false) {
								for (int j = graph[u].outstart, limit2 = graph[u + 1].outstart; j < limit2; j++) {
									int w = outedge[j];
									unitheap.update[w]--;
#ifndef Release
									if (unitheap.update[w] == 0)
										unitheap.update[w] = INT_MAX / 2;
#endif
								}
							}
							else {
								popvexist[u] = true;
							}
					}
				}
			}

#ifndef Release
			time3 = clock();
#endif
			if (graph[v].outdegree <= hugevertex) {
				for (int i = graph[v].outstart, limit1 = graph[v + 1].outstart; i < limit1; i++) {
					int w = outedge[i];
					if (unlikely(unitheap.update[w] == 0)) {
						unitheap.IncrementKey(w);
					}
					else {
#ifndef Release
						if (unitheap.update[w] == INT_MAX)
							unitheap.update[w] = INT_MAX / 2;
#endif
						unitheap.update[w]++;
					}

				}
			}

			for (int i = graph[v].instart, limit1 = graph[v + 1].instart; i < limit1; i++) {
				int u = inedge[i];
				if (graph[u].outdegree <= hugevertex) {
					if (unlikely(unitheap.update[u] == 0)) {
						unitheap.IncrementKey(u);
					}
					else {
#ifndef Release
						if (unitheap.update[u] == INT_MAX)
							unitheap.update[u] = INT_MAX / 2;
#endif
						unitheap.update[u]++;
					}

					if (popvexist[u] == false) {
						if (graph[u].outdegree > 1)
							for (int j = graph[u].outstart, limit2 = graph[u + 1].outstart; j < limit2; j++) {
								int w = outedge[j];
								if (unlikely(unitheap.update[w] == 0)) {
									unitheap.IncrementKey(w);
								}
								else {
#ifndef Release
									if (unitheap.update[w] == INT_MAX)
										unitheap.update[w] = INT_MAX / 2;
#endif
									unitheap.update[w]++;
								}
							}
					}
					else {
						popvexist[u] = false;
					}
				}
			}


#ifndef Release
			time4 = clock();
			sum1 += time2 - time1;
			sum2 += time3 - time2;
			sum3 += time4 - time3;
#endif
		}
		order.insert(order.end() - 1, zero.begin(), zero.end());


#ifndef Release
		vector<int> tmporder = order;
		sort(tmporder.begin(), tmporder.end());
		for (int i = 0; i < tmporder.size() - 1; i++) {
			if (tmporder[i] == tmporder[i + 1]) {
				cout << "same elements: " << tmporder[i] << endl;
				system("pause");
			}
		}
		for (int i = 0; i < tmporder.size(); i++) {
			if (tmporder[i] != i) {
				cout << tmporder[i] << '\t' << i << endl;
				system("pause");
			}
		}
		tmporder = vector<int>();
#endif

		retorder.clear();
		retorder.resize(vsize);
		for (int i = 0; i < vsize; i++) {
			retorder[order[i]] = i;
		}
	}


	void Graph::RCMOrder(vector<int>& retorder) {
		queue<int> que;
		bool* BFSflag = new bool[vsize];
		bool* QueFlag = new bool[vsize];
		memset(BFSflag, 0, sizeof(bool) * vsize);
		memset(QueFlag, 0, sizeof(bool) * vsize);

		vector<int> tmp;
		vector<int> degreevertex(vsize);
		for (int i = 0; i < vsize; i++) {
			degreevertex[i] = i;
		}

		sort(degreevertex.begin(), degreevertex.end(), [&](const int& a, const int& b)->bool {
			if (graph[a].outdegree + graph[a].indegree < graph[b].outdegree + graph[b].indegree)
				return true;
			else
				return false;
			});

		int now;
		vector<int> order;

		for (int k = 0; k < vsize; k++) {
			int i = degreevertex[k];
			if (BFSflag[i] == false) {
				que.push(i);
				//			QueFlag[i]=true;
				BFSflag[i] = true;
				order.push_back(i);

				while (que.empty() == false) {
					now = que.front();
					que.pop();

					//				BFSflag[now]=true;
					tmp.clear();
					for (int it = graph[now].outstart, limit = graph[now + 1].outstart; it < limit; it++) {
						tmp.push_back(outedge[it]);
					}
					sort(tmp.begin(), tmp.end(), [&](const int& a, const int& b)->bool {
						if (graph[a].outdegree + graph[a].indegree < graph[b].outdegree + graph[b].indegree)
							return true;
						else
							return false;
						});
					if (tmp.size() != graph[now].outdegree)
						cout << "tmp.size()!=graph[now].outdegree" << endl;

					for (int i = 0; i < tmp.size(); i++) {
						//					if((BFSflag[tmp[i]]==false)&&(QueFlag[tmp[i]]==false)){
						if (BFSflag[tmp[i]] == false) {
							que.push(tmp[i]);
							BFSflag[tmp[i]] = true;
							order.push_back(tmp[i]);
						}
					}
				}
			}
		}

		delete[] BFSflag;
		delete[] QueFlag;

		if (order.size() != vsize) {
			cout << "order.size()!=vsize" << endl;
			quit();
		}

		retorder.resize(vsize);
		for (int i = 0; i < order.size(); i++) {
			retorder[order[i]] = order.size() - 1 - i;
		}
	}


	unsigned long long Graph::LocalityScore(const int w) {
		unsigned long long sum = 0;
		for (int i = 0; i < vsize; i++) {
			for (int j = i - 1; j >= i - w && j >= 0; j--) {
				sum += IntersectionSize(inedge.data() + graph[i].instart, inedge.data() + graph[j].instart, graph[i].indegree, graph[j].indegree, -1);
				if (binary_search(inedge.data() + graph[i].instart, inedge.data() + graph[i].instart + graph[i].indegree, j))
					sum++;
				if (binary_search(inedge.data() + graph[j].instart, inedge.data() + graph[j].instart + graph[j].indegree, i))
					sum++;
			}
		}
		return sum;
	}

}
