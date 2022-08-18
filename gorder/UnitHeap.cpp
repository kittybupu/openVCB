/*
MIT License

Copyright (c) 2016, Hao Wei.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "UnitHeap.h"

namespace Gorder
{

UnitHeap::UnitHeap(int size){
	heapsize=size;
	if(size>0){
		Header.clear();
		Header.resize( max(size>>4, INITIALVALUE+4));
		LinkedList=new ListElement[size];
		update=new int[size];
	
		for(int i=0; i<size; i++){
			LinkedList[i].prev=i-1;
			LinkedList[i].next=i+1;
			LinkedList[i].key=INITIALVALUE;
			update[i]=-INITIALVALUE;
		}
		LinkedList[size-1].next=-1;
		
		Header[INITIALVALUE].first=0;
		Header[INITIALVALUE].second=size-1;
		top=0;
	}
}

UnitHeap::~UnitHeap(){
	delete[] LinkedList;
	delete[] update;
}

void UnitHeap::DeleteElement(const int index){
	int prev=LinkedList[index].prev;
	int next=LinkedList[index].next;
	int key=LinkedList[index].key;

	if(prev>=0)
		LinkedList[prev].next=next;
	if(next>=0)
		LinkedList[next].prev=prev;

	if(Header[key].first==Header[key].second){
		Header[key].first=Header[key].second=-1;
	}else if(Header[key].first==index){
		Header[key].first=next;
	}else if(Header[key].second==index){
		Header[key].second=prev;
	}

	if(top==index){
		top=LinkedList[top].next;
	}
	LinkedList[index].prev=LinkedList[index].next=-1;
}

int UnitHeap::ExtractMax(){
	int tmptop;
	do{
		tmptop=top;
		if(update[top]<0)
			DecreaseTop();
	}while(top!=tmptop);

	DeleteElement(tmptop);
	return tmptop;
}

void UnitHeap::DecrementKey(const int index){
	update[index]--;
}

void UnitHeap::DecreaseTop(){
	const int tmptop=top;
	const int key=LinkedList[tmptop].key;
	const int next=LinkedList[tmptop].next;
	int p=key;
	const int newkey=key+update[tmptop]-(update[tmptop]/2); /**/

	if(newkey<LinkedList[next].key){
		int tmp=LinkedList[Header[p].second].next;
		while(tmp>=0 && LinkedList[tmp].key>=newkey){
			p=LinkedList[tmp].key;
			tmp=LinkedList[Header[p].second].next;
		}
		LinkedList[next].prev=-1;
		const int psecond=Header[p].second;
		int tailnext=LinkedList[ psecond ].next;
		LinkedList[top].prev=psecond;
		LinkedList[top].next=tailnext;
		LinkedList[psecond].next=tmptop;
		if(tailnext>=0)
			LinkedList[tailnext].prev=tmptop;
		top=next;

		if(Header[key].first==Header[key].second)
			Header[key].first=Header[key].second=-1;
		else
			Header[key].first=next;


		LinkedList[tmptop].key=newkey;
		update[tmptop] /= 2; /**/
		Header[newkey].second=tmptop;
		if(Header[newkey].first<0)
			Header[newkey].first=tmptop;
	}

}

void UnitHeap::ReConstruct(){
	vector<int> tmp(heapsize);
	for(int i=0; i<heapsize; i++)
		tmp[i]=i;

	sort(tmp.begin(), tmp.end(), [&](const int a, const int b)->bool{
		if(LinkedList[a].key > LinkedList[b].key)
			return true;
		else
			return false;
	});

	int key=LinkedList[tmp[0]].key;
	LinkedList[tmp[0]].next=tmp[1];
	LinkedList[tmp[0]].prev=-1;
	LinkedList[tmp[tmp.size()-1]].next=-1;
	LinkedList[tmp[tmp.size()-1]].prev=tmp[tmp.size()-2];
	Header=vector<HeadEnd> (max(key+1, (int)Header.size()));
	Header[key].first=tmp[0];
	for(int i=1; i<tmp.size()-1; i++){
		int prev=tmp[i-1];
		int v=tmp[i];
		int next=tmp[i+1];
		LinkedList[v].prev=prev;
		LinkedList[v].next=next;

		int tmpkey=LinkedList[tmp[i]].key;
		if(tmpkey!=key){
			Header[key].second=tmp[i-1];
			Header[tmpkey].first=tmp[i];
			key=tmpkey;
		}
	}
	if(key==LinkedList[tmp[tmp.size()-1]].key)
		Header[key].second=tmp[tmp.size()-1];
	else{
		Header[key].second=tmp[tmp.size()-2];
		int lastone=tmp[tmp.size()-1];
		int lastkey=LinkedList[lastone].key;
		Header[lastkey].first=Header[lastkey].second=lastone;
	}
	top=tmp[0];
	
}

}
