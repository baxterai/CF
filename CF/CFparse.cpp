/*******************************************************************************
 * 
 * No License
 * 
 * This work is under exclusive copyright (c) Baxter AI (baxterai.com). 
 * Nobody else can use, copy, distribute, or modify this work without being 
 * at risk of take-downs, shake-downs, or litigation. 
 * 
 * By publishing this source code in a public repository on GitHub, Terms of 
 * Service have been accepted by which Baxter AI have allowed others to view 
 * and fork their repository.
 * 
 * If you find software that doesn't have a license, that generally means you 
 * have no permission from the creators of the software to use, modify, or 
 * share the software. Although a code host such as GitHub may allow you to 
 * view and fork the code, this does not imply that you are permitted to use, 
 * modify, or share the software for any purpose.
 *
 * This notice has been derived from https://choosealicense.com/no-permission 
 * (https://web.archive.org/web/20180312144938/https://choosealicense.com/no-permission)
 * 
 *******************************************************************************/

/*******************************************************************************
 *
 * File Name: CFparse.cpp
 * Author: Richard Bruce Baxter - Copyright (c) 2005-2018 Baxter AI (baxterai.com)
 * Project: Code Folder
 * Project Version: 1c2a 15-March-2018
 * /
 *******************************************************************************/


#include "CFparse.h"

bool parseBlocksFromFile(CFblock* firstBlockInFile, string parseFileName, int level)
{
	#ifdef CF_DEBUG_PARSE
	for(int i=0; i<level; i++)
	{
		cout << "\t";
	}
	cout << "\033[1;31m parseBlocksFromFile: " << parseFileName << "\033[0m" << endl;
	#endif

	bool result = false;
	
	ifstream parseFileObject(parseFileName.c_str());
	
	if(!parseFileObject.rdbuf()->is_open())
	{
		if(level == CF_INCLUDE_LEVEL_FIRST)
		{
			cout << "CF error - file not found, " << parseFileName << endl;
		}
	}
	else
	{
		result = true;
		int lineCount = 0;	//used for debugging
		bool returnUponHashTagExitStatement = false;
		parseTextBlock(&parseFileObject, firstBlockInFile, level, &lineCount, &returnUponHashTagExitStatement);
		parseFileObject.close();
		
		if(returnUponHashTagExitStatement)
		{
			cout << "parseBlocksFromFile() error: returnUponHashTagExitStatement" << endl;
		}
	}

	#ifdef CF_DEBUG_PARSE
	for(int i=0; i<level; i++)
	{
		cout << "\t";
	}
	cout << "\033[1;31m exiting parseBlocksFromFile: " << parseFileName << "\033[0m" << endl;
	#endif
		
	return result;
}

