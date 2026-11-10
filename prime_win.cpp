#include <windows.h>
#include <iostream>
#include <string>

using namespace std;

bool isPrime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0) return false;
    return true;
}

int main(int argc, char* argv[]) {
    // Daca am primit argumente pe linia de comanda, INSEAMNA CA SUNTEM PROCESUL COPIL
    if (argc == 3) {
        HANDLE hRead = (HANDLE)stoull(argv[1]);
        HANDLE hWrite = (HANDLE)stoull(argv[2]);
        
        int interval[2];
        DWORD readBytes, written;
        ReadFile(hRead, interval, sizeof(interval), &readBytes, NULL);
        
        int start = interval[0], end = interval[1];
        for (int num = start; num <= end; ++num) {
            if (isPrime(num)) WriteFile(hWrite, &num, sizeof(num), &written, NULL);
        }
        
        int sentinel = -1;
        WriteFile(hWrite, &sentinel, sizeof(sentinel), &written, NULL);
        return 0;
    }
    
    // SUNTEM IN PROCESUL PARINTE
    int num_processes = 10, range_size = 1000;
    
    HANDLE p2c_read[10], p2c_write[10], c2p_read[10], c2p_write[10];
    PROCESS_INFORMATION pi[10];
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE }; // Permite copilului sa mosteneasca
    
    for (int i = 0; i < num_processes; ++i) {
        CreatePipe(&p2c_read[i], &p2c_write[i], &sa, 0);
        CreatePipe(&c2p_read[i], &c2p_write[i], &sa, 0);
        
        // Impiedicam copilul sa capete accidental acces la capetele care trebuie sa ramana la parinte
        SetHandleInformation(p2c_write[i], HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(c2p_read[i], HANDLE_FLAG_INHERIT, 0);
        
        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
        
        // Construim comanda care deschide exact acest .exe dar ii paseaza handle-urile prin argumente
        string cmd = string(argv[0]) + " " + to_string((unsigned long long)p2c_read[i]) + " " + to_string((unsigned long long)c2p_write[i]);
        char cmdLine[256];
        strcpy(cmdLine, cmd.c_str());
        
        CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi[i]);
        
        // Ii trimitem intervalul de numere in care sa caute
        int interval[2] = {i * range_size + 1, (i + 1) * range_size};
        DWORD written;
        WriteFile(p2c_write[i], interval, sizeof(interval), &written, NULL);
    }
    
    // Citim numerele gasite de toti copiii prin "teava"
    for (int i = 0; i < num_processes; ++i) {
        int prime;
        DWORD readBytes;
        cout << "\n[Proces Copil " << i + 1 << "] Numere prime in intervalul " << i * range_size + 1 << " - " << (i + 1) * range_size << ":\n";
        while (ReadFile(c2p_read[i], &prime, sizeof(prime), &readBytes, NULL) && readBytes > 0) {
            if (prime == -1) break; // -1 este semnalul ca a terminat
            cout << prime << " ";
        }
        cout << endl;
    }
    
    return 0;
}