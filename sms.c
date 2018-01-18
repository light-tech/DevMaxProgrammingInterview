// DevMax C++ interpreter API function
#ifdef __cplusplus
extern "C"
{
#endif
	void PrintString(const char*);

	// C API we are using
	void* fopen(const char*, const char*);
	int fgetc(void*);
	void* malloc(unsigned int);
	int sprintf(char *, const char *, ...);
#ifdef __cplusplus
}
#endif

#ifndef __cplusplus
typedef int bool;
#define true 1
#define false 0
#endif

void PrintChar(char c)
{
	char buf[2] = { c, '\0' };
	PrintString(buf);
}

void PrintInt(int d)
{
	char buf[10];
	sprintf(buf, "%d", d);
	PrintString(buf);
}

#define DATA_PATH 

#define SMS_DATA_FILE DATA_PATH"sms.txt"

typedef struct SMSDateRecord
{
	bool isActivationDate;
	void *otherDate;
} SMSDateRecord;

SMSDateRecord *MakeSMSDateRecord(bool act)
{
	SMSDateRecord* r = (SMSDateRecord*)malloc(sizeof(SMSDateRecord));	
	r->isActivationDate = act;
	return r;
}

typedef struct DigitTree
{
	void *data;
	DigitTree *children[10];
} DigitTree;

DigitTree *MakeDigitTree()
{
	DigitTree *t = (DigitTree*)malloc(sizeof(DigitTree));

	if (t == nullptr)
	{
		PrintString("Error: Can't allocate memory!\n");
		return nullptr;
	}

	t->data = nullptr;
	for(int i = 0; i < 10; i++)
		t->children[i] = nullptr;
	return t;
}

DigitTree* GetChild(DigitTree *t, char c, bool create)
{
	int d = c - '0';
	if (d < 0 || d > 9)
	{
		PrintInt(d);
		PrintString(" should not appear in a digit tree!\n");
		return nullptr;
	}

	if (create && t->children[d] == nullptr)
	{
		/*PrintString("Create new child node ");
		PrintInt(d);
		PrintString("\n");*/
		t->children[d] = MakeDigitTree();
	}

	return t->children[d];
}

int main(int argc, const char **argv)
{
	PrintString(SMS_DATA_FILE"\n");

	auto file = fopen(SMS_DATA_FILE, "rb");
	if (file == nullptr)
	{
		PrintString("Error: input file not found.\n");
		return -1;
	}

	// new Tree<char, Tree<char, SMSDateRecord*>*>();
	auto phoneTree = MakeDigitTree();

	int line_num = 1;
	while (true)
	{
		PrintString("Line ");
		PrintInt(line_num);

		// Process a line of record	
		int c = fgetc(file);
		if (c == -1)
		{
			PrintString(" End of file!\n");
			break;
		}
		
		// Expect a phone number; if not found, this is the end of file
		if (c < '0' || c > '9')
		{
			PrintString(" Expect phone number at beginning of line.\n");
			break;
		}

		PrintString(" phone #");
		// Get/create the node for the phone number
		auto phoneNode = phoneTree;
		do
		{
			PrintChar(c);
			phoneNode = GetChild(phoneNode, c, true);
		} while ((c = fgetc(file)) != ',' && c != -1);

		// Prepare the tree @ the leaf phoneNode
		// Note that the root node is the `present`
		DigitTree *phoneNodeDateTree;
		if (phoneNode->data == nullptr)
		{
			phoneNodeDateTree = MakeDigitTree();
			phoneNode->data = phoneNodeDateTree;
		}
		else
			phoneNodeDateTree = (DigitTree*)phoneNode->data;

		// Get/create a node corresponding to activation date
		auto activationDateNode = phoneNodeDateTree;
		PrintString(" from ");
		while ((c = fgetc(file)) != ',' && c != -1)
		{
			PrintChar(c);
			if (c >= '0' && c <= '9') // Skip the dash in date separator
				activationDateNode = GetChild(activationDateNode, c, true);
		}

		// Get/create a node corresponding to deactivation date
		auto deactivationDateNode = phoneNodeDateTree;
		PrintString(" to ");
		while ((c = fgetc(file)) != '\n' && c != -1)
		{
			PrintChar(c);
			if (c != '-')
				deactivationDateNode = GetChild(deactivationDateNode, c, true);
		}
		
		// We are not considering malicious input such as there are two records with the same activation date [A - D1] [A - D2]

		if (activationDateNode->data != nullptr)
		{
			// The activation date of the current record is the deactivation date of some record we processed in the past
			// i.e. there are two records for this phone number of form [S -> A] [A -> D] in the file where A, D are activation
			// and deactivation date of this line
			// So we merge them to [S -> D] by changing activationDateNode
			auto record = (SMSDateRecord*)activationDateNode->data;
			activationDateNode = (DigitTree*)record->otherDate;
			//free(activationDateNode->data);
		}
		else
		{
			// Activation date has never appeared, so we make new record
			activationDateNode->data = MakeSMSDateRecord(true);
		}

		if (deactivationDateNode->data != nullptr)
		{
			// The deactivation date of the current record is the activation date of some record we processed in the past
			// we have two records [A - D] [D - S] so we likewise merge them
			auto record = (SMSDateRecord*)deactivationDateNode->data;
			deactivationDateNode = (DigitTree*)record->otherDate;
			//delete deactivationDateNode->data;
		}
		else
		{
			deactivationDateNode->data = MakeSMSDateRecord(false);
		}
		
		((SMSDateRecord*)activationDateNode->data)->otherDate = deactivationDateNode;
		((SMSDateRecord*)deactivationDateNode->data)->otherDate = activationDateNode;

		line_num++;
		PrintString("\n");
	}
	
	return 0;
}