CFblock* parseTextBlock(ifstream* parseFileObject, CFblock* firstBlockInLayer, int level, int* lineCount, bool* returnUponHashTagExitStatement)
{
	#ifdef CF_DEBUG_PARSE
	for(int i=0; i<level; i++)
	{
		cout << "\t";
	}
	cout << "\033[1;34m parseTextBlock \033[0m" << endl;
	#endif
	bool parsingTextBlock = true;
	
	CFblock* currentBlockInLayer = firstBlockInLayer;
	
	char c;
	int charCount = 0;
	bool readingHashTagPreceedingWhiteSpace = true;
	bool readingHashTag = false;
	bool readingHashTagVariableName = false;
	string hashTag = "";
	string hashTagVariableName = "";
	string currentTextBlock = "";		//text for text block
	string currentLine = "";		//text for current line
	while(parsingTextBlock)
	{
		if(parseFileObject->get(c))
		{		
			charCount++;
			currentLine = currentLine + c;
			#ifdef CF_DEBUG_PARSE
			cout << c;
			#endif
			if(c == CHAR_FORWARDSLASH)
			{
				char tempChar;
				if(parseFileObject->get(tempChar))
				{
					#ifdef CF_DEBUG_PARSE
					cout << tempChar;
					#endif
					currentLine = currentLine + tempChar;	//change from c to tempChar 22 July 2013

					if(tempChar == CHAR_STAR)
					{
						readingHashTagPreceedingWhiteSpace = true;
						readingHashTag = false;
						readingHashTagVariableName = false;
						hashTag = "";			//in case a comment was added half way through a hash tag
						hashTagVariableName = "";	//in case a comment was added half way through a hash tag descriptor
										
						//large comment found
						currentTextBlock = currentTextBlock + removeLastCharactersFromString(currentLine, 2);
						currentLine = "";

						currentBlockInLayer->text = currentTextBlock;
						currentTextBlock = "";
						currentBlockInLayer->next = new CFblock();
						currentBlockInLayer = currentBlockInLayer->next;
						currentBlockInLayer = readLargeComment(parseFileObject, currentBlockInLayer, level, lineCount, &parsingTextBlock);
						if(parsingTextBlock)
						{
							waitForNewLine(parseFileObject, level, lineCount, &currentLine, &currentTextBlock, true,  &parsingTextBlock);	//CF does not support #tags proceeding comments on the same line					
						}
					}
					else if(tempChar == CHAR_FORWARDSLASH)	//changed from c to tempChar 22 July 2013
					{
						readingHashTagPreceedingWhiteSpace = true;
						readingHashTag = false;
						readingHashTagVariableName = false;
						hashTag = "";			//in case a comment was added half way through a hash tag
						hashTagVariableName = "";	//in case a comment was added half way through a hash tag descriptor
										
						//single line comment found
						currentTextBlock = currentTextBlock + removeLastCharactersFromString(currentLine, 2);
						currentLine = "";

						currentBlockInLayer->text = currentTextBlock;
						currentTextBlock = "";
						currentBlockInLayer->next = new CFblock();
						currentBlockInLayer = currentBlockInLayer->next;
						currentBlockInLayer = readSmallComment(parseFileObject, currentBlockInLayer, level, lineCount, &parsingTextBlock);
					}
					else if(tempChar == CHAR_NEWLINE)	//changed from c to tempChar 22 July 2013
					{
						readingHashTagPreceedingWhiteSpace = true;
						readingHashTag = false;
						readingHashTagVariableName = false;
						hashTag = "";			//in case a comment was added half way through a hash tag
						hashTagVariableName = "";	//in case a comment was added half way through a hash tag descriptor
										
						(*lineCount)++;
						currentTextBlock = currentTextBlock + currentLine;
						currentLine = "";
					}
					else
					{
						if(readingHashTagVariableName)
						{
							hashTagVariableName = hashTagVariableName + c;
							hashTagVariableName = hashTagVariableName + tempChar;
						}
						else
						{//NB: CF does not support #tags containing '\' or '/' characters (eg readingHashTag)  
							readingHashTagPreceedingWhiteSpace = true;
							readingHashTag = false;
							readingHashTagVariableName = false;
							hashTag = "";			//in case a comment was added half way through a hash tag
							hashTagVariableName = "";	//in case a comment was added half way through a hash tag descriptor
													
							waitForNewLine(parseFileObject, level, lineCount, &currentLine, &currentTextBlock, true, &parsingTextBlock);									
						}
					}	
				}
				else
				{
					parsingTextBlock = false;
				}
			}
			else if(readingHashTagPreceedingWhiteSpace)
			{
				if(SHAREDvarsClass().isWhiteSpace(c) || (c == CHAR_TAB))
				{

				}
				else if(c == CHAR_NEWLINE)
				{
					(*lineCount)++;
					currentTextBlock = currentTextBlock + currentLine;
					currentLine = "";					
				}
				else
				{
					readingHashTagPreceedingWhiteSpace = false;
					readingHashTag = true;

					hashTag = hashTag + c;	//add this character to first character
				}
			}
			else if(readingHashTag)
			{
				//cout << "readingHashTag: " << c << endl;			
				if(SHAREDvarsClass().isWhiteSpace(c))
				{
					//cout << "\nhashTag = " << hashTag << endl;
					if(hashTag == CF_HASH_TAG_SPECIAL_CASE_ELIF)
					{
						//ignore space within #elif defined / #elif !defined cases
						hashTag = hashTag + c;
					}
					else if(getSupportedHashTagID(hashTag, hashTagArrayWithVariables, CF_HASH_TAG_WITH_VARIABLES_NUMBER_OF_TYPES) != CF_BLOCK_CASE_TYPE_UNDEFINED)
					{//#define, #undef, #ifdef, #ifndef, #elif defined, #elif !defined
						#ifdef CF_DEBUG_PARSE
						//cout << "hashTag = " << hashTag << endl;
						#endif

						readingHashTag = false;
						readingHashTagVariableName = true;
					}
					else if(getSupportedHashTagID(hashTag, hashTagArrayWithoutVariables, CF_HASH_TAG_WITHOUT_VARIABLES_NUMBER_OF_TYPES) != CF_BLOCK_CASE_TYPE_UNDEFINED)
					{//#else, #endif
						#ifdef CF_DEBUG_PARSE
						//cout << "hashTag = " << hashTag << endl;
						#endif

						//parse to end of line for consistency
						waitForNewLine(parseFileObject, level, lineCount, &currentLine, NULL, false, &parsingTextBlock);
						currentBlockInLayer = processHashTagStatement(parseFileObject, currentBlockInLayer, level, lineCount, &currentLine, &currentTextBlock, &hashTag, &hashTagVariableName, returnUponHashTagExitStatement);
						readingHashTag = false;
						readingHashTagPreceedingWhiteSpace = true;
					}
					else
					{//unsupported hash tag
						//cout << "unsupported hash tag" << endl;
						waitForNewLine(parseFileObject, level, lineCount, &currentLine, &currentTextBlock, true, &parsingTextBlock);

						readingHashTag = false;		
						readingHashTagPreceedingWhiteSpace = true;			
						hashTag = "";
					}
				}
				else if(c == CHAR_NEWLINE)
				{
					#ifdef CF_DEBUG_PARSE
					//cout << "\nhashTag = " << hashTag << endl;						
					#endif
					if(getSupportedHashTagID(hashTag, hashTagArrayWithoutVariables, CF_HASH_TAG_WITHOUT_VARIABLES_NUMBER_OF_TYPES) != CF_BLOCK_CASE_TYPE_UNDEFINED)
					{
						currentBlockInLayer = processHashTagStatement(parseFileObject, currentBlockInLayer, level, lineCount, &currentLine, &currentTextBlock, &hashTag, &hashTagVariableName, returnUponHashTagExitStatement);					
					}
					else
					{//unsupported #tag
						//waitForNewLine(parseFileObject, level, lineCount, &currentLine, &currentTextBlock, true, &parsingTextBlock);
						(*lineCount)++;
						currentTextBlock = currentTextBlock + currentLine;
						currentLine = "";
						hashTag = "";
					}

					readingHashTag = false;
					readingHashTagPreceedingWhiteSpace = true;
				}
				else
				{
					hashTag = hashTag + c;
				}

			}
			else if(readingHashTagVariableName)
			{
				if(SHAREDvarsClass().isWhiteSpace(c) || (c == CHAR_NEWLINE))
				{
					#ifdef CF_DEBUG_PARSE
					//cout << "\nhashTagVariableName = " << hashTagVariableName << endl;
					#endif
					
					if(SHAREDvarsClass().isWhiteSpace(c))
					{//parse to end of line for consistency
						waitForNewLine(parseFileObject, level, lineCount, &currentLine, NULL, false, &parsingTextBlock);
					}
					currentBlockInLayer = processHashTagStatement(parseFileObject, currentBlockInLayer, level, lineCount, &currentLine, &currentTextBlock, &hashTag, &hashTagVariableName, returnUponHashTagExitStatement);
						
					readingHashTagVariableName = false;
					readingHashTagPreceedingWhiteSpace = true;			
				}
				else
				{
					hashTagVariableName = hashTagVariableName + c;
				}
			}
			else
			{
				cout << "parseBlocksFromFile() token error:" << endl;
				cout << "char = " << c << endl;
				cout << "level = " << level << endl;
				cout << "*lineCount = " <<* lineCount << endl;
				cout << "currentLine = " << currentLine << endl;
				cout << "currentTextBlock = " << currentTextBlock << endl;
			}
		}
		else
		{
			parsingTextBlock = false;
		}
		if(*returnUponHashTagExitStatement)
		{
			parsingTextBlock = false;
		}
	}

	if(!(*returnUponHashTagExitStatement))
	{
		//add remaining code to text block (if any)
		currentTextBlock = currentTextBlock + currentLine;
		currentLine = "";
		if(currentTextBlock != "")
		{
			currentBlockInLayer->text = currentTextBlock;
			currentBlockInLayer->next = new CFblock();
			currentBlockInLayer = currentBlockInLayer->next;
		}			
	}	
			
	#ifdef CF_DEBUG_PARSE
	for(int i=0; i<level; i++)
	{
		cout << "\t";
	}
	cout << "\033[1;34m exiting parseTextBlock \033[0m" << endl;	//4 = blue
	#endif
	
	return currentBlockInLayer;
}

