/*
 * paragraphClass.cpp
 *
 *  Created on: Jul 27, 2013
 *      Author: ns
 */

#include "head/paragraphClass.h"
#include "head/defines.h"
#include "head/objectFileClass.h"

Paragraph::Paragraph()
{
	elfHead = NULL;
	phdrPtr = NULL;
	num = 0;
}

bool Paragraph::GetParagraphHeader( ElfHeader &eh )
{
	if( eh.belongTo == NULL )
		return false;

	ObjectFile *body = eh.belongTo;
	int offset = eh.headerStruct.e_phoff;
	num = eh.headerStruct.e_phnum;
	int size = eh.headerStruct.e_phentsize * num;

	phdrPtr = new Elf64_Phdr[num];
	body->GetData( offset,(u8*)phdrPtr,size );
	elfHead = &eh;

	return true;
}


