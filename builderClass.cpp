/*
 * builderClass.cpp
 *
 *  Created on: Jul 28, 2013
 *      Author: ns
 */

#include <stdlib.h>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include "head/builderClass.h"
#include "head/defines.h"
#include "head/objectFileClass.h"

using namespace std;

Position::Position( int a,int b)
{
	a_ = a;
	b_ = b;
}

Builder::Builder( vector<string> &objFilePath,string lib,string output )
{
	libDirPath_ = lib;
	outputPath = output;
	entryFunc = "mini_crt_entry";
	myEntryFuncObj = "/home/ns/linkerLab/MiniCRT/entry.o";
//	myEntryFuncObj = "/home/ns/linkerLab/libc-o/crt1.o";
	objFilePath.push_back(myEntryFuncObj);
	for( unsigned int i = 0;i < objFilePath.size();++i )
	{
		//这些变量在后面都会以指针形式被保存下来，所以不能用局部变亮
		ObjectFile *pObj = new ObjectFile( objFilePath[i] );
		if( !pObj->IsValid() )
		{
			cout << objFilePath[i] << " is invalid. * Ignore *" << endl;
			continue;
		}
		ElfHeader *pEh = new ElfHeader;
		pEh->GetHeader( *pObj );
		Section *pSec = new Section();
		if( !pSec->GetSection(*pEh) )
		{
			cout << "error while getting section:" << objFilePath[i] << endl;
			exit(0);
		}
		if( !pSec->GetSymbol() )
		{
			cout << "error while getting symbols:" << objFilePath[i] << endl;
			exit(0);
		}

		rawSection.push_back(pSec);
	}
}

bool Builder::NewSymbol( Symbol sym )
{
	if( sym.name == "" )
		return true;
	unsigned char info = sym.symStr->st_info;
	unsigned char type = ELF64_ST_TYPE(info);
	unsigned char bind = ELF64_ST_BIND(info);

	if( ! (type == STT_FUNC || type == STT_COMMON ||
		type == STT_NOTYPE || type == STT_OBJECT ) )
		return true;

	//TODO COMMON块处理
	if( bind == STB_WEAK || bind == STB_LOCAL )
	{
		//此处假设weak和local不会是UNDEF  空串除去
		//加入到finded中
		map<string,Symbol>::iterator it = finded.find(sym.name);
		if( it != finded.end() )
		{
			;
//			if( sym.symStr->st_shndx == SHN_COMMON &&
//				it->second.symStr->st_value < sym.symStr->st_value )
//			{
//				//common块，取最大的
//				it->second.symStr->st_value = sym.symStr->st_value;
//			}
		}
		else
			//TODO 考虑此处的复制是否会有问题
			finded.insert(pair<string,Symbol>(sym.name,sym));

		it = needed.find(sym.name);
		if( it != needed.end() )
			needed.erase(it);
	}
	else if( bind == STB_GLOBAL )
	{
		//可能是COMMON类型的..
		//判断
		map<string,Symbol>::iterator it = finded.find(sym.name);
		if( it != finded.end() )
		{
			if( (it->second.symStr->st_shndx != SHN_UNDEF && it->second.symStr->st_shndx < 0xff00 ) &&
				(sym.symStr->st_shndx != SHN_UNDEF && sym.symStr->st_shndx < 0xff00 ))
			{
				cout << "error:" << sym.name << " more than once defined" << endl;
				return false;
			}
			//sym is strong and it->second is weak or common
			if( sym.symStr->st_shndx != SHN_UNDEF && sym.symStr->st_shndx != SHN_COMMON  )
				it->second = sym;	//覆盖
			//sym is common and it->second is common
			else if( sym.symStr->st_shndx == SHN_COMMON && it->second.symStr->st_shndx == SHN_COMMON )
			{
//				if( sym.symStr->st_value > it->second.symStr->st_value )
//					it->second.symStr->st_value = sym.symStr->st_value;
			}
			//sym is common or undef and it->second is strong .. ignore
			else
				;
		}
		else
		{
			if( sym.symStr->st_shndx != SHN_UNDEF )
			{
				map<string,Symbol>::iterator ndIt = needed.find(sym.name);
				if( ndIt != needed.end() )
					needed.erase(ndIt);
				finded.insert(pair<string,Symbol>(sym.name,sym));
			}
			else
			{
				if( needed.find(sym.name) == needed.end() )
					needed.insert(pair<string,Symbol>(sym.name,sym));
			}
		}
	}
	else
		;
	return true;
}

