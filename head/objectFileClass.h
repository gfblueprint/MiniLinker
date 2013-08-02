/*
 * objectFileClass.h
 *
 *  Created on: Jul 27, 2013
 *      Author: ns
 */

#ifndef OBJECTFILECLASS_H_
#define OBJECTFILECLASS_H_

#include <string>
#include <elf.h>
#include "defines.h"
#include "sectionClass.h"
using namespace std;

class ObjectFile
{
	string sFileName_;
	u8 *bFilePtr_;
	int iLength_;
public:
	ObjectFile( string name );

//	bool SetObjectFile( string name );
//	bool GetElfHeader( ElfHeader &elfHead );
	bool IsValid();
	bool GetData( int offset,u8 *buf,int size );
	string GetName();
};

#endif
