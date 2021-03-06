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
 * File Name: CFcollapse.cpp
 * Author: Richard Bruce Baxter - Copyright (c) 2005-2018 Baxter AI (baxterai.com)
 * Project: Code Folder
 * Project Version: 1c2a 15-March-2018
 * /
 *******************************************************************************/


#include "CFcollapse.h"

//retainPPD is assumed true as not all #define statements are related to code blocks (and as such it is unknown whether they can be deleted entirely from the code)
bool collapseFile(CFblock* firstBlockInList, string outputFileName, bool foldInactive, bool foldComments, bool retainPPD, bool foldSpecific, string foldSpecificBlockNameSubset)
{
	bool result = true;
	
	CFpreprocessorDef* firstPPDinList = new CFpreprocessorDef();
	
	string outputFileString = "";
	if(!collapseBlockToFileObject(firstBlockInList, firstPPDinList, &outputFileString, 1, foldInactive, foldComments, retainPPD, foldSpecific, foldSpecificBlockNameSubset))
	{
		result = false;
	}
	
	ofstream writeFileObject(outputFileName.c_str());
	for(int i=0; i<outputFileString.length(); i++)
	{
		writeFileObject.put(outputFileString[i]);
	}
	//writeFileObject.put(CHAR_NEWLINE);	//required for Windows SW to re-read files?
	writeFileObject.close();
	
	return result;
}