bool Builder::CollectSymbol()
{
	//如果能够集齐返回真

	//先处理rawSection，不够的再到标准库中找
	//TODO section的释放问题

	for( unsigned int i = 0;i < rawSection.size();++i )
	{
		for( unsigned int j = 0;j < rawSection[i]->symArr.size();++j )
		{
			if( !NewSymbol( rawSection[i]->symArr[j] ) )
				return false;
		}
	}

	//TODO 假设COMMON块只会由变量声明产生
	map<string,Symbol>::iterator it = finded.begin();
	while( it != finded.end() )
	{
		if( it->second.symStr->st_shndx == SHN_COMMON )
		{
			cout << "Error :a common didn't fit" << endl;
			return false;
		}
		++it;
	}

	if( needed.empty() )
		return true;

	cout << "正在链接标准库" <<endl;

	/*
	 * 库中所有的o文件全部载入
	 */

	vector<string> libObjPath;
	GetLibObjPath( libDirPath_,libObjPath );

	for( unsigned int i = 0;i < libObjPath.size();++i )
	{
//		cout << "正在添加库:" << libObjPath[i] << endl;
		//方法同Builder构造,只是同时把inLib构建好了
		ObjectFile *pObj = new ObjectFile( libObjPath[i] );
		if( !pObj->IsValid() )
		{
			cout << libObjPath[i] << " is invalid. * Ignore *" << endl;
			continue;
		}
		ElfHeader *pEh = new ElfHeader;
		pEh->GetHeader( *pObj );
		Section *pSec = new Section();
		if( !pSec->GetSection(*pEh) )
		{
			cout << "error while getting section:" << libObjPath[i] << endl;
			exit(0);
		}
		if( !pSec->GetSymbol() )
		{
			cout << "error while getting symbols:" << libObjPath[i] << endl;
			exit(0);
		}

		for( unsigned int j = 0;j < pSec->symArr.size();++j )
		{
			if( pSec->symArr[j].name == "" )
				continue;

			if( pSec->symArr[j].symStr->st_shndx == SHN_UNDEF ||
				pSec->symArr[j].symStr->st_shndx == SHN_COMMON )
			{
				continue;
			}
			if( inLib.find( pSec->symArr[j].name ) != inLib.end() )
			{
				cout << "多次发现:" << pSec->symArr[j].name << " Ignore" << endl;
				continue;
			}
			inLib.insert( pair<string,Symbol>(pSec->symArr[j].name,pSec->symArr[j]) );
		}

		libSection.push_back(pSec);
	}

//	PrintLibSymbol();

	LinkLibSymbol();

	if( needed.size() != 0 )
	{
		cout << "error:无法找到下列符号定义" << endl;
		map<string,Symbol>::iterator it = needed.begin();
		while( it != needed.end() )
		{
			cout << it->first << endl;
			++it;
		}
		cout << "已找到的符号" << endl;
		it = finded.begin();
		while( it != finded.end() )
		{
			cout << it->first << endl;
			++it;
		}
		return false;
	}

	return true;
}

bool Builder::LinkLibSymbol()
{
	map<string,Symbol> couldNotFind;
	while( needed.size() )
	{
		map<string,Symbol>::iterator nowNeedIt = needed.begin();
		map<string,Symbol>::iterator findIt = inLib.find(nowNeedIt->first);

		if( findIt == inLib.end() )		//库中也无法找到
		{
			couldNotFind.insert( pair<string,Symbol>(nowNeedIt->first,nowNeedIt->second) );
			needed.erase(nowNeedIt);
			continue;
		}
		//找到了，要把对应section的所有lib都加载进来
		Section *pSec = findIt->second.belongToSec;
		cout << "添加库：" << pSec->GetFileName() << endl;
		rawSection.push_back(pSec);
		needed.erase(nowNeedIt);
		for( unsigned int i = 0;i < pSec->symArr.size();++i )
			NewSymbol( pSec->symArr[i] );
	}

	if( couldNotFind.size() )
	{
		map<string,Symbol>::iterator it = couldNotFind.begin();
		while( it != couldNotFind.end() )
		{
			cout << "Missing:" << it->first << endl;
			++it;
		}
		return false;
	}
	return true;
}

void Builder::PrintFindedSymbol()
{
	map<string,Symbol>::iterator it = finded.begin();
	while( it != finded.end() )
	{
		cout << " :" << it->first << "\t\t"
				<< it->second.belongToSec->GetFileName() << endl;
		++it;
	}
}

void Builder::PrintNeededSymbol()
{
	map<string,Symbol>::iterator it = needed.begin();
	while( it != needed.end() )
	{
		cout << " :" << it->first << endl;
		++it;
	}
}

