#include <iostream>
#include <fstream>
#include <string>
#include <pthread.h>
#include <math.h>
#include <algorithm>
using namespace std;

struct Symbol {
    char ch;    // Symbol's character                    
    int dec;    // Symbol's decimal
    string code;    // Symbol's code
    int freq;   // Symbol's frequency
};

struct Params {
    pthread_mutex_t* mutex;      // for synchronization.
    int *tId;
    int n;      // number of Symbols.
    int fxdLen;     // fixed length of symbol in decompressed message.
    Symbol* pSym;   // Symbol's array.
    string* cmpMsg;  // compressed message.
};

struct Func1Res {
    string code;    // Symbol's code
    int freq;   // Symbol's frequency
    int id;
};

struct Func2Res {
    char ch; // Decoded character
    int id;
};


string Binarize(int decimal, int fxdLen);   // convert decimal to fixed length binary.           
int Frequence(string* cmpMsg, int fxdLen, string code);  // calculate frequency in compressed message.
void Print(Symbol* pSym, int id);   // Print out the symbol's information.
int FixedLength(Symbol* pSym, int n);   // calcuate fixed length of Symbol.
void* Func1(void* args);
void* Func2(void* args);


int main() {
    string line = "", cmpMsg = "", dmpMsg = "";
    int nSymbol = 0, i = 0, fxdLen = 0, mSymbol = 0, nTotalThread = 0;
    Symbol* pSymbols = NULL;

    if (getline(cin, line)) {

        nSymbol = atoi(line.c_str());

        pSymbols = new Symbol[nSymbol];
        for (int i = 0; i < nSymbol; i++) {
            getline(cin, line);
            pSymbols[i].ch = line[0];
            pSymbols[i].dec = atoi(line.substr(2, line.size() - 2).c_str());
            pSymbols[i].code = "";
            pSymbols[i].freq = 0;
        }

        getline(cin, cmpMsg);

        fxdLen = FixedLength(pSymbols, nSymbol);
        mSymbol = cmpMsg.size() / fxdLen;
        nTotalThread = nSymbol + mSymbol;
    }

    // Create threads
    pthread_t* threadIds = NULL;    // Total ThreadIds
    pthread_mutex_t lock1;   // lock1 for Func1, lock2 for Func2
    Params param;

    // Dynamic initialization.
    threadIds = new pthread_t[nTotalThread];

    if (pthread_mutex_init(&lock1, NULL) != 0) {
        cout << "mutex init has failed" << endl;
        return 1;
    }
    
    int tid=0;

    param = {};
    param.tId = &tid;
    param.mutex = &lock1;
    param.cmpMsg = &cmpMsg;
    param.fxdLen = fxdLen;
    param.pSym = pSymbols;
    param.n = nSymbol;

    dmpMsg.resize(mSymbol, ' ');
    cout << "Alphabet:" << endl;

    // Create threads for Task1.
    for (i = 0; i < nSymbol; i++) {
        int ret = pthread_create(&threadIds[i], NULL, Func1, (void*)&param);
        if (ret != 0) {
            cout << "Error: pthread_create() failed" << endl;
            return 1;
        }
    }

    // Wait for Func1 threads signal for Func2.
    for (i = 0; i < nSymbol; i++) {
	Func1Res *r;
	pthread_join(threadIds[i], (void**)&r);
	pSymbols[r->id].code = r->code;
	pSymbols[r->id].freq = r->freq;
	delete r;
    }

    // Print out Func1's results
    for (i = 0; i < nSymbol; i++) {
        Print(pSymbols, i);
    }

    cout << endl << "Decompressed message: ";
    
    tid=0;
    
    // Create Func2 Threads
    for (i = 0; i < mSymbol; i++) {
        int ret = pthread_create(&threadIds[i], NULL, Func2, (void*)&param);
        if (ret != 0) {
            cout << "Error: pthread_create() failed" << endl;
            return 1;
        }
    }

    //wait for all threads termination
    for (i = 0; i < mSymbol; i++) {
	Func2Res *r;
        pthread_join(threadIds[i], (void**)&r);
	dmpMsg[r->id] = r->ch;
	delete r;
    }

    cout << dmpMsg << endl;

    // Destroy all mutexes for thread.
    pthread_mutex_destroy(&lock1);

    // Unallocate all variables.
    delete[] pSymbols;
    delete[] threadIds;

    return 0;
}


int FixedLength(Symbol* pSym, int n) {
    int topValue = 0;

    for (int i = 0; i < n; i++) {
        if (topValue < pSym[i].dec) topValue = pSym[i].dec;
    }
    return ceil(log2(topValue + 1)) > 0 ? (int)ceil(log2(topValue + 1)) : 0;
}


string Binarize(int decimal, int fxdLen) {
    string code = "";

    while (decimal) {
        code += (decimal % 2) + '0';
        decimal /= 2;
    }

    while (code.size() < fxdLen) {
        code.append("0");
    }

    reverse(code.begin(), code.end());

    return code;

}


int Frequence(string *cmpMsg, int fxdLen, string code) {
    int freq = 0;
    // cout << *cmpMsg << endl;
    for (int j = 0; j < cmpMsg->size(); j += fxdLen) {
	// cout << code << ", " << cmpMsg->substr(j, fxdLen) << endl;
        if (cmpMsg->substr(j, fxdLen) == code) freq++;
    }
    return freq;
}


void Print(Symbol* pSym, int id) {
    cout << "Character: " << pSym[id].ch << ", "
        << "Code: " << pSym[id].code << ", "
        << "Frequency: " << pSym[id].freq << endl;
}


void* Func1(void* args) {
    Params* params = (Params*)args;
    pthread_mutex_lock(params->mutex);
    int tid = (*params->tId)++;
    pthread_mutex_unlock(params->mutex);
    auto code = Binarize(params->pSym[tid].dec, params->fxdLen);

    // Get binary from decimal.
    return (void*)new Func1Res{
	code,
	Frequence(params->cmpMsg, params->fxdLen, code),
	tid
    };
}


void* Func2(void* args) {
    Params* params = (Params*)args;
    pthread_mutex_lock(params->mutex);
    int tid = (*params->tId)++;
    pthread_mutex_unlock(params->mutex);

    auto codedchar = params->cmpMsg->substr(tid * (params->fxdLen),
					    params->fxdLen);
    
    for (int i = 0; i < params->n; i++) {
        if (params->pSym[i].code == codedchar) {
	    return new Func2Res {params->pSym[i].ch, tid};
        }
    }
    return nullptr;
}