bool collapseBlockToFileObject(CFblock* firstBlockInLayer, CFpreprocessorDef* firstPPDinList, string* outputFileString, int level, bool foldInactive, bool foldComments, bool retainPPD, bool foldSpecific, string foldSpecificBlockNameSubset)
{	
	bool result = true;
	
	bool ifCaseStillTrying = true;					//e.g. if "IF" failed, still looking for "ELIF"/"ELSE"
	bool ifCaseFoundSpecificBlock = false;	
	int retainPPDelseStatementTagID = CF_BLOCK_CASE_TYPE_UNDEFINED;	//used to modify #else statement when retainPPD is used
	string retainPPDelseStatementTagVariableName = "";		//used to modify #else statement when retainPPD is used
	bool specialCaseBlockAlwaysRetainTags = false;		//used in case of CF_SPECIAL_CASE_BLOCK_ALWAYS_RETAIN_TAGS_HEADER_IFNDEF
		
	CFblock* currentBlockInLayer = firstBlockInLayer;
	
	while(currentBlockInLayer->next != NULL)
	{
		#ifdef CF_DEBUG_COLLAPSE
		for(int i=0; i<level; i++)
		{
			cout << "\t";
		}
		cout << "\033[1;34m collapseBlockToFileObject: hashTag = " << currentBlockInLayer->hashTag << ", varName = " <<  currentBlockInLayer->hashTagVariableName << ", isActive = " << isPPDactive(firstPPDinList, currentBlockInLayer->hashTagVariableName) << " \033[0m" << endl;
		#endif	
		
		if(currentBlockInLayer->type == CF_BLOCK_TYPE_UNDEFINED)
		{
			cout << "collapseBlockToFileObject() error: (currentBlockInLayer->type == CF_BLOCK_TYPE_UNDEFINED)" << endl;
		}
		else if(currentBlockInLayer->type == CF_BLOCK_TYPE_COMMENT)
		{
			if(level == CF_INCLUDE_LEVEL_FIRST)
			{//only write code for 1st level (ie, output file)
				if(!foldComments)
				{
					*outputFileString = *outputFileString + currentBlockInLayer->text;
				}
				else
				{
					if(currentBlockInLayer->commentType == CF_BLOCK_CASE_TYPE_COMMENT_SMALL)
					{
						*outputFileString = *outputFileString + CHAR_NEWLINE;	//compensate for the fact char new lines are being recorded in small comments
					}
				}
			}
		}
		else if(currentBlockInLayer->type == CF_BLOCK_TYPE_INCLUDE)
		{
			if(level == CF_INCLUDE_LEVEL_FIRST)
			{//only write code for 1st level (ie, output file)
				if(foldComments)
				{									
					*outputFileString = *outputFileString + reworkCodeBlockHashTag(currentBlockInLayer, foldComments, false, NULL);
				}
				else
				{
					*outputFileString = *outputFileString + currentBlockInLayer->text;	//place the "#include _" file inf
				}			
			}
			if(!collapseBlockToFileObject(currentBlockInLayer->lower, firstPPDinList, outputFileString, level+1, foldInactive, foldComments, retainPPD, foldSpecific, foldSpecificBlockNameSubset))
			{
				result = false;
			}
			
		}
		else if(currentBlockInLayer->type == CF_BLOCK_TYPE_CASE)
		{
			if(currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_INCLUDE)
			{
				cout << "collapseBlockToFileObject() error: (currentBlockInLayer->type == CF_BLOCK_TYPE_CASE) && (currentBlockInLayer->hashTagID == CF_HASH_TAG_INCLUDE)" << endl;
			}
			else if(currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_DEFINE)
			{
				if(level == CF_INCLUDE_LEVEL_FIRST)
				{//only write code for 1st level (ie, output file)
					*outputFileString = *outputFileString + currentBlockInLayer->text;
				}			
				PPDadd(firstPPDinList, currentBlockInLayer->hashTagVariableName);
			}	
			else if((currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_IFDEF) || (currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_ELIF_DEFINED))
			{
				if(currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_IFDEF)
				{
					retainPPDelseStatementTagID = CF_BLOCK_CASE_TYPE_IFNDEF;
					retainPPDelseStatementTagVariableName = currentBlockInLayer->hashTagVariableName;			
				}
				
				bool passFoldRequirements = false;
				if(foldInactive)
				{
					if(ifCaseStillTrying)
					{
						if(isPPDactive(firstPPDinList, currentBlockInLayer->hashTagVariableName))
						{
							passFoldRequirements = true;
						}
					}
				}
				else
				{
					passFoldRequirements = true;
					if(foldSpecific)
					{
						#ifdef CF_FOLDSPECIFIC_MATCH_EXACT_STRING
						if(currentBlockInLayer->hashTagVariableName == foldSpecificBlockNameSubset)
						#else
						if((currentBlockInLayer->hashTagVariableName).find(foldSpecificBlockNameSubset) != CPP_STRING_FIND_RESULT_FAIL_VALUE)
						#endif
						{
							passFoldRequirements = false;
							ifCaseFoundSpecificBlock = true;
						}	
					}
				}
				if(passFoldRequirements)
				{
					if(!foldInactive || (foldInactive && retainPPD))
					{
						if(level == CF_INCLUDE_LEVEL_FIRST)
						{//only write code for 1st level (ie, output file)
							updateCodeBlock(outputFileString, currentBlockInLayer, foldInactive, foldComments, retainPPD, foldSpecific, ifCaseStillTrying, ifCaseFoundSpecificBlock, retainPPDelseStatementTagVariableName, CF_BLOCK_CASE_TYPE_ELIF_DEFINED, CF_BLOCK_CASE_TYPE_IFDEF);
						}
					}				
					if(!collapseBlockToFileObject(currentBlockInLayer->lower, firstPPDinList, outputFileString, level, foldInactive, foldComments, retainPPD, foldSpecific, foldSpecificBlockNameSubset))
					{
						result = false;
					}
					ifCaseStillTrying = false;	
				}
			}	
			else if((currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_IFNDEF) || (currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_ELIF_NDEFINED))
			{
				#ifdef CF_SPECIAL_CASE_BLOCK_ALWAYS_RETAIN_TAGS_HEADER_IFNDEF
				specialCaseBlockAlwaysRetainTags = isSpecialCaseBlockAlwaysRetainTags(currentBlockInLayer);
				#endif
								
				if(currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_IFNDEF)
				{
					retainPPDelseStatementTagID = CF_BLOCK_CASE_TYPE_IFDEF;
					retainPPDelseStatementTagVariableName = currentBlockInLayer->hashTagVariableName;			
				}	
				
				bool passFoldRequirements = false;
				//bool foldSpecificNegative = false;
				if(specialCaseBlockAlwaysRetainTags)
				{
					passFoldRequirements = true;
				}
				if(foldInactive)
				{
					if(ifCaseStillTrying)
					{
						if(!isPPDactive(firstPPDinList, currentBlockInLayer->hashTagVariableName))
						{
							passFoldRequirements = true;
						}
					}
				}
				else
				{
					passFoldRequirements = true;
					/*
					if(foldSpecific)
					{
						#ifdef CF_FOLDSPECIFIC_MATCH_EXACT_STRING
						if(currentBlockInLayer->hashTagVariableName == foldSpecificBlockNameSubset)
						#else
						if((currentBlockInLayer->hashTagVariableName).find(foldSpecificBlockNameSubset) != CPP_STRING_FIND_RESULT_FAIL_VALUE)
						#endif
						{
							passFoldRequirements = true;
							//foldSpecificNegative = false;
						}	
					}
					*/
				}				 	
				if(passFoldRequirements)
				{
					if(!foldInactive || (foldInactive && retainPPD) || specialCaseBlockAlwaysRetainTags)	//&& (!foundNegative) <- does not support collapse of endif tags also
					{
						if(level == CF_INCLUDE_LEVEL_FIRST)
						{//only write code for 1st level (ie, output file)
							updateCodeBlock(outputFileString, currentBlockInLayer, foldInactive, foldComments, retainPPD, foldSpecific, ifCaseStillTrying, ifCaseFoundSpecificBlock, retainPPDelseStatementTagVariableName, CF_BLOCK_CASE_TYPE_ELIF_NDEFINED, CF_BLOCK_CASE_TYPE_IFNDEF);
						}
					}
									
					if(!collapseBlockToFileObject(currentBlockInLayer->lower, firstPPDinList, outputFileString, level, foldInactive, foldComments, retainPPD, foldSpecific, foldSpecificBlockNameSubset))
					{
						result = false;
					}
					ifCaseStillTrying = false;	
				}			
			}	
			else if(currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_ELSE)
			{
				bool passFoldRequirements = false;
				if(foldInactive)
				{
					if(ifCaseStillTrying)
					{
						passFoldRequirements = true;
					}
				}
				else
				{
					passFoldRequirements = true;
				}	
				if(passFoldRequirements)
				{
					if(!foldInactive || (foldInactive && retainPPD))
					{
						if(level == CF_INCLUDE_LEVEL_FIRST)
						{//only write code for 1st level (ie, output file)
							updateCodeBlock(outputFileString, currentBlockInLayer, foldInactive, foldComments, retainPPD, foldSpecific, ifCaseStillTrying, ifCaseFoundSpecificBlock, retainPPDelseStatementTagVariableName, CF_BLOCK_CASE_TYPE_ELSE, retainPPDelseStatementTagID);
						}
					}			
					if(!collapseBlockToFileObject(currentBlockInLayer->lower, firstPPDinList, outputFileString, level, foldInactive, foldComments, retainPPD, foldSpecific, foldSpecificBlockNameSubset))
					{
						result = false;
					}
					ifCaseStillTrying = false;
				}
			}	
			else if(currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_ENDIF)
			{	
				if((!foldInactive && !(foldSpecific && ifCaseFoundSpecificBlock && ifCaseStillTrying)) || (foldInactive && retainPPD && !ifCaseStillTrying) || specialCaseBlockAlwaysRetainTags)
				{
					if(level == CF_INCLUDE_LEVEL_FIRST)
					{//only write code for 1st level (ie, output file)
						if(foldComments)
						{
							*outputFileString = *outputFileString + reworkCodeBlockHashTag(currentBlockInLayer, foldComments, false, NULL);
						}
						else
						{
							*outputFileString = *outputFileString + currentBlockInLayer->text;
						}
					}
				}
				
				ifCaseStillTrying = true;
				ifCaseFoundSpecificBlock = false;
				retainPPDelseStatementTagID = CF_BLOCK_CASE_TYPE_UNDEFINED;
				retainPPDelseStatementTagVariableName = "";	
				specialCaseBlockAlwaysRetainTags = false;								
				//assume at the end of the currentBlockInLayer 
			}	
			else if(currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_UNDEF)
			{
				if(level == CF_INCLUDE_LEVEL_FIRST)
				{//only write code for 1st level (ie, output file)
					*outputFileString = *outputFileString + currentBlockInLayer->text;
				}			
				PPDdeactivate(firstPPDinList, currentBlockInLayer->hashTagVariableName);
			}			
			else
			{
				cout << "collapseBlockToFileObject() error: (currentBlockInLayer->hashTagID == " << currentBlockInLayer->hashTagID << ")" << endl;
			}																									
		}
		else if(currentBlockInLayer->type == CF_BLOCK_TYPE_CODE)
		{
			if(level == CF_INCLUDE_LEVEL_FIRST)
			{//only write code for 1st level (ie, output file)
				*outputFileString = *outputFileString + currentBlockInLayer->text;
			}
		}
		else
		{
			cout << "collapseBlockToFileObject() error: (currentBlockInLayer->type == " << currentBlockInLayer->type << ")" << endl;
		}
		
		currentBlockInLayer = currentBlockInLayer->next;	
	}
	return result;
}

