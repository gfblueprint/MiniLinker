/*
 * main.cpp
 *
 *  Created on: Aug 4, 2013
 *      Author: ns
 */

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include "head/defines.h"
#include "head/builderClass.h"

int main( int argc,char *argv[] )
{
	const char *optStr = ":o:l:";
	vector<string> objPath;
	string libPath;
	string outputPath;

	int opt = 0;
	while((opt=getopt(argc,argv,optStr)) != -1 )
	{
		switch( opt )
		{
		case 'o':
			if(  outputPath.length() )
			{
				cout << "错误：定义了多个输出路径" << endl;
				return 0;
			}
			outputPath = optarg;
			break;
		case 'l':
			if( libPath.size() )
			{
				cout << "错误：定义了多个库路径" << endl;
				return 0;
			}
			libPath = optarg;
			break;
		case ':':
			cout << "缺少参数" << endl;
			break;
		case '?':
			cout << "忽略错误选项：" << (char)optopt << endl;
			break;
		}
	}
	if( !libPath.size() )
	{
		cout << "错误：缺少库" << endl;
		return 0;
	}
	if( !outputPath.size() )
	{
		cout << "错误：没有定义输出路径" << endl;
		return 0;
	}

	for( ;optind < argc;++optind )
		objPath.push_back(argv[optind]);
	if( objPath.size() == 0 )
	{
		cout << "错误：缺少输入" << endl;
		return 0;
	}

	Builder bd( objPath,libPath,outputPath);

	if( !bd.CollectSymbol() )
	{
		cout << "失败退出" << endl;
		return 0;
	}
	bd.DoMerge();
	bd.GetSymbolAddress();
	bd.Relocation();
	if( !bd.GenerateBinary() )
	{
		cout << "失败退出" << endl;
		return 0;
	}
	return 0;
}

//#include <iostream>
//#include <vector>
//#include <string>
//#include <elf.h>
//#include "head/defines.h"
//#include "head/objectFileClass.h"
//#include "head/sectionClass.h"
//#include "head/builderClass.h"
//
//using namespace std;
//
//string g_spFileName = "/home/ns/linkerLab/simple/a.o";
//string g_spFileName2 = "/home/ns/linkerLab/simple/b.o";
//string g_output = "/home/ns/linkerLab/simple/my_ab";
////string g_lib = "/home/ns/linkerLab/minicrt";
//string g_lib = "/home/ns/linkerLab/libc-o";
//
//int main()
//{
//	vector<string> objPath;
//
//	objPath.push_back( g_spFileName );
////	objPath.push_back( g_spFileName2 );
//	Builder bd( objPath,g_lib,g_output );
//
//	if( !bd.CollectSymbol() )
//		return 0;
//	bd.DoMerge();
//	bd.GetSymbolAddress();
//	bd.Relocation();
//	bd.GenerateBinary();
//
//	return 0;
//}
