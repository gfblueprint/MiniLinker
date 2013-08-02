/*
 * builderClass.h
 *
 *  Created on: Jul 28, 2013
 *      Author: ns
 */

#ifndef BUILDERCLASS_H_
#define BUILDERCLASS_H_

#include <vector>
#include <string>
#include <map>
#include "defines.h"
#include "sectionClass.h"

using namespace std;

struct Position
{
	int a_;
	int b_;

	Position( int a,int b );
};

class Builder
{
	//负责控制链接的类
	/*
	 * 目前设计就一个段，权限是rwx，数据于代码在一起
	 */
public:
	//标准库所在的目录，当缺少符号定义的时候会在标准库中寻找
	string libDirPath_;
	string entryFunc_;
	string outputPath_;
	//用户代码需要我的func来调用和退出
	string myEntryFuncObj_;
	/*
	 * 需要被处理的section集合
	 * 每一个元素都是一个.o文件中的所有section集合
	 */
	vector<Section*> rawSection_;
	//虚拟合并所得顺序
	vector<Position> order_;

	//目前还没有找到的符号集
	map<string,Symbol> needed_;
	//目前已经找到的符号集
	map<string,Symbol> finded_;
	//库中的符号集合
	map<string,Symbol> inLib_;
	vector<Section*> libSection_;

	Builder( vector<string> &objFilePath,string lib,string output );
	//找到完整的符号集，会搜索库
	bool CollectSymbol();
	bool NewSymbol( Symbol sym );
	void PrintFindedSymbol();
	void PrintNeededSymbol();
	void PrintOrder();
	void PrintLib();

	/*
	 *  虚拟合并，只拼接顺序，并计算虚拟地址和文件偏移
	 *  只生成一个段表，指令数据一起
	 *  符号表先不计入
	 */
	bool DoMerge();
	int GetHeadSize();
	void GetSymbolAddress();
	void Relocation();
	bool GenerateBinary();
	void GetLibObjPath( string libPath,vector<string> &re );
	//缺失的符号在库中查找
	bool LinkLibSymbol();
	void PrintLibSymbol();
};

#endif /* BUILDERCLASS_H_ */

