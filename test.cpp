/*
 * test.cpp
 *
 *  Created on: Jul 27, 2013
 *      Author: ns
 */


#include <iostream>
#include <vector>
#include <string>
#include <elf.h>
#include "head/defines.h"
#include "head/objectFileClass.h"
#include "head/sectionClass.h"
#include "head/builderClass.h"

using namespace std;

string g_spFileName = "/home/ns/linkerLab/simple/a.o";
string g_spFileName2 = "/home/ns/linkerLab/simple/b.o";
string g_output = "/home/ns/linkerLab/simple/my_ab";
string g_lib = "/home/ns/linkerLab/MiniCRT";

int main()
{
	cout << "ehdr size:" << sizeof( Elf64_Ehdr) << endl;
	vector<string> objPath;

//	objPath.push_back( g_c );

//	objPath.push_back( g_spFileName );
	objPath.push_back( g_spFileName2 );
	Builder bd( objPath,g_lib,g_output );

	if( !bd.CollectSymbol() )
		return 0;
//	bd.PrintFindedSymbol();
//	cout << "-----------needed-----------" << endl;
//	bd.PrintNeededSymbol();
	bd.DoMerge();
//	bd.PrintOrder();
	bd.GetSymbolAddress();
	bd.Relocation();
	bd.GenerateBinary();

	return 0;
}