bool Builder::DoMerge()
{
	//先把要合并的节挑出来
	for( unsigned int i = 0;i < rawSection.size();++i )
	{
		Section *pSec = rawSection[i];
		for( int j = 0;j < pSec->secHeader.num;++j )
		{
			/*
			 * 包含这样属性的节一股脑加上
			 */
			if( pSec->secHeader.sectionHeaderPtr[j].sh_size == 0 )
				continue;
			if( pSec->secHeader.sectionHeaderPtr[j].sh_type == SHT_PROGBITS ||
				pSec->secHeader.sectionHeaderPtr[j].sh_type == SHT_NOBITS	)
			{
				Position tp( i,j );
				order.push_back(tp);
			}
		}
	}

	//TODO 添加对齐
	/*
	 * 修正原有的shdr中的sh_addr和sh_offset即可
	 */

	int offset = GetHeadSize();
	int vAddr = ENTRY_ADDRESS + offset;

	for( unsigned int i = 0;i < order.size();++i )
	{
		Section *pSec = rawSection[order[i].a_];
		pSec->secHeader.sectionHeaderPtr[order[i].b_].sh_offset = offset;
		pSec->secHeader.sectionHeaderPtr[order[i].b_].sh_addr = vAddr;
		int size = pSec->secHeader.sectionHeaderPtr[order[i].b_].sh_size;
		offset += size;
		vAddr += size;
	}

	return true;
}

int Builder::GetHeadSize()
{
	/*
	 * 在把节都挑选出来以后，计算头部信息要多大
	 * 一个elf header
	 * n个shdr
	 * 1个phdr
	 */

	return sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
}

void Builder::GetSymbolAddress()
{
	/*
	 * 运行在DoMerge之后，保证order中的节的sh_addr被修正了
	 * 计算finded中symbol的实际地址。
	 * 计算方法：
	 * 节的虚拟地址加上符号在节中的偏移
	 *
	 * TODO 假设所有finded里的symbol都在order的节中
	 */

	map<string,Symbol>::iterator it = finded.begin();
	while( it != finded.end() )
	{
		int secIdx = it->second.symStr->st_shndx;
		if( it->second.symStr->st_shndx == 0 || it->second.symStr->st_shndx >= 0xff00 )
		{
			++it;
			continue;
		}
		int secAddr = it->second.belongToSec->secHeader.sectionHeaderPtr[secIdx].sh_addr;
		int symAddr = secAddr + it->second.symStr->st_value;
		it->second.SetAddress(symAddr);

		cout << "Addr:" << it->first << "\t\t" << hex << symAddr << dec << endl;

		++it;
	}
}

void Builder::PrintOrder()
{
	cout << "----------order----------" <<endl;
	for( unsigned i = 0;i < order.size();++i )
	{
		cout << i << " : " << order[i].a_ << "\t\t" << order[i].b_ << endl;
	}
	cout << "-----------end-----------" << endl;
}

void Builder::PrintLib()
{
	DIR *libDir = opendir( libDirPath_.c_str() );
	if( libDir == NULL )
	{
		cout << "不能打开" << libDirPath_ << endl;
		return ;
	}

	dirent *now;
	while( (now = readdir(libDir) ) != NULL )
	{
		cout << now->d_name << endl;
	}
}

void Builder::Relocation()
{
	/*
	 * 重定位
	 * 扫描所有节，如果节属性是rela类型，则进行处理
	 * TODO 暂时忽略rel类型
	 */

	for( unsigned int secCnt = 0;secCnt < rawSection.size();++secCnt )
	{
		Section *pSec = rawSection[secCnt];
		SectionHeader ph = pSec->secHeader;

		for( int i = 0;i < ph.num;++i )
		{
			if( ph.sectionHeaderPtr[i].sh_type == SHT_RELA )
			{
//				if( ph.sectionHeaderPtr[i].sh_flags != SHF_INFO_LINK )
//				{
//					cout << "warning : 2" << endl;
//					continue ;
//				}
				Elf64_Rela *pRela = (Elf64_Rela*)pSec->data[i];

				int cnt = pSec->secHeader.sectionHeaderPtr[i].sh_size /
						  pSec->secHeader.sectionHeaderPtr[i].sh_entsize;

				for( int j = 0;j < cnt;++j )
				{
					int relIdx = ph.sectionHeaderPtr[i].sh_info;	//需要修正的地址
					int symIdx = ph.sectionHeaderPtr[i].sh_link;

					Elf64_Sym *pSym = &(((Elf64_Sym*)pSec->data[symIdx])[ELF64_R_SYM(pRela->r_info)]);
					char *pStrTab = (char*)pSec->data[ ph.sectionHeaderPtr[symIdx].sh_link ];
					string symName( pStrTab + pSym->st_name );

					if( symName == "" )
						continue;

					map<string,Symbol>::iterator it = finded.find(symName);
					if( it == finded.end() )
						continue;

					int relAddr = it->second.address;
					if( relAddr == -1 )
						continue;

					//计算p
					int pVal = ph.sectionHeaderPtr[relIdx].sh_addr + pRela->r_offset;
					int *relPls = (int*)(pSec->data[relIdx] + pRela->r_offset);

					if( ELF64_R_TYPE(pRela->r_info)	== R_X86_64_PC32 )
					{
						//S+A-P
						*relPls = relAddr + pRela->r_addend - pVal;
					}
					else if( ELF64_R_TYPE(pRela->r_info) == R_X86_64_32 )
					{
						//S+A
						*relPls = relAddr + pRela->r_addend;
					}
					else
					{
						cout << "warning : 4" << endl;
					}

#ifdef SYM_DEBUG
				cout << "rela:" << symName << "\t" << symIdx << "\t" << hex << ELF64_R_SYM(pRela->r_info) << "\t" << relIdx << dec << endl;
				cout << "pVal:" << hex << pVal << dec << endl;
				cout << "symbol rel addr:" << hex << relAddr << dec << endl;
				cout << "change to:" << hex << *relPls << dec << endl;
#endif
					++pRela;
				}
			}
			else if( ph.sectionHeaderPtr[i].sh_type == SHT_REL )
			{
				cout << "warning : find rel" << endl;
			}
			else
				;
		}

		#ifdef SYM_DEBUG
					cout << "--------------" << endl;
		#endif
	}
}