CFblock* readLargeComment(ifstream* parseFileObject, CFblock* firstBlockInComment, int level, int* lineCount, bool* parsingTextBlock)
{	
	CFblock* currentBlockInLayer = firstBlockInComment;
	/*
	#ifdef CF_DEBUG_PARSE
	for(int i=0; i<level; i++)
	{
		cout << "\t";
	}
	cout << "readLargeComment" << endl;
	#endif
	*/
	char c;
	string currentLine = "";
	currentLine = currentLine + CHAR_FORWARDSLASH + CHAR_STAR;
	string currentTextBlock = "";
	bool readingLargeComment = true;
	while(readingLargeComment)
	{
		if(parseFileObject->get(c))
		{
			currentLine = currentLine + c;
			#ifdef CF_DEBUG_PARSE
			cout << c;
			#endif

			if(c == CHAR_NEWLINE)
			{
				(*lineCount)++;
				currentTextBlock = currentTextBlock + currentLine;
				currentLine = "";
			}
			else
			{
				while((c == CHAR_STAR) && readingLargeComment)
				{
					if(parseFileObject->get(c))
					{
						currentLine = currentLine + c;
						#ifdef CF_DEBUG_PARSE
						cout << c;
						#endif

						if(c == CHAR_FORWARDSLASH)
						{
							readingLargeComment = false;

							currentTextBlock = currentTextBlock + currentLine;
							currentLine = "";
							currentBlockInLayer->type = CF_BLOCK_TYPE_COMMENT;
							currentBlockInLayer->commentType = CF_BLOCK_CASE_TYPE_COMMENT_LARGE;
							currentBlockInLayer->text = currentTextBlock;
							currentTextBlock = "";
							currentBlockInLayer->next = new CFblock();
							currentBlockInLayer = currentBlockInLayer->next;
						}
						else if(c == CHAR_NEWLINE)
						{
							(*lineCount)++;
							currentTextBlock = currentTextBlock + currentLine;
							currentLine = "";							
						}
						else
						{
						}
					}
					else
					{
						readingLargeComment = false;
						*parsingTextBlock = false;
					}
				}
			}
		}
		else
		{
			readingLargeComment = false;
			*parsingTextBlock = false;
		}
	}	
	
	/*
	#ifdef CF_DEBUG_PARSE
	for(int i=0; i<level; i++)
	{
		cout << "\t";
	}
	cout << "finished readLargeComment" << endl;
	#endif	
	*/
	return currentBlockInLayer;
}

