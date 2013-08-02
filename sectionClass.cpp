/*
 * sectionClass.cpp
 *
 *  Created on: Jul 27, 2013
 *      Author: ns
 */

#include <iostream>
#include <vector>
#include "head/defines.h"
#include "head/sectionClass.h"

using namespace std;

ElfHeader::ElfHeader()
{
	belongTo = NULL;
}

bool ElfHeader::GetHeader( ObjectFile &obj )
{
	if( !obj.IsValid() )
		return false;

	if( obj.GetData(0,(u8*)&headerStruct,sizeof(Elf64_Ehdr)) )
	{
		belongTo = &obj;
		return true;
	}
	else
		return false;
}

void ElfHeader::PrintInfomation()
{
	if( belongTo == NULL )
	{
		cout << "Dosen't belong to any one" << endl;
		return ;
	}

	cout << "Printing " << belongTo->GetName() << "'s elf header infomation" << endl;

	cout << "Magic:" << hex;
	for( unsigned int i = 0;i < sizeof(headerStruct.e_ident);++i )
		cout << headerStruct.e_ident[i] << " ";
	cout << endl << dec;

	cout << "Type:" << headerStruct.e_type << endl;
	cout << "Machine:" << headerStruct.e_machine << endl;
	cout << "Version:" << headerStruct.e_version << endl;
	cout << "Entry:" << headerStruct.e_entry << endl;
	cout << "P offset:" << headerStruct.e_phoff << endl;
	cout << "S offset:" << headerStruct.e_shoff << endl;
	cout << "flags:" << headerStruct.e_flags << endl;
	cout << "Elf header size:" << headerStruct.e_ehsize << endl;
	cout << "Phent size:" << headerStruct.e_phentsize << endl;
	cout << "Phent num:" << headerStruct.e_phnum << endl;
	cout << "Section head size:" << headerStruct.e_shentsize << endl;
	cout << "Section num:" << headerStruct.e_shnum << endl;
	cout << "Section name sym index:" << headerStruct.e_shstrndx << endl;
}

SectionHeader::SectionHeader()
{
	num = 0;
	sectionHeaderPtr = NULL;
	elfHead = NULL;
}

bool SectionHeader::GetSectionHeader( ElfHeader eh )
{
	if( eh.belongTo == NULL )
		return false;

	ObjectFile *body = eh.belongTo;

	sectionHeaderPtr = new Elf64_Shdr[eh.headerStruct.e_shnum];
	if( ! body->GetData(eh.headerStruct.e_shoff,(u8*)sectionHeaderPtr
			,eh.headerStruct.e_shentsize*eh.headerStruct.e_shnum) )
	{
		delete sectionHeaderPtr;
		sectionHeaderPtr = NULL;
		return false;
	}

	num = eh.headerStruct.e_shnum;
	elfHead = &eh;
	return true;
}

string SectionHeader::GetSectionName( char *nameSecData,int idx )
{
	if( sectionHeaderPtr == NULL )
		return "";
	int nameOffset = sectionHeaderPtr[idx].sh_name;
	return string( nameSecData + nameOffset );
}

void SectionHeader::PrintSectionName( char *nameSecData )
{
	for( int i = 0;i < num;++i )
		cout << GetSectionName( nameSecData,i ) << endl;
}

Symbol::Symbol( Section *p )
{
	belongToSec = p;
	symStr = NULL;
	address = -1;
}

bool Symbol::Init( Elf64_Sym *sym,char *pStrSec )
{
	if( belongToSec == NULL || sym == NULL || pStrSec == NULL )
		return false;
	symStr = sym;

	name = string( (char*)(pStrSec+sym->st_name) );

	return true;
}

void Symbol::SetAddress( int addr )
{
	address = addr;
}

Section::Section()
{
	elfHead = NULL;
}

bool Section::GetSection( ElfHeader &eh )
{
	if( !secHeader.GetSectionHeader(eh) )
		return false;

	//开始读入节的内容
	ObjectFile *body = eh.belongTo;
	for( int i = 0;i < secHeader.num;++i )
	{
		int size = secHeader.sectionHeaderPtr[i].sh_size;
		int offset = secHeader.sectionHeaderPtr[i].sh_offset;
		/*
		 * 注意：此处对于NOBITS类型，即bss类型也分配了空间
		 * 这些空间会被写入到最后的bin文件中
		 * 为了计算文件偏移和内存偏移统一
		 */
		u8* tmp = new u8[size];
		if( secHeader.sectionHeaderPtr[i].sh_type != SHT_NOBITS )
		{
			if( !body->GetData(offset,tmp,size) )
			{
				//读取失败
				for( unsigned int i = 0;i < data.size();++i )
					delete data[i];
				data.clear();
				elfHead = NULL;
				return false;
			}
		}
		data.push_back(tmp);	//按顺序压入
	}

	elfHead = &eh;
	return true;
}

void Section::PrintSectionName()
{
	if( elfHead == NULL )
		return ;
	//节名所在的符号表索引
	int nameSecIdx = elfHead->headerStruct.e_shstrndx;
	secHeader.PrintSectionName( (char*)data[nameSecIdx] );
}

bool Section::GetSymbol()
{
	if( elfHead == NULL )
		return false;
	//遍历所有的节，将所有type为SHT_SYMTAB的节都提取出来
	//SHT_DYNSYM,SHT_SUNW_LDYNSYM暂不做处理
	for( unsigned int i = 0;i < data.size();++i )
	{
		if( secHeader.sectionHeaderPtr[i].sh_type == SHT_SYMTAB )
		{
			//符号项数
			int cnt = 	secHeader.sectionHeaderPtr[i].sh_size /
						secHeader.sectionHeaderPtr[i].sh_entsize;
			Elf64_Sym *symMovePtr = (Elf64_Sym*)data[i];

			int strTabIdx = secHeader.sectionHeaderPtr[i].sh_link;
			char *pStrTab = (char*)data[strTabIdx];
			for( int j = 0;j < cnt;++j )
			{
				Symbol tmp(this);
				if( tmp.Init( symMovePtr,pStrTab ) )
					symArr.push_back(tmp);		//获得一个符号
				++symMovePtr;
			}
		}
	}
	return true;
}

void Section::PrintSymbols()
{
	for( unsigned int i = 0;i < symArr.size();++i )
		cout << i << " :" << symArr[i].name << endl;
}

string Section::GetFileName()
{
	return symArr[1].name;
}




