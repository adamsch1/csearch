#include <vector>
#include <stdint.h>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include "SIMDCompressionAndIntersection/include/codecfactory.h"
#include "SIMDCompressionAndIntersection/include/intersection.h"

typedef std::vector<uint32_t> buffer;
typedef std::unordered_map<uint32_t,buffer> termdoc;

class IndexWriter {
public:

	IndexWriter() : flusher(0), buff(), capacity(1000), term(0) {}

	std::function<void(uint32_t term, buffer &buff)> flusher;
	buffer buff;
	unsigned capacity;
  uint32_t term;

	void write( uint32_t term, uint32_t doc ) {
		if( buff.size() == capacity || (term != this->term && this->term > 0)) {
			flusher(this->term, buff);
			buff.clear();
			this->term = term;
		}
		buff.push_back( doc );
	}

};

struct Kompressor {
	SIMDCompressionLib::IntegerCODEC &codec; 

	Kompressor() : codec( *SIMDCompressionLib::CODECFactory::getFromName("s4-bp128-dm") ) { }
	
	void compress( buffer &buff, buffer &out ) {
		// Resize outpt buffer to be sufficiently large
		out.resize( buff.size() + 1024 );
		// Set aside size()
		size_t compressedSize = out.size();
		// Compress.  compressedSize is set to how much data was really used in out 
		codec.encodeArray( buff.data(), buff.size(), out.data(), compressedSize );
		// Shrink and release extra data
		out.resize( compressedSize );
		out.shrink_to_fit();
	}

	void decompress( buffer &compressed, buffer &decompressed) {
		size_t recoveredSize = decompressed.size();
		codec.decodeArray( compressed.data(), compressed.size(), decompressed.data(), recoveredSize );
	}
};

class ForwardSorter {
public:

	ForwardSorter() : map(), size(0), capacity(1000), flusher(0) {}

	termdoc map;
  int size;
	int capacity;

	std::function<void(buffer &buff)> flusher;

	void sort() {
		buffer buff;
		// Copy all the keys into our buffer
		for( auto it=map.begin(); it != map.end(); ++it ) {
			buff.push_back( it->first );
		}
		// Sort the keys then walk in order flushing each
		std::sort( buff.begin(), buff.end());
		for( auto it=buff.begin(); it != buff.end(); ++it ) {
			flusher( map[*it]);
		}
		map.clear();
		size = 0;
	}

	void write( uint32_t term, uint32_t doc ) {

		if( size == capacity  ) {
			sort();
		}

		auto it = map.find( term );
		if( it != map.end() ) {
			it->second.push_back(doc);
		} else {
			map[term].push_back(doc);
		}
		size++;
	}

};

int main() {

	IndexWriter iw;

	Kompressor kk;

	iw.capacity = 10;
	iw.flusher = [&kk]( uint32_t term, buffer& buff ) {
		std::cout <<"IH "<< buff.size() <<  " " << term << std::endl;
		buffer outs;
		size_t size=0;
		kk.compress( buff, outs ); //, size );
		std::cout << "Size: " << 32.0 * static_cast<double>(outs.size()) / static_cast<double>(buff.size()) << std::endl;
		std::cout << "Compressed: " << size << " " << outs.size()<< " " << buff.size() << std::endl;

		buffer two;
		two.resize( buff.size() );
		kk.decompress( outs, two ); //, size );
		std::cout << "Result: " << (two == buff) << std::endl;
	};

	for( int k=0; k<11; k++ ) {
	  iw.write(1,k*3);
	}
	iw.write(1,200);
	iw.write(1,373);
	iw.write(1,377);

	ForwardSorter fs;

	fs.write( 1, 1);
	fs.write( 1, 2);
	fs.write( 2, 3);
}