CFblock* readSmallComment(ifstream* parseFileObject, CFblock* firstBlockInComment, int level, int* lineCount, bool* parsingTextBlock)
{	
	CFblock* currentBlockInLayer = firstBlockInComment;

	/*
	#ifdef CF_DEBUG_PARSE
	for(int i=0; i<level; i++)
	{
		cout << "\t";
	}
	cout << "readSmallComment" << endl;
	#endif
	*/
	char c;	
	string currentLine = "";
	currentLine = currentLine + CHAR_FORWARDSLASH + CHAR_FORWARDSLASH;
	string currentTextBlock = "";
	bool readingSmallComment = true;
	while(readingSmallComment)
	{
		if(parseFileObject->get(c))
		{
			currentLine = currentLine + c;
			#ifdef CF_DEBUG_PARSE
			cout << c;
			#endif

			if(c == CHAR_NEWLINE)
			{
				readingSmallComment = false;
				(*lineCount)++;
				currentTextBlock = currentTextBlock + currentLine;
				currentLine = "";
				currentBlockInLayer->type = CF_BLOCK_TYPE_COMMENT;
				currentBlockInLayer->commentType = CF_BLOCK_CASE_TYPE_COMMENT_SMALL;
				currentBlockInLayer->text = currentTextBlock;
				currentTextBlock = "";
				currentBlockInLayer->next = new CFblock();
				currentBlockInLayer = currentBlockInLayer->next;
			}
			else
			{
				//do nothing, still waiting for new line
			}
		}
		else
		{
			readingSmallComment = false;
			*parsingTextBlock = false;
		}
	}
	return currentBlockInLayer;
}