bool Builder::GenerateBinary()
{
	/*
	 * 生成二进制文件
	 * elf header
	 * 一个phdr
	 * rawSection里的section header
	 * rawSection里的节合并输出
	 */
	map<string,Symbol>::iterator entryIt = finded.find( entryFunc );
	if( entryIt == finded.end() )
	{
		cout << "error:" << "找不到入口函数" << endl;
		return false;
	}
	int etAddr = entryIt->second.address;

	Elf64_Ehdr eh = rawSection[0]->elfHead->headerStruct;
	eh.e_type = ET_EXEC;
	eh.e_entry = etAddr;
	eh.e_phoff = sizeof(Elf64_Ehdr);
	eh.e_phentsize = sizeof(Elf64_Phdr);
	eh.e_phnum = 1;
	eh.e_shentsize = sizeof(Elf64_Shdr);
	eh.e_shnum = 0;	//0
	eh.e_shstrndx = 0;				//notice 没有字符串表 设为0
	eh.e_shoff = 0;

	int pSize = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
	for( unsigned int i = 0;i < order.size();++i )
	{
		pSize += rawSection[order[i].a_]->secHeader.sectionHeaderPtr[order[i].b_].sh_size;
	}

	Elf64_Phdr ph;
	ph.p_type = PT_LOAD;
	ph.p_offset = 0;
	ph.p_vaddr = ENTRY_ADDRESS;
	ph.p_paddr = ENTRY_ADDRESS;		//null
	ph.p_filesz = pSize;			//TODO 此段计算也需要修正
	ph.p_memsz = pSize;				//TODO bss段在计算offset的时候处理
	ph.p_flags = PF_X | PF_W | PF_R;	//全属性
	ph.p_align = 0;					//无指定

	fstream newBin( outputPath.c_str(),ios::binary | ios::out );
	newBin.write( (char*)&eh,sizeof( Elf64_Ehdr ) );
	newBin.write( (char*)&ph,sizeof( Elf64_Phdr ) );

	for( unsigned int i = 0;i < order.size();++i )
	{
		char *pTmp = (char*)rawSection[order[i].a_]->data[order[i].b_];
		int size = rawSection[order[i].a_]->secHeader.sectionHeaderPtr[order[i].b_].sh_size;
		newBin.write( pTmp,size );
	}
	newBin.close();

	cout << "已经生成文件" << outputPath << endl;

	return true;
}

void Builder::GetLibObjPath( string libPath,vector<string> &re )
{
	//忽略子目录
	DIR *libDir = opendir( libPath.c_str() );
	struct stat statBuf;
	if( libDir == NULL )
	{
		cout << "不能打开" << libPath << endl;
		return ;
	}

	dirent *now;
	while( (now = readdir(libDir) ) != NULL )
	{
		string path = libPath + '/' + now->d_name;
		stat( path.c_str(),&statBuf );
		if( S_ISDIR( statBuf.st_mode ) )
			continue;
		re.push_back(path);
	}
}

void Builder::PrintLibSymbol()
{
	map<string,Symbol>::iterator it = inLib.begin();
	while( it != inLib.end() )
	{
		cout << "lib:" << it->first << endl;
		++it;
	}
}














