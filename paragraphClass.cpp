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
	elfHead_ = NULL;
	phdrPtr_ = NULL;
	num_ = 0;
}

bool Paragraph::GetParagraphHeader( ElfHeader &eh )
{
	if( eh.belongTo_ == NULL )
		return false;

	ObjectFile *body = eh.belongTo_;
	int offset = eh.headerStruct_.e_phoff;
	num_ = eh.headerStruct_.e_phnum;
	int size = eh.headerStruct_.e_phentsize * num_;

	phdrPtr_ = new Elf64_Phdr[num_];
	body->GetData( offset,(u8*)phdrPtr_,size );
	elfHead_ = &eh;

	return true;
}