void updateCodeBlock(string* outputFileString, CFblock* currentBlockInLayer, bool foldInactive, bool foldComments, bool retainPPD, bool foldSpecific, bool ifCaseStillTrying, bool ifCaseFoundSpecificBlock, string retainPPDelseStatementTagVariableName, int originalBlockCase, int replacementBlockCase)
{
	bool replacementCase = false;
	if(currentBlockInLayer->hashTagID == originalBlockCase)
	{
		if(!foldInactive)
		{
			if(foldSpecific && ifCaseFoundSpecificBlock && ifCaseStillTrying)
			{
				replacementCase = true;
			}
		}
		else
		{
			if(retainPPD)
			{
				replacementCase = true;
			}
		}
	}

	if(replacementCase)
	{
		if(currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_ELSE)
		{
			currentBlockInLayer->hashTagVariableName = retainPPDelseStatementTagVariableName;	//temporarily set this value
		}
		*outputFileString = *outputFileString + reworkCodeBlockHashTag(currentBlockInLayer, foldComments, true, replacementBlockCase);
	}
	else
	{											
		if(foldComments)
		{
			*outputFileString = *outputFileString + reworkCodeBlockHashTag(currentBlockInLayer, foldComments, false, NULL);
		}
		else
		{
			*outputFileString = *outputFileString + currentBlockInLayer->text;
		}
	}
}
							
