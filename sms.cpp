#ifdef _DEVMAX_INTERPRETER_
extern "C"
{
	// DevMax C++ interpreter I/O API
	void PrintString(const char*);

	// Add standard C API we are using so that we can interpret without copying headers
	typedef void FILE;
	FILE *fopen(const char*, const char*);
	int fgetc(FILE*);
	void memcpy(void*, void*, int);
	void* malloc(unsigned int);
	int sprintf(char *, const char *, ...);
	int strcmp(char *, char *);
}

#else // If we are compiling normally

#include <string.h>
#include <stdio.h>

void PrintString(const char* str)
{
	printf("%s", str);
}
#endif

#include "LVitaCpp/LTTree.h"

#define SMS_DATA_FILE "sms.txt"

class SMSDateRecord
{
public:
	SMSDateRecord(bool act, char* dateBuf, int dateBufLength)
	{
		isActivationDate = act;
		memcpy(dateStr, dateBuf, dateBufLength);
		dateStr[dateBufLength] = '\0';
	}

#ifdef _DEVMAX_INTERPRETER_
	// Unfortunately, our C++ interpreter can't handle `new` so I need to use malloc
	static SMSDateRecord *Make(bool act, char* dateBuf, int dateBufLength)
	{
		SMSDateRecord* r = (SMSDateRecord*)malloc(sizeof(SMSDateRecord));
		r->isActivationDate = act;
		memcpy(r->dateStr, dateBuf, dateBufLength);
		r->dateStr[dateBufLength] = '\0';

		return r;
	}
#endif

	// Date string
	char dateStr[9];

	// Flag to determine if this is Activation/Deactivation node
	bool isActivationDate;

	// The other date in the activation tree
	// If this is an activation date then otherDate will be deactivation date; and vice versa
	LTTree<char, SMSDateRecord*> *otherDate;
};

class PhoneNumberInfo
{
public:
	PhoneNumberInfo(char *numstr, int length)
	{
		memcpy(phoneNumber, numstr, length);
		phoneNumber[length] = '\0';
		activationTree = default_create_node<char, SMSDateRecord*>(nullptr, 0);
	}

#ifdef _DEVMAX_INTERPRETER_
	static PhoneNumberInfo *Make(char *numstr, int length)
	{
		PhoneNumberInfo* r = (PhoneNumberInfo*)malloc(sizeof(PhoneNumberInfo));
		memcpy(r->phoneNumber, numstr, length);
		r->phoneNumber[length] = '\0';
		r->activationTree = default_create_node<char, SMSDateRecord*>(nullptr, 0);

		return r;
	}
#endif

	// Add an activation/deactivation date from file
	LTTree<char, SMSDateRecord*> *AddDate(FILE *file, bool act)
	{
		char dateBuf[8];
		int dateBufLength = 0;
		int c;
		auto activationDateNode = activationTree;
		while ((c = fgetc(file)) != ',' && c != '\n' && c != -1)
		{
			if (c >= '0' && c <= '9') // Skip the dash in date separator
			{
				dateBuf[dateBufLength++] = c;
				activationDateNode = activationDateNode->CreateChild(c);
			}
		}
		dateBuf[dateBufLength] = '\0';

		auto record = activationDateNode->data;
		if (record != nullptr)
		{
			// The activation date of the current record is the deactivation date of some record we processed in the past
			// i.e. there are two records for this phone number of form [S -> A] [A -> D] in the file where A, D are activation
			// and deactivation date of this line
			// So we merge them to [S -> D] by changing activationDateNode

			// The other situation is that: The deactivation date of the current record is the activation date of some record we processed in the past
			// we have two records [A - D] [D - S] so we likewise merge them

			// Fortunately, the same code works in both situation!
			activationDateNode = record->otherDate;

			// TODO free(activationDateNode->data);
		}
		else
		{
#ifdef _DEVMAX_INTERPRETER_
			// Activation date has never appeared, so we make new record
			activationDateNode->data = SMSDateRecord::Make(act, dateBuf, dateBufLength);
#else
			activationDateNode->data = new SMSDateRecord(act, dateBuf, dateBufLength);
#endif
		}

		return activationDateNode;
	}

	// Null-terminated phone number
	char phoneNumber[20];

	// Tree of SMS activation/deactivation records
	// Note that the root node is the `present`
	LTTree<char, SMSDateRecord*> *activationTree;
};

int main(int argc, const char **argv)
{
	PrintString(SMS_DATA_FILE"\n");

	auto file = fopen(SMS_DATA_FILE, "rb");
	if (file == nullptr)
	{
		PrintString("Error: input file not found.\n");
		return -1;
	}

	LTTree<char, PhoneNumberInfo*> phoneTree(0);
	phoneTree.data = nullptr;

	char numBuf[20];
	int numBufLength;
	while (true)
	{
		// Process a line of record	
		int c = fgetc(file);
		if (c == -1)
			break;

		// Expect a phone number; if not found, this is the end of file
		if (c < '0' || c > '9')
		{
			PrintString("Expect phone number at beginning of line.\n");
			break;
		}

		// Get/create the node for the phone number
		numBufLength = 0;
		auto phoneNode = &phoneTree;
		do
		{
			phoneNode = phoneNode->CreateChild(c);
			numBuf[numBufLength++] = c;
		} while ((c = fgetc(file)) != ',' && c != -1);

		numBuf[numBufLength] = '\0';
		auto numInfo = phoneNode->data;
		if (numInfo == nullptr)
#ifdef _DEVMAX_INTERPRETER_
			phoneNode->data = numInfo = PhoneNumberInfo::Make(numBuf, numBufLength);
#else
			phoneNode->data = numInfo = new PhoneNumberInfo(numBuf, numBufLength);
#endif

		auto activationDateNode = numInfo->AddDate(file, true);
		auto deactivationDateNode = numInfo->AddDate(file, false);

		activationDateNode->data->otherDate = deactivationDateNode;
		deactivationDateNode->data->otherDate = activationDateNode;
	}

	// Go through the tree to collect activation dates for each phone number
	int dummy = 0;
	phoneTree.DFS<int>(
		[](LTTree<char, PhoneNumberInfo*> *node, int &state)
	{
		if (node->data != nullptr)
		{
			PrintString(node->data->phoneNumber);
			PrintString(" -> ");

			auto actTree = node->data->activationTree;
			if (actTree->data != nullptr)
			{
				// There is a record at the root of the tree
				// This means that the number is still used at the present
				// The corresponding date is the otherDate field
				auto record = actTree->data->otherDate->data;
				PrintString(record->dateStr);
			}
			else
			{
				// The number is no longer active at the present
				// We have to DFS to search for it
				SMSDateRecord *latest = nullptr;
				actTree->DFS<SMSDateRecord*>(
					[](LTTree<char, SMSDateRecord*> *node, SMSDateRecord *&latest)
				{
					auto record = node->data;
					if (record != nullptr && !record->isActivationDate)
					{
						if (latest == nullptr || strcmp(latest->dateStr, record->dateStr) < 0)
							latest = record;
					}

					return false;
				},
					[](LTTree<char, SMSDateRecord*> *node, SMSDateRecord *&latest) { /* no op */ },
					latest
				);
				PrintString(latest->otherDate->data->dateStr);
			}

			PrintString("\n");
		}

		return false;
	},
		[](LTTree<char, PhoneNumberInfo*> *node, int &state) { /* no op */ },
		dummy
	);

	return 0;
}