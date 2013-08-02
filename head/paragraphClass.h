/*
 * paragraphClass.h
 *
 *  Created on: Jul 27, 2013
 *      Author: ns
 */

#ifndef PARAGRAPHCLASS_H_
#define PARAGRAPHCLASS_H_

#include "defines.h"
#include "objectFileClass.h"

class Paragraph
{
	//段表
public:
	ElfHeader *elfHead;
	Elf64_Phdr *phdrPtr;
	int num;

	Paragraph();
	bool GetParagraphHeader( ElfHeader &eh );
};


#endif /* PARAGRAPHCLASS_H_ */
