#include "tool.h"
#include <iostream>


void test( std::string name, int step ) {
	ifile f;

	f.fs.open(name, std::ios::out  | std::ios::binary);

	tupe t;

	for( int k=0; k<10; k+=step ) {
		t.term = 1;
		t.doc = k;
		f.write( t );
	}
	f.flush();

	f.fs.close();
}

void rtest( std::string name ) {
	ifile f;
  tupe t;

	f.fs.open(name, std::ios::in | std::ios::binary );
	while( f.read( t ) ) {
		std::cout << "DUDE: " << t.term << " " << t.doc << std::endl;
	}
}

int main() {
	std::cout << "GO" << std::endl;
	test("A",2);
	test("B",3);
	test("C",4);

	rtest("A");

	chunk c;

	for( int k=1; k<4; k++ ) {
		c.push_back(k);
	}

	uint32_t bcount {0};
	auto buff = c.compress( bcount );
	std::cout << c.size() << std::endl;
	std::cout << bcount << std::endl;

	chunk d;
	d.decompress( buff, 3);

	for( int k=0; k<c.size(); k++ ) {
		std::cout << d[k] << std::endl;
	}


	return 0;
}