void PPDadd(CFpreprocessorDef* firstPPDinList, string PPD)
{
	CFpreprocessorDef* currentPPD = firstPPDinList;
	while(currentPPD->next != NULL)
	{
		currentPPD = currentPPD->next;
	}
	currentPPD->name = PPD;
	currentPPD->active = true;
	currentPPD->next = new CFpreprocessorDef();
}

void PPDdeactivate(CFpreprocessorDef* firstPPDinList, string PPD)
{
	CFpreprocessorDef* currentPPD = firstPPDinList;
	while(currentPPD->next != NULL)
	{
		if(currentPPD->name == PPD)
		{	
			currentPPD->active = false;
		}
		currentPPD = currentPPD->next;
	}
}

bool isPPDactive(CFpreprocessorDef* firstPPDinList, string PPD)
{
	bool isActive = false;
	CFpreprocessorDef* currentPPD = firstPPDinList;
	while(currentPPD->next != NULL)
	{
		if(currentPPD->name == PPD)
		{
			if(currentPPD->active)
			{
				isActive = true;
			}
		}
		currentPPD = currentPPD->next;
	}
	return isActive;	
}

string reworkCodeBlockHashTag(CFblock* currentBlockInLayer, bool foldComments, bool modifyHashTag, int newHashTagID)
{
	string hashTagLineWithAppendedCommentsRemoved = "";
	
	string prependedWhiteSpace = extractPrependedWhiteSpace(currentBlockInLayer->text);
	
	string hashTag = currentBlockInLayer->hashTag;	//OR hashTagArrayAll[currentBlockInLayer->hashTagID];
	if(modifyHashTag)
	{		
		hashTag = hashTagArrayAll[newHashTagID];
	}
	
	//cout << "2" << endl;
	string comments = "";
	if(foldComments)
	{
		if((currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_ELSE) || (currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_ENDIF))
		{//if original hashTagID is #else/#endif, then expect no hashTagVariableName in text
			int indexOfComments = (currentBlockInLayer->text).find(currentBlockInLayer->hashTag) + (currentBlockInLayer->hashTag).length();
			int lengthOfComments = (currentBlockInLayer->text).length() - indexOfComments  - 1;	//-1 to ignore the CHAR_NEW
			comments = (currentBlockInLayer->text).substr(indexOfComments, lengthOfComments);
			/*
			cout << "hashTagLength = " << (currentBlockInLayer->hashTag).length() << endl;
			cout << "indexOfComments = " << indexOfComments << endl;	
			cout << "lengthOfComments = " << lengthOfComments << endl;							
			*/
		}
		else
		{
			int indexOfComments = (currentBlockInLayer->text).find(currentBlockInLayer->hashTagVariableName) + (currentBlockInLayer->hashTagVariableName).length();
			int lengthOfComments = (currentBlockInLayer->text).length() - indexOfComments  - 1;	//-1 to ignore the CHAR_NEW
			comments = (currentBlockInLayer->text).substr(indexOfComments, lengthOfComments);
		}	
	}

	/*for safety only
	if(!modifyHashTag)
	{
		if((currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_ELSE) || (currentBlockInLayer->hashTagID == CF_BLOCK_CASE_TYPE_ENDIF))
		{
			currentBlockInLayer->hashTagVariableName = "";
		}
	}
	*/
			
	//cout << "\n reworkCodeBlockHashTag():" << endl;
	//cout << "\033[1;36m orig text: " << currentBlockInLayer->text << "\033[0m" << endl;
	//cout << "prependedWhiteSpace = " << prependedWhiteSpace << endl;
	//cout << "hashTag = " << hashTag << endl;
	//cout << "currentBlockInLayer->hashTagVariableName = " << currentBlockInLayer->hashTagVariableName << endl;
	//cout << "comments = " <<comments << endl;
	
	hashTagLineWithAppendedCommentsRemoved = hashTagLineWithAppendedCommentsRemoved + prependedWhiteSpace + hashTag + CHAR_SPACE + currentBlockInLayer->hashTagVariableName + comments + CHAR_NEWLINE;
	
	/*
	cout << "\n reworkCodeBlockHashTag():" << endl;
	cout << "\033[1;36m orig text: " << currentBlockInLayer->text << "\033[0m" << endl;
	cout << "prependedWhiteSpace = " << prependedWhiteSpace << endl;
	cout << "hashTagID = " << currentBlockInLayer->hashTagID << endl;
	cout << "hashTag = " << currentBlockInLayer->hashTag << endl;
	cout << "newHashTagID = " << newHashTagID << endl;	
	cout << "newHashTag = " << hashTag << endl;
	cout << "comments = " << comments << endl;
	cout << "currentBlockInLayer->hashTagVariableName = " << currentBlockInLayer->hashTagVariableName << endl;
	cout << "hashTagLineWithAppendedCommentsRemoved = " << hashTagLineWithAppendedCommentsRemoved << endl;	
	cout << "end reworkCodeBlockHashTag" << endl;
	*/
	
	return hashTagLineWithAppendedCommentsRemoved;
}

string extractPrependedWhiteSpace(string text)
{
	bool stillWhiteSpace = true;
	string whiteSpace = "";
	for(int i=0; i<text.length(); i++)
	{
		if(stillWhiteSpace)
		{
			if(SHAREDvarsClass().isWhiteSpace(text[i]))
			{
				whiteSpace = whiteSpace + text[i];	
			}
			else
			{
				stillWhiteSpace = false;
			}
		}
	}
	return whiteSpace;
}

#ifdef CF_SPECIAL_CASE_BLOCK_ALWAYS_RETAIN_TAGS_HEADER_IFNDEF
bool isSpecialCaseBlockAlwaysRetainTags(CFblock* currentBlockInLayer)
{
	bool specialCaseBlockAlwaysRetainTags = false;
	int indexOfSpecial = (currentBlockInLayer->hashTagVariableName).find(CF_SPECIAL_CASE_BLOCK_ALWAYS_RETAIN_TAGS_HEADER_IFNDEF_TAG_NAME_PREPEND);
	if(indexOfSpecial == 0)
	{
		specialCaseBlockAlwaysRetainTags = true;
	}
	return specialCaseBlockAlwaysRetainTags;
}
#endif
