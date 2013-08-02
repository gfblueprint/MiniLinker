/*
 * sectionClass.h
 *
 *  Created on: Jul 27, 2013
 *      Author: ns
 */

#ifndef SECTIONCLASS_H_
#define SECTIONCLASS_H_

#include <vector>
#include "defines.h"
#include "elf.h"
#include "objectFileClass.h"

using namespace std;

class ObjectFile;

class ElfHeader
{
public:
	Elf64_Ehdr headerStruct;
	ObjectFile* belongTo;

	ElfHeader();

	void PrintInfomation();
	bool GetHeader( ObjectFile &obj );
};

class SectionHeader
{
	//一个.o文件的节头表
public:
	Elf64_Shdr *sectionHeaderPtr;
	ElfHeader *elfHead;
	int num;

	SectionHeader();
	bool GetSectionHeader( ElfHeader eh );
	void PrintSectionName( char *nameSecData );
	string GetSectionName( char *nameSecData,int idx );
};

class Section;
class Symbol
{
public:
	Elf64_Sym *symStr;		//符号表的原始信息
	string name;
	Section *belongToSec;

	int address;			//载入地址，由别人填写

	Symbol( Section *p );

	bool Init( Elf64_Sym *sym,char *pStrSec );
	void SetAddress( int addr );
};

class Section
{
	//.o文件的节集合
public:
	SectionHeader secHeader;
	vector<u8*> data;
	ElfHeader *elfHead;		//冗余，为了方便
	vector<Symbol> symArr;	//该.o文件中所有符号信息，为了处理方便，提取出来

	Section();
	bool GetSection( ElfHeader &eh );
	//提取出symbol信息,需在GetSection之后使用
	bool GetSymbol();
	void PrintSectionName();
	void PrintSymbols();
	string GetFileName();
};

#endif
