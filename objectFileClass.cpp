/*
 * objectFile.cpp
 *
 *  Created on: Jul 27, 2013
 *      Author: ns
 */

#include <fstream>
#include <string>
#include <string.h>
#include <iostream>
#include "head/objectFileClass.h"

using namespace std;

ObjectFile::ObjectFile( string name )
{
	bFilePtr_ = NULL;

//	cout << name << endl;
	fstream fin;
	fin.open(name.c_str(),ios::binary | ios::in);

	if( !fin.is_open() )
		return ;
	sFileName_ = name;

	fin.seekg(0,ios::end);		//为了获取文件大小
	iLength_ = fin.tellg();
	fin.seekg(0,ios::beg);

	bFilePtr_ = new u8[iLength_];
	fin.read((char*)bFilePtr_,iLength_);
}

bool ObjectFile::IsValid()
{
	return bFilePtr_ == NULL ? false : true;
}

bool ObjectFile::GetData( int offset,u8 *buf,int size )
{
	if( !IsValid() )
		return false;
	if( offset+size > iLength_ )
		return false;
	memcpy(buf,bFilePtr_+offset,size);
	return true;
}

string ObjectFile::GetName()
{
	if( !IsValid() )
		return "";
	return sFileName_;
}
