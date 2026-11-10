#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

// Functie pentru verificarea numerelor prime
bool isPrime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0) return false;
    return true;
}

int main() {
    int num_processes = 10;
    int range_size = 1000;
    
    // Arrays pentru Pipe-uri (10 in jos pt interval, 10 in sus pt rezultate)
    int parent_to_child[10][2];
    int child_to_parent[10][2];
    
    for (int i = 0; i < num_processes; ++i) {
        if (pipe(parent_to_child[i]) == -1 || pipe(child_to_parent[i]) == -1) {
            cerr << "Pipe failed" << endl;
            return 1;
        }
        
        pid_t pid = fork();
        
        if (pid == 0) { // SUNTEM IN PROCESUL COPIL
            close(parent_to_child[i][1]); // Inchidem scrierea de la parinte (copilul doar citeste)
            close(child_to_parent[i][0]); // Inchidem citirea catre parinte (copilul doar scrie rezultate)
            
            int interval[2];
            read(parent_to_child[i][0], interval, sizeof(interval));
            
            int start = interval[0];
            int end = interval[1];
            
            for (int num = start; num <= end; ++num) {
                if (isPrime(num)) {
                    write(child_to_parent[i][1], &num, sizeof(num)); // Trimitem numerele gasite "prin teava"
                }
            }
            
            int sentinel = -1;
            write(child_to_parent[i][1], &sentinel, sizeof(sentinel)); // Spunem parintelui ca am terminat
            
            close(parent_to_child[i][0]);
            close(child_to_parent[i][1]);
            exit(0);
        }
    }
    
    // SUNTEM IN PROCESUL PARINTE
    for (int i = 0; i < num_processes; ++i) {
        close(parent_to_child[i][0]); 
        close(child_to_parent[i][1]);
        
        // Trimitem intervalul aferent fiecarui copil (ex: 1-1000, 1001-2000, etc)
        int interval[2] = {i * range_size + 1, (i + 1) * range_size};
        write(parent_to_child[i][1], interval, sizeof(interval));
    }
    
    // Citim si afisam numerele din "teava" de sus
    for (int i = 0; i < num_processes; ++i) {
        int prime;
        cout << "\n[Proces Copil " << i + 1 << "] Numere prime in intervalul " << i * range_size + 1 << " - " << (i + 1) * range_size << ":\n";
        while (read(child_to_parent[i][0], &prime, sizeof(prime)) > 0) {
            if (prime == -1) break; // Copilul ne-a anuntat ca s-a oprit
            cout << prime << " ";
        }
        cout << endl;
    }
    
    // Parintele asteapta sa moara toti copiii frumos (evitare zombie)
    for (int i = 0; i < num_processes; ++i) wait(NULL);
    
    return 0;
}
