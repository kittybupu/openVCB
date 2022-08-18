#include "Util.h"

namespace Gorder
{

unsigned long long MyRand64(){
	unsigned long long ret, tmp;
	ret=rand();
	ret=ret<<33;
	tmp=rand();
	ret=ret|(tmp<<2);
	tmp=rand();
	ret=ret|(tmp>>29);

	return ret;
}

string extractFilename(const char* filename){
	string name(filename);
	int pos=name.find_last_of('.');

	return name.substr(0, pos);
}

void quit(){
	system("pause");
	exit(0);
}

}
