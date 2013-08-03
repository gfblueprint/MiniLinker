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
	ElfHeader *elfHead_;
	Elf64_Phdr *phdrPtr_;
	int num_;

	Paragraph();
	bool GetParagraphHeader( ElfHeader &eh );
};


#endif /* PARAGRAPHCLASS_H_ */
