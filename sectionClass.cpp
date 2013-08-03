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
	belongTo_ = NULL;
}

bool ElfHeader::GetHeader( ObjectFile &obj )
{
	if( !obj.IsValid() )
		return false;

	if( obj.GetData(0,(u8*)&headerStruct_,sizeof(Elf64_Ehdr)) )
	{
		belongTo_ = &obj;
		return true;
	}
	else
		return false;
}

void ElfHeader::PrintInfomation()
{
	if( belongTo_ == NULL )
	{
		cout << "Dosen't belong to any one" << endl;
		return ;
	}

	cout << "Printing " << belongTo_->GetName() << "'s elf header infomation" << endl;

	cout << "Magic:" << hex;
	for( unsigned int i = 0;i < sizeof(headerStruct_.e_ident);++i )
		cout << headerStruct_.e_ident[i] << " ";
	cout << endl << dec;

	cout << "Type:" << headerStruct_.e_type << endl;
	cout << "Machine:" << headerStruct_.e_machine << endl;
	cout << "Version:" << headerStruct_.e_version << endl;
	cout << "Entry:" << headerStruct_.e_entry << endl;
	cout << "P offset:" << headerStruct_.e_phoff << endl;
	cout << "S offset:" << headerStruct_.e_shoff << endl;
	cout << "flags:" << headerStruct_.e_flags << endl;
	cout << "Elf header size:" << headerStruct_.e_ehsize << endl;
	cout << "Phent size:" << headerStruct_.e_phentsize << endl;
	cout << "Phent num:" << headerStruct_.e_phnum << endl;
	cout << "Section head size:" << headerStruct_.e_shentsize << endl;
	cout << "Section num:" << headerStruct_.e_shnum << endl;
	cout << "Section name sym index:" << headerStruct_.e_shstrndx << endl;
}

SectionHeader::SectionHeader()
{
	num_ = 0;
	sectionHeaderPtr_ = NULL;
	elfHead_ = NULL;
}

bool SectionHeader::GetSectionHeader( ElfHeader eh )
{
	if( eh.belongTo_ == NULL )
		return false;

	ObjectFile *body = eh.belongTo_;

	sectionHeaderPtr_ = new Elf64_Shdr[eh.headerStruct_.e_shnum];
	if( ! body->GetData(eh.headerStruct_.e_shoff,(u8*)sectionHeaderPtr_
			,eh.headerStruct_.e_shentsize*eh.headerStruct_.e_shnum) )
	{
		delete sectionHeaderPtr_;
		sectionHeaderPtr_ = NULL;
		return false;
	}

	num_ = eh.headerStruct_.e_shnum;
	elfHead_ = &eh;
	return true;
}

string SectionHeader::GetSectionName( char *nameSecData,int idx )
{
	if( sectionHeaderPtr_ == NULL )
		return "";
	int nameOffset = sectionHeaderPtr_[idx].sh_name;
	return string( nameSecData + nameOffset );
}

void SectionHeader::PrintSectionName( char *nameSecData )
{
	for( int i = 0;i < num_;++i )
		cout << GetSectionName( nameSecData,i ) << endl;
}

Symbol::Symbol( Section *p )
{
	belongToSec_ = p;
	symStr_ = NULL;
	address_ = -1;
}

bool Symbol::Init( Elf64_Sym *sym,char *pStrSec )
{
	if( belongToSec_ == NULL || sym == NULL || pStrSec == NULL )
		return false;
	symStr_ = sym;

	name_ = string( (char*)(pStrSec+sym->st_name) );

	return true;
}

void Symbol::SetAddress( int addr )
{
	address_ = addr;
}

Section::Section()
{
	elfHead_ = NULL;
}

bool Section::GetSection( ElfHeader &eh )
{
	if( !secHeader.GetSectionHeader(eh) )
		return false;

	//开始读入节的内容
	ObjectFile *body = eh.belongTo_;
	for( int i = 0;i < secHeader.num_;++i )
	{
		int size = secHeader.sectionHeaderPtr_[i].sh_size;
		int offset = secHeader.sectionHeaderPtr_[i].sh_offset;
		/*
		 * 注意：此处对于NOBITS类型，即bss类型也分配了空间
		 * 这些空间会被写入到最后的bin文件中
		 * 为了计算文件偏移和内存偏移统一
		 */
		u8* tmp = new u8[size];
		if( secHeader.sectionHeaderPtr_[i].sh_type != SHT_NOBITS )
		{
			if( !body->GetData(offset,tmp,size) )
			{
				//读取失败
				for( unsigned int i = 0;i < data_.size();++i )
					delete data_[i];
				data_.clear();
				elfHead_ = NULL;
				return false;
			}
		}
		data_.push_back(tmp);	//按顺序压入
	}

	elfHead_ = &eh;
	return true;
}

void Section::PrintSectionName()
{
	if( elfHead_ == NULL )
		return ;
	//节名所在的符号表索引
	int nameSecIdx = elfHead_->headerStruct_.e_shstrndx;
	secHeader.PrintSectionName( (char*)data_[nameSecIdx] );
}

bool Section::GetSymbol()
{
	if( elfHead_ == NULL )
		return false;
	//遍历所有的节，将所有type为SHT_SYMTAB的节都提取出来
	//SHT_DYNSYM,SHT_SUNW_LDYNSYM暂不做处理
	for( unsigned int i = 0;i < data_.size();++i )
	{
		if( secHeader.sectionHeaderPtr_[i].sh_type == SHT_SYMTAB )
		{
			//符号项数
			int cnt = 	secHeader.sectionHeaderPtr_[i].sh_size /
						secHeader.sectionHeaderPtr_[i].sh_entsize;
			Elf64_Sym *symMovePtr = (Elf64_Sym*)data_[i];

			int strTabIdx = secHeader.sectionHeaderPtr_[i].sh_link;
			char *pStrTab = (char*)data_[strTabIdx];
			for( int j = 0;j < cnt;++j )
			{
				Symbol tmp(this);
				if( tmp.Init( symMovePtr,pStrTab ) )
					symArr_.push_back(tmp);		//获得一个符号
				++symMovePtr;
			}
		}
	}
	return true;
}

void Section::PrintSymbols()
{
	for( unsigned int i = 0;i < symArr_.size();++i )
		cout << i << " :" << symArr_[i].name_ << endl;
}

string Section::GetFileName()
{
	return symArr_[1].name_;
}