void waitForNewLine(ifstream* parseFileObject, int level, int* lineCount, string* currentLine, string* currentTextBlock, bool writeToTextBlock, bool* parsingTextBlock)
{
	/*
	#ifdef CF_DEBUG_PARSE
	for(int i=0; i<level; i++)
	{
		cout << "\t";
	}
	cout << "waitForNewLine" << endl;
	#endif
	*/
	char c;
	bool waitingForNewLine = true;
	while(waitingForNewLine)
	{
		if(parseFileObject->get(c))
		{
			*currentLine = *currentLine + c;
			#ifdef CF_DEBUG_PARSE
			cout << c;
			#endif

			if(c == CHAR_NEWLINE)
			{
				waitingForNewLine = false;
				(*lineCount)++;
				if(writeToTextBlock)
				{
					*currentTextBlock = *currentTextBlock +* currentLine;
					*currentLine = "";
				}					
			}
			else
			{
				//do nothing, still waiting for new line
			}
		}
		else
		{
			waitingForNewLine = false;
			*parsingTextBlock = false;
		}
	}
}

CFblock* processHashTagStatement(ifstream* parseFileObject, CFblock* currentBlockInLayer, int level, int* lineCount, string* currentLine, string* currentTextBlock, string* hashTag, string* hashTagVariableName, bool* returnUponHashTagExitStatement)
{	
	CFblock* firstHashTagBlock = new CFblock();
	CFblock* currentHashTagBlock = firstHashTagBlock;
	int hashTagID = getSupportedHashTagID(*hashTag, hashTagArrayAll, CF_HASH_TAG_NUMBER_OF_TYPES);
	currentHashTagBlock->hashTagID = hashTagID;
	currentHashTagBlock->hashTag = *hashTag;
	currentHashTagBlock->text = *currentLine;
	*currentLine = "";					
	if(hashTagID == CF_BLOCK_CASE_TYPE_INCLUDE)
	{
		#ifdef CF_DEBUG_PARSE
		for(int i=0; i<level; i++)
		{
			cout << "\t";
		}
		cout << "\033[1;35m processHashTagStatement:* hashTag = " << hashTagArrayAll[hashTagID] << ",* hashTagVariableName = " <<* hashTagVariableName << " \033[0m" << endl;	//5 = magenta
		#endif	
		
		//cout << "go down level (file)" << endl;
		currentHashTagBlock->type = CF_BLOCK_TYPE_INCLUDE;
		currentHashTagBlock->hashTagVariableName = *hashTagVariableName;
		
		string includeFileName = *hashTagVariableName;	
		includeFileName = removeSpecificCharactersFromString(includeFileName, CHAR_INVERTED_COMMAS);	//removing "" from #include "header.h"
		includeFileName = removeSpecificCharactersFromString(includeFileName, CHAR_BACKSLASH);	//removing "\" from #include "header.h"	//(NB relative paths in #include defined with a / or a \ indicate that the header h/hpp files not be in the same folder as source c/cpp files - which is not allowed by CF)	
		includeFileName = removeSpecificCharactersFromString(includeFileName, CHAR_FORWARDSLASH);	//removing "/" from #include "header.h" //(NB relative paths in #include defined with a / or a \ indicate that the header h/hpp files not be in the same folder as source c/cpp files - which is not allowed by CF)	

		currentHashTagBlock->lower = new CFblock();
		if(!parseBlocksFromFile(currentHashTagBlock->lower, includeFileName, (level+1)))
		{
			//file is not in the current directory, it may be a standard library header e.g. "math.h"; it will be ignored for the purposes of CFing
		}			
		//cout << "go up a level (file)" << endl;					
	}
	else if(hashTagID > CF_BLOCK_CASE_TYPE_INCLUDE)
	{
		currentHashTagBlock->type = CF_BLOCK_TYPE_CASE;
		if(getSupportedHashTagID(*hashTag, hashTagArrayDefinitions, CF_HASH_TAG_CASE_DEFINITIONS_NUMBER_OF_TYPES)  != CF_BLOCK_CASE_TYPE_UNDEFINED)
		{//#define, #undef
			#ifdef CF_DEBUG_PARSE
			for(int i=0; i<level; i++)
			{
				cout << "\t";
			}
			cout << "\033[1;35m processHashTagStatement:* hashTag = " << hashTagArrayAll[hashTagID] << ",* hashTagVariableName = " <<* hashTagVariableName << " \033[0m" << endl;	//5 = magenta
			#endif				
			//cout << "stay" << endl;
			currentHashTagBlock->hashTagVariableName = *hashTagVariableName;	//will be blank in the case of #else tag
		}		
		else if(getSupportedHashTagID(*hashTag, hashTagArrayExit, CF_HASH_TAG_CASE_EXIT_NUMBER_OF_TYPES) != CF_BLOCK_CASE_TYPE_UNDEFINED)
		{//#elif defined, #elif !defined, #else, #endif
			//cout << "go up level" << endl;
			//exit current codeblock being parsed
			currentHashTagBlock->hashTagVariableName = *hashTagVariableName;	//will be blank in the case of #else/#endif tags
			*returnUponHashTagExitStatement = true;
		}
		else if(getSupportedHashTagID(*hashTag, hashTagArrayEnter, CF_HASH_TAG_CASE_ENTER_NUMBER_OF_TYPES)  != CF_BLOCK_CASE_TYPE_UNDEFINED)
		{//#ifdef, #ifndef,
			#ifdef CF_DEBUG_PARSE
			for(int i=0; i<level; i++)
			{
				cout << "\t";
			}
			cout << "\033[1;35m processHashTagStatement:* hashTag = " << hashTagArrayAll[hashTagID] << ",* hashTagVariableName = " <<* hashTagVariableName << " \033[0m" << endl;	//5 = magenta
			#endif			
			//cout << "go down level" << endl;
			currentHashTagBlock->hashTagVariableName = *hashTagVariableName;	//will be blank in the case of #else tag
			bool endifTagFound = false;
			while(!endifTagFound)
			{
				currentHashTagBlock->lower = new CFblock();
				//currentHashTagBlock->next = new CFblock();
				bool ifStatementReturnedUponHashTagExitStatement = false;
				currentHashTagBlock->next = parseTextBlock(parseFileObject, currentHashTagBlock->lower, (level+1), lineCount, &ifStatementReturnedUponHashTagExitStatement);		//this should be (level+0) - temporarily changed for CF_DEBUG_PARSE
				currentHashTagBlock = currentHashTagBlock->next;
				#ifdef CF_DEBUG_PARSE
				for(int i=0; i<level; i++)
				{
					cout << "\t";
				}
				cout << "\033[1;35m processHashTagStatement:* hashTag = " << hashTagArrayAll[currentHashTagBlock->hashTagID] << ",* hashTagVariableName = " << currentHashTagBlock->hashTagVariableName << " \033[0m" << endl;	//5 = magenta
				#endif					
				if(ifStatementReturnedUponHashTagExitStatement)
				{
					if(currentHashTagBlock->hashTagID == CF_BLOCK_CASE_TYPE_ENDIF)
					{
						//cout << "successfully completed if/else statement" << endl;
						endifTagFound = true;
					}
				}
				else
				{
					cout << "processHashTagStatement() error: incomplete #ifdef/#ifndef found (no #else):" <<* hashTag << " " <<* hashTagVariableName << "line=" <<* lineCount << endl;
				}
			
			}
		}
		else
		{
			cout << "processHashTagStatement() error: bad hashTagID = " << hashTagID << endl;
		}
	}
	else
	{
		cout << "processHashTagStatement() error: hashTagID = " << hashTagID << endl;
	}
	
	*hashTagVariableName = "";
	*hashTag = "";
	
	if(*returnUponHashTagExitStatement)
	{
		//add the currently stored text block which contains data from the previous line(s) [note if the previous line was a #tag statement, the currentTextBlock is probably empty]					
		currentBlockInLayer->text = *currentTextBlock;
		*currentTextBlock = "";
		currentBlockInLayer->next = new CFblock();
		return currentHashTagBlock;
	}
	else
	{
		//add the currently stored text block which contains data from the previous line(s) [note if the previous line was a #tag statement, the currentTextBlock is probably empty]					
		currentBlockInLayer->text = *currentTextBlock;
		*currentTextBlock = "";
		currentBlockInLayer->next = firstHashTagBlock;
		currentHashTagBlock->next = new CFblock();
		currentBlockInLayer = currentHashTagBlock->next;
		
		return currentBlockInLayer;
		
	}
}

string removeLastCharactersFromString(string str, int numberOfCharactersToRemove)
{
	string newStr = "";
	if(numberOfCharactersToRemove < str.length())
	{
		for(int i=0; i<str.length()-numberOfCharactersToRemove; i++)
		{
			newStr = newStr + str[i];
		}
	}
	return newStr;
}

string removeSpecificCharactersFromString(string str, char characterToRemove)
{
	string newStr = "";
	for(int i=0; i<str.length(); i++)
	{
		if(str[i] != characterToRemove)
		{
			newStr = newStr + str[i];
		}
	}
	return newStr;
}



