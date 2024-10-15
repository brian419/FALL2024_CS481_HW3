/*
    Name: Jeongbin Son
    Email: json10@crimson.ua.edu
    Course Section: CS 481
    Homework #: 3
    To Compile: clang++ -Xpreprocessor -fopenmp -I/usr/local/opt/libomp/include -L/usr/local/opt/libomp/lib -lomp -std=c++11 -o gameoflife gameoflife.cpp    
    To Run: ./gameoflife <board size> <max generations> <num threads> <output directory>    
    ex: ./gameoflife 100 100 2 /Users/brianson/Desktop/cs481/hw3/output
    ex: ./gameoflife 5000 5000 1 /scratch/ualclsd0197/output_dir
*/

#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <sys/stat.h> 
#include <sys/types.h> 
#include <omp.h> 

using namespace std;
using namespace std::chrono;

// this function will create an output directory
void creatingoutputDirectory(const string &outputDirectory) {
    struct stat directoryInfo;

    // if the directory doesn't exist, create it
    if (stat(outputDirectory.c_str(), &directoryInfo) != 0) {
        cout << "Creating directory: " << outputDirectory << endl;
        if (mkdir(outputDirectory.c_str(), 0777) == -1) {
            cerr << "There was an error creating the directory " << outputDirectory << endl; // debug error message
            exit(1);
        }
    }
}

// this function will count the number of alive neighbors around a given cell
inline int countAliveNeighbors(const int *board, int index, int boardSize) {
    int aliveNeighborsCount = 0;
    int row = index / boardSize;
    int col = index % boardSize;

    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue; 
            int newRow = row + i;
            int newCol = col + j;
            if (newRow >= 0 && newRow < boardSize && newCol >= 0 && newCol < boardSize) {
                aliveNeighborsCount += board[newRow * boardSize + newCol];
            }
        }
    }
    return aliveNeighborsCount;
}

// this function will compute the next generation of the board
void nextGeneration(int *current, int *next, int boardSize) {
    #pragma omp parallel for
    for (int i = 0; i < boardSize * boardSize; ++i) {
        int aliveNeighbors = countAliveNeighbors(current, i, boardSize);
        next[i] = (current[i] == 1) ? (aliveNeighbors < 2 || aliveNeighbors > 3 ? 0 : 1) : (aliveNeighbors == 3 ? 1 : 0);
    }
}

// this function will check if the two boards are the same
bool isBoardSame(const int *board1, const int *board2, int size) {
    for (int i = 0; i < size * size; ++i) {
        if (board1[i] != board2[i]) {
            return false; 
        }
    }
    return true; 
}

// this function will write the final board to a file
void writingFinalBoardToFile(const int *board, const string &outputDirectory, int generation, int boardSize) {
    creatingoutputDirectory(outputDirectory);  
    ofstream outFile(outputDirectory + "/final_board_gen_" + to_string(generation) + ".txt");
    for (int i = 0; i < boardSize; ++i) {
        for (int j = 0; j < boardSize; ++j) {
            outFile << (board[i * boardSize + j] ? '*' : '.') << " ";
        }
        outFile << endl;
    }
    outFile.close();
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        cout << "Incorrect usage. Please type '" << argv[0] << " <board size> <max generations> <num threads> <output directory> \nex: ./gameoflifetest 100 100 2 /Users/brianson/Desktop/cs481/hw3/output'" << endl;
        return 1;
    }

    // command line arguments
    string outputDirectory = argv[4];
    int boardSize = stoi(argv[1]);         
    int maxAmountGenerations = stoi(argv[2]);    
    int numThreads = stoi(argv[3]);        

    omp_set_num_threads(numThreads);       

    int *board = new int[boardSize * boardSize](); 
    int *nextGenerationBoard = new int[boardSize * boardSize]();

    srand(12345); // fixed seed

    #pragma omp parallel for
    for (int i = 0; i < boardSize * boardSize; ++i) {
        board[i] = rand() % 2; 
    }

    auto start = high_resolution_clock::now(); 

    int generation = 0;   
    bool noChange = false; 

    while (generation < maxAmountGenerations && !noChange) {
        nextGeneration(board, nextGenerationBoard, boardSize); 

        if (isBoardSame(board, nextGenerationBoard, boardSize)) {
            noChange = true;  
        }

        swap(board, nextGenerationBoard); 
        generation++;
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    cout << "Simulation completed in " << generation << " generations and took " << duration.count() << " ms." << endl;

    writingFinalBoardToFile(board, outputDirectory, generation, boardSize);

    delete[] board; 
    delete[] nextGenerationBoard;

    return 0;
}
