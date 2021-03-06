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
	outputPath_ = output;
	entryFunc_ = "mini_crt_entry";
	myEntryFuncObj_ =  lib + "/entry.o";
	objFilePath.push_back(myEntryFuncObj_);
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

		rawSection_.push_back(pSec);
	}
}

bool Builder::NewSymbol( Symbol sym )
{
	if( sym.name_ == "" )
		return true;
	unsigned char info = sym.symStr_->st_info;
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
		map<string,Symbol>::iterator it = finded_.find(sym.name_);
		if( it != finded_.end() )
		{
			;
		}
		else
			//TODO 考虑此处的复制是否会有问题
			finded_.insert(pair<string,Symbol>(sym.name_,sym));

		it = needed_.find(sym.name_);
		if( it != needed_.end() )
			needed_.erase(it);
	}
	else if( bind == STB_GLOBAL )
	{
		//可能是COMMON类型的..
		//判断
		map<string,Symbol>::iterator it = finded_.find(sym.name_);
		if( it != finded_.end() )
		{
			if( (it->second.symStr_->st_shndx != SHN_UNDEF && it->second.symStr_->st_shndx < 0xff00 ) &&
				(sym.symStr_->st_shndx != SHN_UNDEF && sym.symStr_->st_shndx < 0xff00 ))
			{
				cout << "error:" << sym.name_ << " more than once defined" << endl;
				return false;
			}
			//sym is strong and it->second is weak or common
			if( sym.symStr_->st_shndx != SHN_UNDEF && sym.symStr_->st_shndx != SHN_COMMON  )
				it->second = sym;	//覆盖
			//sym is common and it->second is common
			else if( sym.symStr_->st_shndx == SHN_COMMON && it->second.symStr_->st_shndx == SHN_COMMON )
			{
			}
			//sym is common or undef and it->second is strong .. ignore
			else
				;
		}
		else
		{
			if( sym.symStr_->st_shndx != SHN_UNDEF )
			{
				map<string,Symbol>::iterator ndIt = needed_.find(sym.name_);
				if( ndIt != needed_.end() )
					needed_.erase(ndIt);
				finded_.insert(pair<string,Symbol>(sym.name_,sym));
			}
			else
			{
				if( needed_.find(sym.name_) == needed_.end() )
					needed_.insert(pair<string,Symbol>(sym.name_,sym));
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
	for( unsigned int i = 0;i < rawSection_.size();++i )
	{
		for( unsigned int j = 0;j < rawSection_[i]->symArr_.size();++j )
		{
			if( !NewSymbol( rawSection_[i]->symArr_[j] ) )
				return false;
		}
	}

	map<string,Symbol>::iterator it = finded_.begin();
	while( it != finded_.end() )
	{
		if( it->second.symStr_->st_shndx == SHN_COMMON )
		{
			cout << "error:" << it->first << "没有初始化" << endl;
			return false;
		}
		++it;
	}

	if( needed_.empty() )
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

		for( unsigned int j = 0;j < pSec->symArr_.size();++j )
		{
			if( pSec->symArr_[j].name_ == "" )
				continue;

			if( pSec->symArr_[j].symStr_->st_shndx == SHN_UNDEF ||
				pSec->symArr_[j].symStr_->st_shndx == SHN_COMMON )
			{
				continue;
			}
			if( inLib_.find( pSec->symArr_[j].name_ ) != inLib_.end() )
			{
				cout << "多次发现:" << pSec->symArr_[j].name_ << " Ignore" << endl;
				continue;
			}
			inLib_.insert( pair<string,Symbol>(pSec->symArr_[j].name_,pSec->symArr_[j]) );
		}

		libSection_.push_back(pSec);
	}

//	PrintLibSymbol();

	LinkLibSymbol();

	if( needed_.size() != 0 )
	{
		cout << "error:无法找到下列符号定义" << endl;
		map<string,Symbol>::iterator it = needed_.begin();
		while( it != needed_.end() )
		{
			cout << it->first << endl;
			++it;
		}
#ifdef DEBUG
		cout << "已找到的符号" << endl;
		it = finded_.begin();
		while( it != finded_.end() )
		{
			cout << it->first << endl;
			++it;
		}
#endif
		return false;
	}

	return true;
}

bool Builder::LinkLibSymbol()
{
	map<string,Symbol> couldNotFind;
	while( needed_.size() )
	{
		map<string,Symbol>::iterator nowNeedIt = needed_.begin();
		map<string,Symbol>::iterator findIt = inLib_.find(nowNeedIt->first);

		if( findIt == inLib_.end() )		//库中也无法找到
		{
			couldNotFind.insert( pair<string,Symbol>(nowNeedIt->first,nowNeedIt->second) );
			needed_.erase(nowNeedIt);
			continue;
		}
		//找到了，要把对应section的所有lib都加载进来
		Section *pSec = findIt->second.belongToSec_;
		cout << "添加库：" << pSec->GetFileName() << endl;
		rawSection_.push_back(pSec);
		needed_.erase(nowNeedIt);
		for( unsigned int i = 0;i < pSec->symArr_.size();++i )
			NewSymbol( pSec->symArr_[i] );
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
	map<string,Symbol>::iterator it = finded_.begin();
	while( it != finded_.end() )
	{
		cout << " :" << it->first << "\t\t"
				<< it->second.belongToSec_->GetFileName() << endl;
		++it;
	}
}

void Builder::PrintNeededSymbol()
{
	map<string,Symbol>::iterator it = needed_.begin();
	while( it != needed_.end() )
	{
		cout << " :" << it->first << endl;
		++it;
	}
}

bool Builder::DoMerge()
{
	//先把要合并的节挑出来
	//所有节最后分三种，可读可执行，可写可读，只读
	for( unsigned int i = 0;i < rawSection_.size();++i )
	{
		Section *pSec = rawSection_[i];
		for( int j = 0;j < pSec->secHeader.num_;++j )
		{
			if( pSec->secHeader.sectionHeaderPtr_[j].sh_size == 0 )
				continue;
			if( pSec->secHeader.sectionHeaderPtr_[j].sh_type == SHT_PROGBITS ||
				pSec->secHeader.sectionHeaderPtr_[j].sh_type == SHT_NOBITS	)
			{
				Position tp( i,j );
				Elf64_Xword flags = pSec->secHeader.sectionHeaderPtr_[j].sh_flags;
				if( flags & SHF_EXECINSTR )
					exeSec_.push_back(tp);
				else if( flags & SHF_WRITE )
					writeSec_.push_back(tp);
				else if( flags & SHF_ALLOC )
					readSec_.push_back(tp);
				else
					;
			}
		}
	}
	int offset = GetHeadSize();
	int vAddr = ENTRY_ADDRESS + offset;

	for( unsigned int i = 0;i < exeSec_.size();++i )
	{
		order_.push_back( exeSec_[i] );
		Elf64_Shdr *pshdr = &(rawSection_[exeSec_[i].a_]->secHeader.sectionHeaderPtr_[exeSec_[i].b_]);
		CalculateSectionAddress( pshdr,offset,vAddr );
	}
	//程序段比较特殊的要求
	vAddr = vAddr + CountAlign( vAddr,PAGE_SIZE_ALGIN ) + offset;
	for( unsigned int i = 0;i < writeSec_.size();++i )
	{
		order_.push_back( writeSec_[i] );
		Elf64_Shdr *pshdr = &(rawSection_[writeSec_[i].a_]->secHeader.sectionHeaderPtr_[writeSec_[i].b_]);
		CalculateSectionAddress( pshdr,offset,vAddr );

	}
	vAddr = vAddr + CountAlign( vAddr,PAGE_SIZE_ALGIN ) + offset;
	for( unsigned int i = 0;i < readSec_.size();++i )
	{
		order_.push_back( readSec_[i] );
		Elf64_Shdr *pshdr = &(rawSection_[readSec_[i].a_]->secHeader.sectionHeaderPtr_[readSec_[i].b_]);
		CalculateSectionAddress( pshdr,offset,vAddr );
	}

	return true;
}

void Builder::CalculateSectionAddress( Elf64_Shdr *pshdr,int &offset,int &vAddr )
{
	int tn = CountAlign( vAddr,pshdr->sh_addralign );
	offset += tn;
	vAddr += tn;
	pshdr->sh_offset = offset;
	pshdr->sh_addr = vAddr;
	tn = pshdr->sh_size;
	offset += tn;
	vAddr += tn;
}

int Builder::GetHeadSize()
{
	/*
	 * 在把节都挑选出来以后，计算头部信息要多大
	 * 一个elf header
	 * 0个shdr
	 * 3个phdr
	 */

	return sizeof(Elf64_Ehdr) + 3*sizeof(Elf64_Phdr);
}

void Builder::GetSymbolAddress()
{
	/*
	 * 运行在DoMerge之后，保证order中的节的sh_addr被修正了
	 * 计算finded中symbol的实际地址。
	 * 计算方法：
	 * 节的虚拟地址加上符号在节中的偏移
	 *
	 */

	map<string,Symbol>::iterator it = finded_.begin();
	while( it != finded_.end() )
	{
		int secIdx = it->second.symStr_->st_shndx;
		if( it->second.symStr_->st_shndx == 0 || it->second.symStr_->st_shndx >= 0xff00 )
		{
			++it;
			continue;
		}
		int secAddr = it->second.belongToSec_->secHeader.sectionHeaderPtr_[secIdx].sh_addr;
		int symAddr = secAddr + it->second.symStr_->st_value;
		it->second.SetAddress(symAddr);
#ifdef DEBUG
		cout << "Addr:" << it->first << "\t\t" << hex << symAddr << dec << endl;
#endif
		++it;
	}
}

void Builder::PrintOrder()
{
	cout << "----------order----------" <<endl;
	for( unsigned i = 0;i < order_.size();++i )
	{
		cout << i << " : " << order_[i].a_ << "\t\t" << order_[i].b_ << endl;
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

	for( unsigned int secCnt = 0;secCnt < rawSection_.size();++secCnt )
	{
		Section *pSec = rawSection_[secCnt];
		SectionHeader ph = pSec->secHeader;

		for( int i = 0;i < ph.num_;++i )
		{
			if( ph.sectionHeaderPtr_[i].sh_type == SHT_RELA )
			{
//				if( ph.sectionHeaderPtr[i].sh_flags != SHF_INFO_LINK )
//				{
//					cout << "warning : 2" << endl;
//					continue ;
//				}
				Elf64_Rela *pRela = (Elf64_Rela*)pSec->data_[i];

				int cnt = pSec->secHeader.sectionHeaderPtr_[i].sh_size /
						  pSec->secHeader.sectionHeaderPtr_[i].sh_entsize;

				for( int j = 0;j < cnt;++j )
				{
					int relIdx = ph.sectionHeaderPtr_[i].sh_info;	//需要修正的地址
					int symIdx = ph.sectionHeaderPtr_[i].sh_link;

					int relAddr;

					Elf64_Sym *pSym = &(((Elf64_Sym*)pSec->data_[symIdx])[ELF64_R_SYM(pRela->r_info)]);
					char *pStrTab = (char*)pSec->data_[ ph.sectionHeaderPtr_[symIdx].sh_link ];
					string symName( pStrTab + pSym->st_name );

					if( symName == "" )
					{

						//静态变量，没有对应的sym。将relAddr设为对应节的虚拟地址即可。会通过append正确计算
						relAddr = pSec->secHeader.sectionHeaderPtr_[pSym->st_shndx].sh_addr;
					}
					else
					{
						map<string,Symbol>::iterator it = finded_.find(symName);
						if( it == finded_.end() )
							continue;

						relAddr = it->second.address_;
						if( relAddr == -1 )
							continue;
					}

					//计算p
					int pVal = ph.sectionHeaderPtr_[relIdx].sh_addr + pRela->r_offset;
					int *relPls = (int*)(pSec->data_[relIdx] + pRela->r_offset);

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
#ifdef DEBUG
						cout << "warning : 4" << endl;
#endif
					}

#ifdef DEBUG
				cout << "rela:" << symName << "\t" << symIdx << "\t" << hex << ELF64_R_SYM(pRela->r_info) << "\t" << relIdx << dec << endl;
				cout << "pVal:" << hex << pVal << dec << endl;
				cout << "symbol rel addr:" << hex << relAddr << dec << endl;
				cout << "change to:" << hex << *relPls << dec << endl;
#endif
					++pRela;
				}
			}
			else if( ph.sectionHeaderPtr_[i].sh_type == SHT_REL )
			{
				//没有处理这种节
				cout << "warning : find rel" << endl;
			}
			else
				;
		}

		#ifdef  DEBUG
					cout << "--------------" << endl;
		#endif
	}
}

void Builder::FillPhdr( Elf64_Phdr *pphdr,vector<Position> &block )
{
	int tn = block.size();
	if( tn == 0 )
	{
		pphdr->p_type = PT_NULL;
		return ;
	}
	Elf64_Shdr *pbeg = &(rawSection_[block[0].a_]->secHeader.sectionHeaderPtr_[block[0].b_]);
	Elf64_Shdr *pend = &(rawSection_[block[tn-1].a_]->secHeader.sectionHeaderPtr_[block[tn-1].b_]);
	pphdr->p_type = PT_LOAD;
	pphdr->p_offset = pbeg->sh_offset;
	pphdr->p_vaddr = pbeg->sh_addr;
	pphdr->p_paddr = pbeg->sh_addr;
	pphdr->p_filesz = pend->sh_offset - pbeg->sh_offset + pend->sh_size;
	pphdr->p_memsz = pphdr->p_filesz;
	pphdr->p_align = 0;
}

int Builder::CountAlign( int addr,int n )
{
	int tn = 1<<n;
	if( addr % tn == 0)
		return 0;
	return tn - (addr%tn);
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
	map<string,Symbol>::iterator entryIt = finded_.find( entryFunc_ );
	if( entryIt == finded_.end() )
	{
		cout << "error:" << "找不到入口函数" << endl;
		return false;
	}
	int etAddr = entryIt->second.address_;

	Elf64_Ehdr eh = rawSection_[0]->elfHead_->headerStruct_;
	eh.e_type = ET_EXEC;
	eh.e_entry = etAddr;
	eh.e_phoff = sizeof(Elf64_Ehdr);
	eh.e_phentsize = sizeof(Elf64_Phdr);
	eh.e_phnum = 3;
	eh.e_shentsize = sizeof(Elf64_Shdr);
	eh.e_shnum = 0;	//0
	eh.e_shstrndx = 0;				//notice 没有字符串表 设为0
	eh.e_shoff = 0;

	//代码段比较特殊，因为要加上程序头
	int tn = exeSec_.size()-1;
	Elf64_Shdr *pexeEndShdr = &(rawSection_[exeSec_[tn].a_]->secHeader.sectionHeaderPtr_[exeSec_[tn].b_]);
	Elf64_Phdr ph[3];
	FillPhdr( &ph[0],exeSec_ );
	ph[0].p_flags = PF_X | PF_R;
	ph[0].p_offset = 0;
	ph[0].p_filesz = pexeEndShdr->sh_offset + pexeEndShdr->sh_size;
	ph[0].p_memsz = ph[0].p_filesz;
	ph[0].p_vaddr = ENTRY_ADDRESS;
	ph[0].p_paddr = ph[0].p_vaddr;

	FillPhdr( &ph[1],writeSec_ );
	ph[1].p_flags = PF_W | PF_R;
	FillPhdr( &ph[2],readSec_ );
	ph[2].p_flags = PF_R;

	fstream newBin( outputPath_.c_str(),ios::binary | ios::out );
	newBin.write( (char*)&eh,sizeof( Elf64_Ehdr ) );
	newBin.write( (char*)ph,3*sizeof( Elf64_Phdr ) );

	int nowOffset = GetHeadSize();
	for( unsigned int i = 0;i < order_.size();++i )
	{
		Elf64_Shdr *pshdr = &(rawSection_[order_[i].a_]->secHeader.sectionHeaderPtr_[order_[i].b_]);
		int tn = pshdr->sh_offset - nowOffset;
		if( tn < 0 )
		{
			cout << "error:GenerateBinary()" <<endl;
		}
		if( tn > 0 )
		{
			char *pc = new char[tn];
#ifdef DEBUG
			for( int j = 0;j < tn;++j )
				pc[j] = 0xCC;
#endif
			newBin.write( pc,tn );			//填补由于对齐产生的空白，随便添些东西
		}
		char *pTmp = (char*)rawSection_[order_[i].a_]->data_[order_[i].b_];
		int size = pshdr->sh_size;
		newBin.write( pTmp,size );
		nowOffset = pshdr->sh_offset + size;
	}
	newBin.close();

	cout << "已经生成文件" << outputPath_ << endl;

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
	map<string,Symbol>::iterator it = inLib_.begin();
	while( it != inLib_.end() )
	{
		cout << "lib:" << it->first << endl;
		++it;
	}
}














