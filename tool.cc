#include <iostream>
#include <string.h>
#include <algorithm>

#include "tool.h"

void test( std::string name, int step ) {
	std::fstream fs;
	fs.open(name, std::ios::out  | std::ios::binary);
	tool::ifile f(&fs);

//	f.fs->open(name, std::ios::out  | std::ios::binary);

	tool::tupe t;

	for( int k=0; k<10; k+=step ) {
		t.term = 1;
		t.doc = k;
		f.write( t );
	}
	f.flush();

}

void dtest() {
	std::fstream fs;
	fs.open( "EE", std::ios::out  | std::ios::binary);

	tool::document d;

	d.id = 123;
	d.c.push_back(1);
	d.c.push_back(2);

	d.write( &fs );
  fs.close();

	tool::document e;
	fs.open("EE", std::ios::in | std::ios::binary);
	e.read( &fs );

}

void rtest( std::string name ) {
	std::fstream fs;
	fs.open(name, std::ios::in  | std::ios::binary);
	tool::ifile f(&fs);
	tool::tupe t;

	while( f.read( t ) ) {
		std::cout << "DUDE: " << t.term << " " << t.doc << std::endl;
	}
}

void mtest() {
	std::fstream fs[3];
	std::fstream ofs;

	fs[0].open("A", std::ios::in  | std::ios::binary);
	fs[1].open("B", std::ios::in  | std::ios::binary);
	fs[2].open("C", std::ios::in  | std::ios::binary);

	ofs.open("D", std::ios::out | std::ios::binary);

	tool::ifile outs(&ofs);

	tool::ifile *b[3] = { new tool::ifile(&fs[0]), new tool::ifile(&fs[1]), new tool::ifile(&fs[2]) };
	tool::merge( b, 3, outs );

	delete( b[0] );
	delete( b[1] );
	delete( b[2] );
}

int main() {
	dtest();
	std::cout << "GO" << std::endl;
	test("A",2);
	test("B",3);
	test("C",4);


	rtest("A");
	mtest();
	//rtest("A");

	tool::chunk c;

	for( int k=1; k<4; k++ ) {
		c.push_back(k);
	}

	uint32_t bcount {0};
	auto buff = c.compress( bcount );
	std::cout << c.size() << std::endl;
	std::cout << bcount << std::endl;

	tool::chunk d;
	d.decompress( buff, 3);
	delete[] buff;

	for( int k=0; k<c.size(); k++ ) {
		std::cout << d[k] << std::endl;
	}


	return 0;
}
